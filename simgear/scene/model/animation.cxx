// animation.cxx - classes to manage model animation.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <string.h>             // for strcmp()
#include <math.h>
#include <algorithm>
#include <functional>

#include <OpenThreads/Atomic>
#include <OpenThreads/Mutex>
#include <OpenThreads/ReentrantMutex>
#include <OpenThreads/ScopedLock>

#include <osg/AlphaFunc>
#include <osg/Drawable>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LOD>
#include <osg/Math>
#include <osg/Object>
#include <osg/StateSet>
#include <osg/Switch>
#include <osg/TexMat>
#include <osg/Texture2D>
#include <osg/Transform>
#include <osg/Uniform>
#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/ParameterOutput>
#include <osg/TemplatePrimitiveFunctor>

#include <simgear/bvh/BVHGroup.hxx>
#include <simgear/bvh/BVHLineGeometry.hxx>

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>

#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/EffectCullVisitor.hxx>
#include <simgear/scene/util/DeletionManager.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/util/SGStateAttributeVisitor.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/SGTransientModelData.hxx>

#include "vg/vgu.h"

#include "animation.hxx"
#include "model.hxx"

#include "SGTranslateTransform.hxx"
#include "SGMaterialAnimation.hxx"
#include "SGRotateTransform.hxx"
#include "SGScaleTransform.hxx"
#include "SGInteractionAnimation.hxx"
#include "SGPickAnimation.hxx"
#include "SGTrackToAnimation.hxx"

#include "ConditionNode.hxx"

using OpenThreads::Mutex;
using OpenThreads::ReentrantMutex;
using OpenThreads::ScopedLock;

using namespace simgear;
////////////////////////////////////////////////////////////////////////
// Static utility functions.
////////////////////////////////////////////////////////////////////////


/*
 * collect line segments from nodes within the hierarchy.
*/
class LineCollector : public osg::NodeVisitor {
    struct LineCollector_LinePrimitiveFunctor {
        LineCollector_LinePrimitiveFunctor() : _lineCollector(0) { }
        void operator() (const osg::Vec3&, bool) { }
        void operator() (const osg::Vec3& v1, const osg::Vec3& v2, bool)
        {
            if (_lineCollector) _lineCollector->addLine(v1, v2);
        }
        void operator() (const osg::Vec3&, const osg::Vec3&, const osg::Vec3&, bool) { }
        void operator() (const osg::Vec3&, const osg::Vec3&, const osg::Vec3&, const osg::Vec3&, bool) { }
        LineCollector* _lineCollector;
    };

public:
    LineCollector() : osg::NodeVisitor(osg::NodeVisitor::NODE_VISITOR, osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) { }

    virtual void apply(osg::Geode& geode)
    {
        osg::TemplatePrimitiveFunctor<LineCollector_LinePrimitiveFunctor> pf;
        pf._lineCollector = this;
        for (unsigned i = 0; i < geode.getNumDrawables(); ++i) {
            geode.getDrawable(i)->accept(pf);
        }
    }

    virtual void apply(osg::Node& node)
    {
        traverse(node);
    }

    virtual void apply(osg::Transform& transform)
    {
        osg::Matrix matrix = _matrix;
        if (transform.computeLocalToWorldMatrix(_matrix, this))
            traverse(transform);
        _matrix = matrix;
    }

    const std::vector<SGLineSegmentf>& getLineSegments() const
    {
        return _lineSegments;
    }

    void addLine(const osg::Vec3& v1, const osg::Vec3& v2)
    {
        // Trick to get the ends in the right order.
        // Use the x axis in the original coordinate system. Choose the
        // most negative x-axis as the one pointing forward
        SGVec3f tv1(toSG(_matrix.preMult(v1)));
        SGVec3f tv2(toSG(_matrix.preMult(v2)));
        if (tv1[0] > tv2[0])
            _lineSegments.push_back(SGLineSegmentf(tv1, tv2));
        else
            _lineSegments.push_back(SGLineSegmentf(tv2, tv1));
    }

    void addBVHElements(osg::Node& node, simgear::BVHLineGeometry::Type type)
    {
        if (_lineSegments.empty())
            return;

        SGSceneUserData* userData;
        userData = SGSceneUserData::getOrCreateSceneUserData(&node);

        simgear::BVHNode* bvNode = userData->getBVHNode();
        if (!bvNode && _lineSegments.size() == 1) {
            simgear::BVHLineGeometry* bvLine;
            bvLine = new simgear::BVHLineGeometry(_lineSegments.front(), type);
            userData->setBVHNode(bvLine);
            return;
        }

        simgear::BVHGroup* group = new simgear::BVHGroup;
        if (bvNode)
            group->addChild(bvNode);

        for (unsigned i = 0; i < _lineSegments.size(); ++i) {
            simgear::BVHLineGeometry* bvLine;
            bvLine = new simgear::BVHLineGeometry(_lineSegments[i], type);
            group->addChild(bvLine);
        }
        userData->setBVHNode(group);
    }

private:
    osg::Matrix _matrix;
    std::vector<SGLineSegmentf> _lineSegments;
};

/**
 * Set up the transform matrix for a translation.
 */
static void
set_translation (osg::Matrix &matrix, double position_m, const SGVec3d &axis)
{
  SGVec3d xyz = axis * position_m;
  matrix.makeIdentity();
  matrix(3, 0) = xyz[0];
  matrix(3, 1) = xyz[1];
  matrix(3, 2) = xyz[2];
}

/**
 * Read an interpolation table from properties.
 */
static SGInterpTable *
read_interpolation_table(const SGPropertyNode* props)
{
  const SGPropertyNode* table_node = props->getNode("interpolation");
  if (!table_node)
    return 0;
  return new SGInterpTable(table_node);
}

static std::string
unit_string(const char* value, const char* unit)
{
  return std::string(value) + unit;
}

class SGPersonalityScaleOffsetExpression : public SGUnaryExpression<double> {
public:
  SGPersonalityScaleOffsetExpression(SGExpression<double>* expr,
                                     SGPropertyNode const* config,
                                     const std::string& scalename,
                                     const std::string& offsetname,
                                     double defScale = 1,
                                     double defOffset = 0) :
    SGUnaryExpression<double>(expr),
    _scale(config, scalename.c_str(), defScale),
    _offset(config, offsetname.c_str(), defOffset)
  { }
  void setScale(double scale)
  { _scale = scale; }
  void setOffset(double offset)
  { _offset = offset; }

  virtual void eval(double& value, const simgear::expression::Binding* b) const
  {
    _offset.shuffle();
    _scale.shuffle();
    value = _offset + _scale*getOperand()->getValue(b);
  }

  virtual bool isConst() const { return false; }

private:
  mutable SGPersonalityParameter<double> _scale;
  mutable SGPersonalityParameter<double> _offset;
};


static SGExpressiond*
read_factor_offset(const SGPropertyNode* configNode, SGExpressiond* expr,
                   const std::string& factor, const std::string& offset)
{
  double factorValue = configNode->getDoubleValue(factor, 1);
  if (factorValue != 1)
    expr = new SGScaleExpression<double>(expr, factorValue);
  double offsetValue = configNode->getDoubleValue(offset, 0);
  if (offsetValue != 0)
    expr = new SGBiasExpression<double>(expr, offsetValue);
  return expr;
}

static SGExpressiond*
read_offset_factor(const SGPropertyNode* configNode, SGExpressiond* expr,
                   const std::string& factor, const std::string& offset)
{
  double offsetValue = configNode->getDoubleValue(offset, 0);
  if (offsetValue != 0)
    expr = new SGBiasExpression<double>(expr, offsetValue);
  double factorValue = configNode->getDoubleValue(factor, 1);
  if (factorValue != 1)
    expr = new SGScaleExpression<double>(expr, factorValue);
  return expr;
}

SGExpressiond*
read_value(const SGPropertyNode* configNode, SGPropertyNode* modelRoot,
           const char* unit, double defMin, double defMax)
{
  const SGPropertyNode * expression = configNode->getNode( "expression" );
  if( expression != NULL )
    return SGReadDoubleExpression( modelRoot, expression->getChild(0) );

  SGExpression<double>* value = 0;

  std::string inputPropertyName = configNode->getStringValue("property", "");
  if (inputPropertyName.empty()) {
    std::string spos = unit_string("starting-position", unit);
    double initPos = configNode->getDoubleValue(spos, 0);
    value = new SGConstExpression<double>(initPos);
  } else {
    SGPropertyNode* inputProperty;
    inputProperty = modelRoot->getNode(inputPropertyName, true);
    value = new SGPropertyExpression<double>(inputProperty);
  }

  SGInterpTable* interpTable = read_interpolation_table(configNode);
  if (interpTable) {
    return new SGInterpTableExpression<double>(value, interpTable);
  } else {
    std::string offset = unit_string("offset", unit);
    std::string min = unit_string("min", unit);
    std::string max = unit_string("max", unit);
    
    if (configNode->getBoolValue("use-personality", false)) {
      value = new SGPersonalityScaleOffsetExpression(value, configNode,
                                                     "factor", offset);
    } else {
      value = read_factor_offset(configNode, value, "factor", offset);
    }
    
    double minClip = configNode->getDoubleValue(min, defMin);
    double maxClip = configNode->getDoubleValue(max, defMax);
    if (minClip > SGMiscd::min(SGLimitsd::min(), -SGLimitsd::max()) ||
        maxClip < SGLimitsd::max())
      value = new SGClipExpression<double>(value, minClip, maxClip);
    
    return value;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////
// Animation installer
////////////////////////////////////////////////////////////////////////

class SGAnimation::RemoveModeVisitor : public SGStateAttributeVisitor {
public:
  RemoveModeVisitor(osg::StateAttribute::GLMode mode) :
    _mode(mode)
  { }
  virtual void apply(osg::StateSet* stateSet)
  {
    if (!stateSet)
      return;
    stateSet->removeMode(_mode);
  }
private:
  osg::StateAttribute::GLMode _mode;
};

class SGAnimation::RemoveAttributeVisitor : public SGStateAttributeVisitor {
public:
  RemoveAttributeVisitor(osg::StateAttribute::Type type) :
    _type(type)
  { }
  virtual void apply(osg::StateSet* stateSet)
  {
    if (!stateSet)
      return;
    while (stateSet->getAttribute(_type)) {
      stateSet->removeAttribute(_type);
    }
  }
private:
  osg::StateAttribute::Type _type;
};

class SGAnimation::RemoveTextureModeVisitor : public SGStateAttributeVisitor {
public:
  RemoveTextureModeVisitor(unsigned unit, osg::StateAttribute::GLMode mode) :
    _unit(unit),
    _mode(mode)
  { }
  virtual void apply(osg::StateSet* stateSet)
  {
    if (!stateSet)
      return;
    stateSet->removeTextureMode(_unit, _mode);
  }
private:
  unsigned _unit;
  osg::StateAttribute::GLMode _mode;
};

class SGAnimation::RemoveTextureAttributeVisitor :
  public SGStateAttributeVisitor {
public:
  RemoveTextureAttributeVisitor(unsigned unit,
                                osg::StateAttribute::Type type) :
    _unit(unit),
    _type(type)
  { }
  virtual void apply(osg::StateSet* stateSet)
  {
    if (!stateSet)
      return;
    while (stateSet->getTextureAttribute(_unit, _type)) {
      stateSet->removeTextureAttribute(_unit, _type);
    }
  }
private:
  unsigned _unit;
  osg::StateAttribute::Type _type;
};

class SGAnimation::BinToInheritVisitor : public SGStateAttributeVisitor {
public:
  virtual void apply(osg::StateSet* stateSet)
  {
    if (!stateSet)
      return;
    stateSet->setRenderBinToInherit();
  }
};

class SGAnimation::DrawableCloneVisitor : public osg::NodeVisitor {
public:
  DrawableCloneVisitor() :
    osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
  {}
  void apply(osg::Geode& geode)
  {
    for (unsigned i = 0 ; i < geode.getNumDrawables(); ++i) {
      osg::CopyOp copyOp(osg::CopyOp::DEEP_COPY_ALL &
                         ~osg::CopyOp::DEEP_COPY_TEXTURES);
      geode.setDrawable(i, copyOp(geode.getDrawable(i)));
    }
  }
};

namespace
{
// Set all drawables to not use display lists. OSG will use
// glDrawArrays instead.
struct DoDrawArraysVisitor : public osg::NodeVisitor {
    DoDrawArraysVisitor() :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {}
    void apply(osg::Geode& geode)
    {
        using namespace osg;
        using namespace std;

        for (int i = 0; i < (int)geode.getNumDrawables(); ++i)
            geode.getDrawable(i)->setUseDisplayList(false);
    }
};
}

SGAnimation::SGAnimation(simgear::SGTransientModelData &modelData) :
  osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
  _modelData(modelData),
  _found(false),
  _configNode(modelData.getConfigNode()),
  _modelRoot(modelData.getModelRoot())
{
  _name = modelData.getConfigNode()->getStringValue("name", "");
  _enableHOT = modelData.getConfigNode()->getBoolValue("enable-hot", true);
  std::vector<SGPropertyNode_ptr> objectNames =
    modelData.getConfigNode()->getChildren("object-name");
  for (unsigned i = 0; i < objectNames.size(); ++i)
    _objectNames.push_back(objectNames[i]->getStringValue());
}

SGAnimation::~SGAnimation()
{
  if (!_found)
  {
      std::list<std::string>::const_iterator i;
      std::string info;
      for (i = _objectNames.begin(); i != _objectNames.end(); ++i)
      {
          if (!info.empty())
              info.append(", ");
          info.append("'");
          info.append(*i);
          info.append("'");
      }
      if (!info.empty())
      {
          SG_LOG(SG_IO, SG_DEV_ALERT, "Could not find at least one of the following"
                  " objects for animation: " << info << " in file: " << _modelData.getPath());
      }
  }
}

bool
SGAnimation::animate(simgear::SGTransientModelData &modelData)
{
  std::string type = modelData.getConfigNode()->getStringValue("type", "none");
  if (type == "alpha-test") {
    SGAlphaTestAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "billboard") {
    SGBillboardAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "blend") {
    SGBlendAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "dist-scale") {
    SGDistScaleAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "flash") {
    SGFlashAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "interaction") {
    SGInteractionAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "material") {
    SGMaterialAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "noshadow") {
    SGShadowAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "pick") {
    SGPickAnimation anim(modelData);
    anim.apply(modelData.getNode());
  } else if (type == "knob") {
    SGKnobAnimation anim(modelData);
    anim.apply(modelData.getNode());
  } else if (type == "slider") {
    SGSliderAnimation anim(modelData);
    anim.apply(modelData.getNode());
  } else if (type == "touch") {
      SGTouchAnimation anim(modelData);
      anim.apply(modelData.getNode());
  } else if (type == "range") {
    SGRangeAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "rotate" || type == "spin") {
    SGRotateAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "scale") {
    SGScaleAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "select") {
    SGSelectAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "shader") {
    SGShaderAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "textranslate" || type == "texrotate" ||
             type == "textrapezoid" || type == "texmultiple") {
    SGTexTransformAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "timed") {
    SGTimedAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "locked-track") {
    SGTrackToAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "translate") {
    SGTranslateAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "light") {
    SGLightAnimation anim(modelData);
    anim.apply(modelData);
  } else if (type == "null" || type == "none" || type.empty()) {
    SGGroupAnimation anim(modelData);
    anim.apply(modelData);
  } else
    return false;

  return true;
}
  
  
void
SGAnimation::apply(osg::Node* node)
{
  // duh what a special case ...
  if (_objectNames.empty()) {
    osg::Group* group = node->asGroup();
    if (group) {
      osg::ref_ptr<osg::Group> animationGroup;
      installInGroup(std::string(), *group, animationGroup);
    }
  } else
    node->accept(*this);
}

void
SGAnimation::install(osg::Node& node)
{
  _found = true;
  if (_enableHOT)
    node.setNodeMask( SG_NODEMASK_TERRAIN_BIT | node.getNodeMask());
  else
    node.setNodeMask(~SG_NODEMASK_TERRAIN_BIT & node.getNodeMask());
}

osg::Group*
SGAnimation::createAnimationGroup(osg::Group& parent)
{
  // default implementation, we do not need a new group
  // for every animation type. Usually animations that just change
  // the StateSet of some parts of the model
  return 0;
}

void
SGAnimation::apply(osg::Group& group)
{
    // the trick is to first traverse the children and then
    // possibly splice in a new group node if required.
    // Else we end up in a recursive loop where we infinitly insert new
    // groups in between
    traverse(group);

    // Note that this algorithm preserves the order of the child objects
    // like they appear in the object-name tags.
    // The timed animations require this
    osg::ref_ptr<osg::Group> animationGroup;
    std::list<std::string>::const_iterator nameIt;
    for (nameIt = _objectNames.begin(); nameIt != _objectNames.end(); ++nameIt)
        installInGroup(*nameIt, group, animationGroup);
}

void
SGAnimation::apply(simgear::SGTransientModelData &modelData)
{
    apply(modelData.getNode());
}

void
SGAnimation::installInGroup(const std::string& name, osg::Group& group,
                            osg::ref_ptr<osg::Group>& animationGroup)
{
  int i = group.getNumChildren() - 1;
  for (; 0 <= i; --i) {
    osg::Node* child = group.getChild(i);

    // Check if this one is already processed
    if (std::find(_installedAnimations.begin(),
                  _installedAnimations.end(), child)
        != _installedAnimations.end())
      continue;

    if (name.empty() || child->getName() == name) {
      // fire the installation of the animation
      install(*child);
      
      // create a group node on demand
      if (!animationGroup.valid()) {
        animationGroup = createAnimationGroup(group);
        // Animation type that does not require a new group,
        // in this case we can stop and look for the next object
        if (animationGroup.valid() && !_name.empty())
          animationGroup->setName(_name);
      }
      if (animationGroup.valid()) {
        animationGroup->addChild(child);
        group.removeChild(i);
      }

      // store that we already have processed this child node
      // We can hit this one twice if an animation references some
      // part of a subtree twice
      _installedAnimations.push_back(child);
    }
  }
}

//------------------------------------------------------------------------------
SGVec3d SGAnimation::readVec3( const SGPropertyNode& cfg,
                               const std::string& name,
                               const std::string& suffix,
                               const SGVec3d& def ) const
{
  SGVec3d vec;
  vec[0] = cfg.getDoubleValue(name + "/x" + suffix, def.x());
  vec[1] = cfg.getDoubleValue(name + "/y" + suffix, def.y());
  vec[2] = cfg.getDoubleValue(name + "/z" + suffix, def.z());
  return vec;
}

//------------------------------------------------------------------------------
SGVec3d SGAnimation::readVec3( const std::string& name,
                               const std::string& suffix,
                               const SGVec3d& def ) const
{
  return readVec3(*_configNode, name, suffix, def);
}
/**
* Visitor to find a group by its name.
*/
class FindGroupVisitor :
    public osg::NodeVisitor
{
public:

    FindGroupVisitor(const std::string& name) :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _name(name),
        _group(0)
    {
        if (name.empty())
            SG_LOG(SG_IO, SG_DEV_WARN, "FindGroupVisitor: empty name provided");
    }

    osg::Group* getGroup() const
    {
        return _group;
    }

    virtual void apply(osg::Group& group)
    {
        if (_name != group.getName())
            return traverse(group);

        if (!_group)
            _group = &group;

        // Different paths can exists for example with a picking animation (pick
        // render group)
        else if (_group != &group)
            SG_LOG
            (
                SG_IO,
                SG_DEV_WARN,
                "FindGroupVisitor: name not unique '" << _name << "'"
            );
    }

protected:

    std::string _name;
    osg::Group *_group;
};

/*
 * If an object is specified in the axis tag it is assumed to be a single line segment with two vertices.
 * This function will take action when axis has an object-name tag and the corresponding object
 * can be found within the hierarchy.
 */
bool SGAnimation::setCenterAndAxisFromObject(osg::Node *rootNode, SGVec3d& center, SGVec3d &axis, simgear::SGTransientModelData &modelData) const
{
    std::string axis_object_name = std::string();
    bool can_warn = true;

    if (_configNode->hasValue("axis/object-name"))
    {
        axis_object_name = _configNode->getStringValue("axis/object-name");
    }
    else if (!_configNode->getNode("axis")) {
        axis_object_name = _configNode->getStringValue("object-name") + std::string("-axis");
        // for compatibility we will not warn if no axis object can be found when there was nothing 
        // specified - as the axis could just be the default at the origin
        // so if there is a [objectname]-axis use it, otherwise fallback to the previous behaviour
        can_warn = false; 
    }

    if (!axis_object_name.empty())
    {
        /*
        * First search the currently loaded cache map to see if this axis object has already been located.
        * If we find it, we use it.
        */
        const SGLineSegment<double> *axisSegment = modelData.getAxisDefinition(axis_object_name);
        if (!axisSegment)
        {
            /*
             * Find the object by name
             */
            FindGroupVisitor axis_object_name_finder(axis_object_name);
            rootNode->accept(axis_object_name_finder);
            osg::Group *object_group = axis_object_name_finder.getGroup();

            if (object_group)
            {
                /*
                 * we have found the object group (for the axis). This should be two vertices
                 * Now process this (with the line collector) to get the vertices.
                 * Once we have that we can then calculate the center and the affected axes.
                 */
                object_group->setNodeMask(0xffffffff);
                LineCollector lineCollector;
                object_group->accept(lineCollector);
                std::vector<SGLineSegmentf> segs = lineCollector.getLineSegments();

                if (!segs.empty())
                {
                    /*
                     * Store the axis definition in the map; as once hidden it will not be possible
                     * to locate it again (and in any case it will be quicker to do it this way)
                     * This makes the axis/center static; there could be a use case for making this
                     * dynamic (and rebuilding the transforms), in which case this would need to
                     * do something different with the object; possibly storing a reference to the node
                     * so it can be extracted for dynamic processing.
                     */
                    SGLineSegmentd segd(*(segs.begin()));
                    axisSegment = modelData.addAxisDefinition(axis_object_name, segd);
                    /*
                     * Hide the axis object. This also helps the modeller to know which axis animations are unassigned.
                     */
                    object_group->setNodeMask(0);
                }
                else {
                    reportFailure(LoadFailure::Misconfigured, ErrorCode::XMLModelLoad,
                                  "Could not find valid line segment for axis animation:" + axis_object_name,
                                  SGPath::fromUtf8(_modelData.getPath()));
                    SG_LOG(SG_IO, SG_DEV_ALERT, "Could not find a valid line segment for animation:  " << axis_object_name << " in file: " << _modelData.getPath());
                }
            }
            else if (can_warn) {
                reportFailure(LoadFailure::Misconfigured, ErrorCode::XMLModelLoad,
                              "Could not find object for axis animation:" + axis_object_name,
                              SGPath::fromUtf8(_modelData.getPath()));
                SG_LOG(SG_IO, SG_DEV_ALERT, "Could not find at least one of the following objects for axis animation: " << axis_object_name << " in file: " << _modelData.getPath());
            }
        }
        if (axisSegment)
        {
            center = 0.5*(axisSegment->getStart() + axisSegment->getEnd());
            axis = axisSegment->getEnd() - axisSegment->getStart();
            return true;
        }
    }
    return false;
}
//------------------------------------------------------------------------------
// factored out to share with SGKnobAnimation
void SGAnimation::readRotationCenterAndAxis(osg::Node *_rootNode, SGVec3d& center,
                                             SGVec3d& axis, simgear::SGTransientModelData &modelData) const
{
  center = SGVec3d::zeros();
  if (setCenterAndAxisFromObject(_rootNode, center, axis, modelData))
  {
      if (8 * SGLimitsd::min() < norm(axis))
          axis = normalize(axis);
      return;
  }

  if( _configNode->hasValue("axis/x1-m") )
  {
    SGVec3d v1 = readVec3("axis", "1-m"), // axis/[xyz]1-m
            v2 = readVec3("axis", "2-m"); // axis/[xyz]2-m
    center = 0.5*(v1+v2);
    axis = v2 - v1;
  }
  else
  {
    axis = readVec3("axis");
  }
  if( 8 * SGLimitsd::min() < norm(axis) )
    axis = normalize(axis);

  center = readVec3("center", "-m", center);
}

//------------------------------------------------------------------------------
SGExpressiond* SGAnimation::readOffsetValue(const char* tag_name) const
{
  const SGPropertyNode* node = _configNode->getChild(tag_name);
  if( !node )
    return 0;

  SGExpressiond_ref expression;
  if( !node->nChildren() )
    expression = new SGConstExpression<double>(node->getDoubleValue());
  else
    expression = SGReadDoubleExpression(_modelRoot, node->getChild(0));

  if( !expression )
    return 0;

  expression = expression->simplify();

  if( expression->isConst() && expression->getValue() == 0 )
    return 0;

  return expression.release();
}

void
SGAnimation::removeMode(osg::Node& node, osg::StateAttribute::GLMode mode)
{
  RemoveModeVisitor visitor(mode);
  node.accept(visitor);
}

void
SGAnimation::removeAttribute(osg::Node& node, osg::StateAttribute::Type type)
{
  RemoveAttributeVisitor visitor(type);
  node.accept(visitor);
}

void
SGAnimation::removeTextureMode(osg::Node& node, unsigned unit,
                               osg::StateAttribute::GLMode mode)
{
  RemoveTextureModeVisitor visitor(unit, mode);
  node.accept(visitor);
}

void
SGAnimation::removeTextureAttribute(osg::Node& node, unsigned unit,
                                    osg::StateAttribute::Type type)
{
  RemoveTextureAttributeVisitor visitor(unit, type);
  node.accept(visitor);
}

void
SGAnimation::setRenderBinToInherit(osg::Node& node)
{
  BinToInheritVisitor visitor;
  node.accept(visitor);
}

void
SGAnimation::cloneDrawables(osg::Node& node)
{
  DrawableCloneVisitor visitor;
  node.accept(visitor);
}

const SGCondition*
SGAnimation::getCondition() const
{
  const SGPropertyNode* conditionNode = _configNode->getChild("condition");
  if (!conditionNode)
    return 0;
  return sgReadCondition(_modelRoot, conditionNode);
}



////////////////////////////////////////////////////////////////////////
// Implementation of null animation
////////////////////////////////////////////////////////////////////////

// Ok, that is to build a subgraph from different other
// graph nodes. I guess that this stems from the time where modellers
// could not build hierarchical trees ...
SGGroupAnimation::SGGroupAnimation(simgear::SGTransientModelData &modelData):
  SGAnimation(modelData)
{
}

osg::Group*
SGGroupAnimation::createAnimationGroup(osg::Group& parent)
{
  osg::Group* group = new osg::Group;
  parent.addChild(group);
  return group;
}


////////////////////////////////////////////////////////////////////////
// Implementation of translate animation
////////////////////////////////////////////////////////////////////////

class SGTranslateAnimation::UpdateCallback : public osg::NodeCallback {
public:
  UpdateCallback(SGCondition const* condition,
                 SGExpressiond const* animationValue) :
    _condition(condition),
    _animationValue(animationValue)
  {
      setName("SGTranslateAnimation::UpdateCallback");
  }
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    if (!_condition || _condition->test()) {
      SGTranslateTransform* transform;
      transform = static_cast<SGTranslateTransform*>(node);
      transform->setValue(_animationValue->getValue());
    }
    traverse(node, nv);
  }
public:
  SGSharedPtr<SGCondition const> _condition;
  SGSharedPtr<SGExpressiond const> _animationValue;
};

SGTranslateAnimation::SGTranslateAnimation(simgear::SGTransientModelData &modelData) :
  SGAnimation(modelData)
{
  _condition = getCondition();
  SGSharedPtr<SGExpressiond> value;
  value = read_value(modelData.getConfigNode(), modelData.getModelRoot(), "-m",
                     -SGLimitsd::max(), SGLimitsd::max());
  _animationValue = value->simplify();
  if (_animationValue)
    _initialValue = _animationValue->getValue();
  else
    _initialValue = 0;

      SGVec3d _center;
  if (modelData.getNode() && !setCenterAndAxisFromObject(modelData.getNode(), _center, _axis, modelData))
    _axis = readTranslateAxis(modelData.getConfigNode());
}

osg::Group*
SGTranslateAnimation::createAnimationGroup(osg::Group& parent)
{
  SGTranslateTransform* transform = new SGTranslateTransform;
  transform->setName("translate animation");
  if (_animationValue && !_animationValue->isConst()) {
    UpdateCallback* uc = new UpdateCallback(_condition, _animationValue);
    transform->setUpdateCallback(uc);
  }
  transform->setAxis(_axis);
  transform->setValue(_initialValue);
  parent.addChild(transform);
  return transform;
}


////////////////////////////////////////////////////////////////////////
// Implementation of rotate/spin animation
////////////////////////////////////////////////////////////////////////

class SGRotAnimTransform : public SGRotateTransform
{
public:
    SGRotAnimTransform();
    SGRotAnimTransform(const SGRotAnimTransform&,
                       const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
    META_Node(simgear, SGRotAnimTransform);
    virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,
                                           osg::NodeVisitor* nv) const;
    virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,
                                           osg::NodeVisitor* nv) const;
    SGSharedPtr<SGCondition const> _condition;
    SGSharedPtr<SGExpressiond const> _animationValue;
    // used when condition is false
    mutable double _lastAngle;
};

SGRotAnimTransform::SGRotAnimTransform()
    : _lastAngle(0.0)
{
}

SGRotAnimTransform::SGRotAnimTransform(const SGRotAnimTransform& rhs,
                                       const osg::CopyOp& copyop)
    : SGRotateTransform(rhs, copyop), _condition(rhs._condition),
      _animationValue(rhs._animationValue), _lastAngle(rhs._lastAngle)
{
}

bool SGRotAnimTransform::computeLocalToWorldMatrix(osg::Matrix& matrix,
                                                   osg::NodeVisitor* nv) const
{
    double angle = 0.0;
    if (!_condition || _condition->test()) {
        angle = _animationValue->getValue();
        _lastAngle = angle;
    } else {
        angle = _lastAngle;
    }
    double angleRad = SGMiscd::deg2rad(angle);
    if (_referenceFrame == RELATIVE_RF) {
        // FIXME optimize
        osg::Matrix tmp;
        set_rotation(tmp, angleRad, getCenter(), getAxis());
        matrix.preMult(tmp);
    } else {
        osg::Matrix tmp;
        SGRotateTransform::set_rotation(tmp, angleRad, getCenter(), getAxis());
        matrix = tmp;
    }
    return true;
}

bool SGRotAnimTransform::computeWorldToLocalMatrix(osg::Matrix& matrix,
                                                   osg::NodeVisitor* nv) const
{
    double angle = 0.0;
    if (!_condition || _condition->test()) {
        angle = _animationValue->getValue();
        _lastAngle = angle;
    } else {
        angle = _lastAngle;
    }
    double angleRad = SGMiscd::deg2rad(angle);
    if (_referenceFrame == RELATIVE_RF) {
        // FIXME optimize
        osg::Matrix tmp;
        set_rotation(tmp, -angleRad, getCenter(), getAxis());
        matrix.postMult(tmp);
    } else {
        osg::Matrix tmp;
        set_rotation(tmp, -angleRad, getCenter(), getAxis());
        matrix = tmp;
    }
    return true;
}

// Cull callback
class SpinAnimCallback : public osg::NodeCallback {
public:
    SpinAnimCallback(SGCondition const* condition,
                       SGExpressiond const* animationValue,
          double initialValue = 0.0) :
    _condition(condition),
    _animationValue(animationValue),
    _initialValue(initialValue)
    {}
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
public:
    SGSharedPtr<SGCondition const> _condition;
    SGSharedPtr<SGExpressiond const> _animationValue;
    double _initialValue;
protected:
    // This cull callback can run in different threads if there is
    // more than one camera. It is probably safe to overwrite the
    // reference values in multiple threads, but we'll provide a
    // threadsafe way to manage those values just to be safe.
    struct ReferenceValues : public osg::Referenced
    {
        ReferenceValues(double t, double rot, double vel)
            : _time(t), _rotation(rot), _rotVelocity(vel)
        {
        }
        double _time;
        double _rotation;
        double _rotVelocity;
    };
    OpenThreads::AtomicPtr _referenceValues;
};

void SpinAnimCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    using namespace osg;
    SGRotateTransform* transform = static_cast<SGRotateTransform*>(node);
    EffectCullVisitor* cv = dynamic_cast<EffectCullVisitor*>(nv);
    if (!cv)
        return;
    if (!_condition || _condition->test()) {
        double t = nv->getFrameStamp()->getSimulationTime();
        double rps = _animationValue->getValue() / 60.0;
        ref_ptr<ReferenceValues>
            refval(static_cast<ReferenceValues*>(_referenceValues.get()));
    if (!refval || refval->_rotVelocity != rps) {
            ref_ptr<ReferenceValues> newref;
            if (!refval.valid()) {
                // initialization
                newref = new ReferenceValues(t, 0.0, rps);
            } else {
                double newRot = refval->_rotation + (t - refval->_time) * refval->_rotVelocity;
                newref = new ReferenceValues(t, newRot, rps);
            }
            // increment reference pointer, because it will be stored
            // naked in _referenceValues.
            newref->ref();
            if (_referenceValues.assign(newref, refval)) {
                if (refval.valid()) {
                    DeletionManager::instance()->addStaleObject(refval.get());
                    refval->unref();
                }
            } else {
                // Another thread installed new values before us
                newref->unref();
            }
            // Whatever happened, we can use the reference values just
            // calculated.
            refval = newref;
        }
        double rotation = refval->_rotation + (t - refval->_time) * rps;
        double intPart;
        double rot = modf(rotation, &intPart);
        double angle = rot * 2.0 * osg::PI;
        transform->setAngleRad(angle);
        traverse(transform, nv);
    } else {
        traverse(transform, nv);
    }
}

SGVec3d readTranslateAxis(const SGPropertyNode* configNode)
{
    SGVec3d axis;
    
    if (configNode->hasValue("axis/x1-m")) {
        SGVec3d v1, v2;
        v1[0] = configNode->getDoubleValue("axis/x1-m", 0);
        v1[1] = configNode->getDoubleValue("axis/y1-m", 0);
        v1[2] = configNode->getDoubleValue("axis/z1-m", 0);
        v2[0] = configNode->getDoubleValue("axis/x2-m", 0);
        v2[1] = configNode->getDoubleValue("axis/y2-m", 0);
        v2[2] = configNode->getDoubleValue("axis/z2-m", 0);
        axis = v2 - v1;
    } else {
        axis[0] = configNode->getDoubleValue("axis/x", 0);
        axis[1] = configNode->getDoubleValue("axis/y", 0);
        axis[2] = configNode->getDoubleValue("axis/z", 0);
    }
    if (8*SGLimitsd::min() < norm(axis))
        axis = normalize(axis);

    return axis;
}

SGRotateAnimation::SGRotateAnimation(simgear::SGTransientModelData &modelData) :
  SGAnimation(modelData)
{
  std::string type = modelData.getConfigNode()->getStringValue("type", "");
  _isSpin = (type == "spin");

  _condition = getCondition();
  SGSharedPtr<SGExpressiond> value;
  value = read_value(modelData.getConfigNode(), modelData.getModelRoot(), "-deg",
                     -SGLimitsd::max(), SGLimitsd::max());
  _animationValue = value->simplify();
  if (_animationValue)
    _initialValue = _animationValue->getValue();
  else
    _initialValue = 0;
  
  readRotationCenterAndAxis(modelData.getNode(), _center, _axis, modelData);
}

osg::Group*
SGRotateAnimation::createAnimationGroup(osg::Group& parent)
{
    if (_isSpin) {
        SGRotateTransform* transform = new SGRotateTransform;
        transform->setName("spin rotate animation");
        SpinAnimCallback* cc;
        cc = new SpinAnimCallback(_condition, _animationValue, _initialValue);
        transform->setCullCallback(cc);
        transform->setCenter(_center);
        transform->setAxis(_axis);
        transform->setAngleDeg(_initialValue);
        parent.addChild(transform);
        return transform;
    } else {
        SGRotAnimTransform* transform = new SGRotAnimTransform;
        transform->setName("rotate animation");
        transform->_condition = _condition;
        transform->_animationValue = _animationValue;
        transform->_lastAngle = _initialValue;
        transform->setCenter(_center);
        transform->setAxis(_axis);
        parent.addChild(transform);
        return transform;
    }
}


////////////////////////////////////////////////////////////////////////
// Implementation of scale animation
////////////////////////////////////////////////////////////////////////

class SGScaleAnimation::UpdateCallback : public osg::NodeCallback {
public:
  UpdateCallback(const SGCondition* condition,
                 SGSharedPtr<const SGExpressiond> animationValue[3]) :
    _condition(condition)
  {
    _animationValue[0] = animationValue[0];
    _animationValue[1] = animationValue[1];
    _animationValue[2] = animationValue[2];
    setName("SGScaleAnimation::UpdateCallback");
  }
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    if (!_condition || _condition->test()) {
      SGScaleTransform* transform;
      transform = static_cast<SGScaleTransform*>(node);
      SGVec3d scale(_animationValue[0]->getValue(),
                    _animationValue[1]->getValue(),
                    _animationValue[2]->getValue());
      transform->setScaleFactor(scale);
    }
    traverse(node, nv);
  }
public:
  SGSharedPtr<SGCondition const> _condition;
  SGSharedPtr<SGExpressiond const> _animationValue[3];
};

SGScaleAnimation::SGScaleAnimation(simgear::SGTransientModelData &modelData) :
  SGAnimation(modelData)
{
  _condition = getCondition();

  // default offset/factor for all directions
  double offset = modelData.getConfigNode()->getDoubleValue("offset", 0);
  double factor = modelData.getConfigNode()->getDoubleValue("factor", 1);

  SGSharedPtr<SGExpressiond> inPropExpr;

  std::string inputPropertyName;
  inputPropertyName = modelData.getConfigNode()->getStringValue("property", "");
  if (inputPropertyName.empty()) {
    inPropExpr = new SGConstExpression<double>(0);
  } else {
    SGPropertyNode* inputProperty;
    inputProperty = modelData.getModelRoot()->getNode(inputPropertyName, true);
    inPropExpr = new SGPropertyExpression<double>(inputProperty);
  }

  SGInterpTable* interpTable = read_interpolation_table(modelData.getConfigNode());
  if (interpTable) {
    SGSharedPtr<SGExpressiond> value;
    value = new SGInterpTableExpression<double>(inPropExpr, interpTable);
    _animationValue[0] = value->simplify();
    _animationValue[1] = value->simplify();
    _animationValue[2] = value->simplify();
  } else if (modelData.getConfigNode()->getBoolValue("use-personality", false)) {
    SGSharedPtr<SGExpressiond> value;
    value = new SGPersonalityScaleOffsetExpression(inPropExpr, modelData.getConfigNode(),
                                                   "x-factor", "x-offset",
                                                   factor, offset);
    double minClip = modelData.getConfigNode()->getDoubleValue("x-min", 0);
    double maxClip = modelData.getConfigNode()->getDoubleValue("x-max", SGLimitsd::max());
    value = new SGClipExpression<double>(value, minClip, maxClip);
    _animationValue[0] = value->simplify();
    
    value = new SGPersonalityScaleOffsetExpression(inPropExpr, modelData.getConfigNode(),
                                                   "y-factor", "y-offset",
                                                   factor, offset);
    minClip = modelData.getConfigNode()->getDoubleValue("y-min", 0);
    maxClip = modelData.getConfigNode()->getDoubleValue("y-max", SGLimitsd::max());
    value = new SGClipExpression<double>(value, minClip, maxClip);
    _animationValue[1] = value->simplify();
    
    value = new SGPersonalityScaleOffsetExpression(inPropExpr, modelData.getConfigNode(),
                                                   "z-factor", "z-offset",
                                                   factor, offset);
    minClip = modelData.getConfigNode()->getDoubleValue("z-min", 0);
    maxClip = modelData.getConfigNode()->getDoubleValue("z-max", SGLimitsd::max());
    value = new SGClipExpression<double>(value, minClip, maxClip);
    _animationValue[2] = value->simplify();
  } else {
    SGSharedPtr<SGExpressiond> value;
    value = read_factor_offset(modelData.getConfigNode(), inPropExpr, "x-factor", "x-offset");
    double minClip = modelData.getConfigNode()->getDoubleValue("x-min", 0);
    double maxClip = modelData.getConfigNode()->getDoubleValue("x-max", SGLimitsd::max());
    value = new SGClipExpression<double>(value, minClip, maxClip);
    _animationValue[0] = value->simplify();

    value = read_factor_offset(modelData.getConfigNode(), inPropExpr, "y-factor", "y-offset");
    minClip = modelData.getConfigNode()->getDoubleValue("y-min", 0);
    maxClip = modelData.getConfigNode()->getDoubleValue("y-max", SGLimitsd::max());
    value = new SGClipExpression<double>(value, minClip, maxClip);
    _animationValue[1] = value->simplify();

    value = read_factor_offset(modelData.getConfigNode(), inPropExpr, "z-factor", "z-offset");
    minClip = modelData.getConfigNode()->getDoubleValue("z-min", 0);
    maxClip = modelData.getConfigNode()->getDoubleValue("z-max", SGLimitsd::max());
    value = new SGClipExpression<double>(value, minClip, maxClip);
    _animationValue[2] = value->simplify();
  }
  _initialValue[0] = modelData.getConfigNode()->getDoubleValue("x-starting-scale", 1);
  _initialValue[0] *= modelData.getConfigNode()->getDoubleValue("x-factor", factor);
  _initialValue[0] += modelData.getConfigNode()->getDoubleValue("x-offset", offset);
  _initialValue[1] = modelData.getConfigNode()->getDoubleValue("y-starting-scale", 1);
  _initialValue[1] *= modelData.getConfigNode()->getDoubleValue("y-factor", factor);
  _initialValue[1] += modelData.getConfigNode()->getDoubleValue("y-offset", offset);
  _initialValue[2] = modelData.getConfigNode()->getDoubleValue("z-starting-scale", 1);
  _initialValue[2] *= modelData.getConfigNode()->getDoubleValue("z-factor", factor);
  _initialValue[2] += modelData.getConfigNode()->getDoubleValue("z-offset", offset);
  _center[0] = modelData.getConfigNode()->getDoubleValue("center/x-m", 0);
  _center[1] = modelData.getConfigNode()->getDoubleValue("center/y-m", 0);
  _center[2] = modelData.getConfigNode()->getDoubleValue("center/z-m", 0);
}

osg::Group*
SGScaleAnimation::createAnimationGroup(osg::Group& parent)
{
  SGScaleTransform* transform = new SGScaleTransform;
  transform->setName("scale animation");
  transform->setCenter(_center);
  transform->setScaleFactor(_initialValue);
  UpdateCallback* uc = new UpdateCallback(_condition, _animationValue);
  transform->setUpdateCallback(uc);
  parent.addChild(transform);
  return transform;
}


// Don't create a new state state everytime we need GL_NORMALIZE!

namespace
{
Mutex normalizeMutex;

osg::StateSet* getNormalizeStateSet()
{
    static osg::ref_ptr<osg::StateSet> normalizeStateSet;
    ScopedLock<Mutex> lock(normalizeMutex);
    if (!normalizeStateSet.valid()) {
        normalizeStateSet = new osg::StateSet;
        normalizeStateSet->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
        normalizeStateSet->setDataVariance(osg::Object::STATIC);
    }
    return normalizeStateSet.get();
}
}

////////////////////////////////////////////////////////////////////////
// Implementation of dist scale animation
////////////////////////////////////////////////////////////////////////

class SGDistScaleAnimation::Transform : public osg::Transform {
public:
  Transform() : _min_v(0.0), _max_v(0.0), _factor(0.0), _offset(0.0) {}
  Transform(const Transform& rhs,
            const osg::CopyOp& copyOp = osg::CopyOp::SHALLOW_COPY)
    : osg::Transform(rhs, copyOp), _table(rhs._table), _center(rhs._center),
      _min_v(rhs._min_v), _max_v(rhs._max_v), _factor(rhs._factor),
      _offset(rhs._offset)
  {
  }
  META_Node(simgear, SGDistScaleAnimation::Transform);
  Transform(const SGPropertyNode* configNode)
  {
    setName(configNode->getStringValue("name", "dist scale animation"));
    setReferenceFrame(RELATIVE_RF);
    setStateSet(getNormalizeStateSet());
    _factor = configNode->getFloatValue("factor", 1);
    _offset = configNode->getFloatValue("offset", 0);
    _min_v = configNode->getFloatValue("min", SGLimitsf::epsilon());
    _max_v = configNode->getFloatValue("max", SGLimitsf::max());
    _table = read_interpolation_table(configNode);
    _center[0] = configNode->getFloatValue("center/x-m", 0);
    _center[1] = configNode->getFloatValue("center/y-m", 0);
    _center[2] = configNode->getFloatValue("center/z-m", 0);
  }
  virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const 
  {
    osg::Matrix transform;
    double scale_factor = computeScaleFactor(nv);
    transform(0,0) = scale_factor;
    transform(1,1) = scale_factor;
    transform(2,2) = scale_factor;
    transform(3,0) = _center[0]*(1 - scale_factor);
    transform(3,1) = _center[1]*(1 - scale_factor);
    transform(3,2) = _center[2]*(1 - scale_factor);
    matrix.preMult(transform);
    return true;
  }
  
  virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const
  {
    double scale_factor = computeScaleFactor(nv);
    if (fabs(scale_factor) <= SGLimits<double>::min())
      return false;
    osg::Matrix transform;
    double rScaleFactor = 1/scale_factor;
    transform(0,0) = rScaleFactor;
    transform(1,1) = rScaleFactor;
    transform(2,2) = rScaleFactor;
    transform(3,0) = _center[0]*(1 - rScaleFactor);
    transform(3,1) = _center[1]*(1 - rScaleFactor);
    transform(3,2) = _center[2]*(1 - rScaleFactor);
    matrix.postMult(transform);
    return true;
  }

  static bool writeLocalData(const osg::Object& obj, osgDB::Output& fw)
  {
    const Transform& trans = static_cast<const Transform&>(obj);
    fw.indent() << "center " << trans._center << "\n";
    fw.indent() << "min_v " << trans._min_v << "\n";
    fw.indent() << "max_v " << trans._max_v << "\n";
    fw.indent() << "factor " << trans._factor << "\n";
    fw.indent() << "offset " << trans._offset << "\n";
    return true;
  }
private:
  double computeScaleFactor(osg::NodeVisitor* nv) const
  {
    if (!nv)
      return 1;

    double scale_factor = (toOsg(_center) - nv->getEyePoint()).length();
    if (_table == 0) {
      scale_factor = _factor * scale_factor + _offset;
    } else {
      scale_factor = _table->interpolate( scale_factor );
    }
    if (scale_factor < _min_v)
      scale_factor = _min_v;
    if (scale_factor > _max_v)
      scale_factor = _max_v;

    return scale_factor;
  }

  SGSharedPtr<SGInterpTable> _table;
  SGVec3d _center;
  double _min_v;
  double _max_v;
  double _factor;
  double _offset;
};


SGDistScaleAnimation::SGDistScaleAnimation(simgear::SGTransientModelData &modelData) :
  SGAnimation(modelData)
{
}

osg::Group*
SGDistScaleAnimation::createAnimationGroup(osg::Group& parent)
{
  Transform* transform = new Transform(getConfig());
  parent.addChild(transform);
  return transform;
}

namespace
{
  osgDB::RegisterDotOsgWrapperProxy distScaleAnimationTransformProxy
  (
   new SGDistScaleAnimation::Transform,
   "SGDistScaleAnimation::Transform",
   "Object Node Transform SGDistScaleAnimation::Transform Group",
   0,
   &SGDistScaleAnimation::Transform::writeLocalData
   );
}

////////////////////////////////////////////////////////////////////////
// Implementation of flash animation
////////////////////////////////////////////////////////////////////////

class SGFlashAnimation::Transform : public osg::Transform {
public:
  Transform() : _power(0.0), _factor(0.0), _offset(0.0), _min_v(0.0),
                _max_v(0.0), _two_sides(false)
  {}

  Transform(const Transform& rhs,
            const osg::CopyOp& copyOp = osg::CopyOp::SHALLOW_COPY)
    : osg::Transform(rhs, copyOp), _center(rhs._center), _axis(rhs._axis),
      _power(rhs._power), _factor(rhs._factor), _offset(rhs._offset),
      _min_v(rhs._min_v), _max_v(rhs._max_v), _two_sides(rhs._two_sides)
  {
  }
  META_Node(simgear, SGFlashAnimation::Transform);

  Transform(const SGPropertyNode* configNode)
  {
    setReferenceFrame(RELATIVE_RF);
    setName(configNode->getStringValue("name", "flash animation"));
    setStateSet(getNormalizeStateSet());

    _axis[0] = configNode->getFloatValue("axis/x", 0);
    _axis[1] = configNode->getFloatValue("axis/y", 0);
    _axis[2] = configNode->getFloatValue("axis/z", 1);
    _axis.normalize();
    
    _center[0] = configNode->getFloatValue("center/x-m", 0);
    _center[1] = configNode->getFloatValue("center/y-m", 0);
    _center[2] = configNode->getFloatValue("center/z-m", 0);
    
    _offset = configNode->getFloatValue("offset", 0);
    _factor = configNode->getFloatValue("factor", 1);
    _power = configNode->getFloatValue("power", 1);
    _two_sides = configNode->getBoolValue("two-sides", false);
    
    _min_v = configNode->getFloatValue("min", SGLimitsf::epsilon());
    _max_v = configNode->getFloatValue("max", 1);
  }
  virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const 
  {
    osg::Matrix transform;
    double scale_factor = computeScaleFactor(nv);
    transform(0,0) = scale_factor;
    transform(1,1) = scale_factor;
    transform(2,2) = scale_factor;
    transform(3,0) = _center[0]*(1 - scale_factor);
    transform(3,1) = _center[1]*(1 - scale_factor);
    transform(3,2) = _center[2]*(1 - scale_factor);
    matrix.preMult(transform);
    return true;
  }
  
  virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const
  {
    double scale_factor = computeScaleFactor(nv);
    if (fabs(scale_factor) <= SGLimits<double>::min())
      return false;
    osg::Matrix transform;
    double rScaleFactor = 1/scale_factor;
    transform(0,0) = rScaleFactor;
    transform(1,1) = rScaleFactor;
    transform(2,2) = rScaleFactor;
    transform(3,0) = _center[0]*(1 - rScaleFactor);
    transform(3,1) = _center[1]*(1 - rScaleFactor);
    transform(3,2) = _center[2]*(1 - rScaleFactor);
    matrix.postMult(transform);
    return true;
  }

  static bool writeLocalData(const osg::Object& obj, osgDB::Output& fw)
  {
    const Transform& trans = static_cast<const Transform&>(obj);
    fw.indent() << "center " << trans._center[0] << " "
                << trans._center[1] << " " << trans._center[2] << " " << "\n";
    fw.indent() << "axis " << trans._axis[0] << " "
                << trans._axis[1] << " " << trans._axis[2] << " " << "\n";
    fw.indent() << "power " << trans._power << " \n";
    fw.indent() << "min_v " << trans._min_v << "\n";
    fw.indent() << "max_v " << trans._max_v << "\n";
    fw.indent() << "factor " << trans._factor << "\n";
    fw.indent() << "offset " << trans._offset << "\n";
    fw.indent() << "twosides " << (trans._two_sides ? "true" : "false") << "\n";
    return true;
  }
private:
  double computeScaleFactor(osg::NodeVisitor* nv) const
  {
    if (!nv)
      return 1;

    osg::Vec3 localEyeToCenter = nv->getEyePoint() - _center;
    localEyeToCenter.normalize();

    double cos_angle = localEyeToCenter*_axis;
    double scale_factor = 0;
    if ( _two_sides && cos_angle < 0 )
      scale_factor = _factor * pow( -cos_angle, _power ) + _offset;
    else if ( cos_angle > 0 )
      scale_factor = _factor * pow( cos_angle, _power ) + _offset;
    
    if ( scale_factor < _min_v )
      scale_factor = _min_v;
    if ( scale_factor > _max_v )
      scale_factor = _max_v;

    return scale_factor;
  }

  virtual osg::BoundingSphere computeBound() const
  {
    // avoid being culled away by small feature culling
    osg::BoundingSphere bs = osg::Group::computeBound();
    bs.radius() *= _max_v;
    return bs;
  }

private:
  osg::Vec3 _center;
  osg::Vec3 _axis;
  double _power, _factor, _offset, _min_v, _max_v;
  bool _two_sides;
};


SGFlashAnimation::SGFlashAnimation(simgear::SGTransientModelData &modelData) :
    SGAnimation(modelData)
{
}

osg::Group*
SGFlashAnimation::createAnimationGroup(osg::Group& parent)
{
  Transform* transform = new Transform(getConfig());
  parent.addChild(transform);
  return transform;
}

namespace
{
  osgDB::RegisterDotOsgWrapperProxy flashAnimationTransformProxy
  (
   new SGFlashAnimation::Transform,
   "SGFlashAnimation::Transform",
   "Object Node Transform SGFlashAnimation::Transform Group",
   0,
   &SGFlashAnimation::Transform::writeLocalData
   );
}

////////////////////////////////////////////////////////////////////////
// Implementation of billboard animation
////////////////////////////////////////////////////////////////////////

class SGBillboardAnimation::Transform : public osg::Transform {
public:
  Transform() : _spherical(true) {}
  Transform(const Transform& rhs,
            const osg::CopyOp& copyOp = osg::CopyOp::SHALLOW_COPY)
    : osg::Transform(rhs, copyOp), _spherical(rhs._spherical) {}
  META_Node(simgear, SGBillboardAnimation::Transform);
  Transform(const SGPropertyNode* configNode) :
    _spherical(configNode->getBoolValue("spherical", true))
  {
    setReferenceFrame(RELATIVE_RF);
    setName(configNode->getStringValue("name", "billboard animation"));
  }
  virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const 
  {
    // More or less taken from plibs ssgCutout
    if (_spherical) {
      matrix(0,0) = 1; matrix(0,1) = 0; matrix(0,2) = 0;
      matrix(1,0) = 0; matrix(1,1) = 0; matrix(1,2) = -1;
      matrix(2,0) = 0; matrix(2,1) = 1; matrix(2,2) = 0;
    } else {
      osg::Vec3 zAxis(matrix(2, 0), matrix(2, 1), matrix(2, 2));
      osg::Vec3 xAxis = osg::Vec3(0, 0, -1)^zAxis;
      osg::Vec3 yAxis = zAxis^xAxis;
      
      xAxis.normalize();
      yAxis.normalize();
      zAxis.normalize();
      
      matrix(0,0) = xAxis[0]; matrix(0,1) = xAxis[1]; matrix(0,2) = xAxis[2];
      matrix(1,0) = yAxis[0]; matrix(1,1) = yAxis[1]; matrix(1,2) = yAxis[2];
      matrix(2,0) = zAxis[0]; matrix(2,1) = zAxis[1]; matrix(2,2) = zAxis[2];
    }
    return true;
  }
  
  virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const
  {
    // Hmm, don't yet know how to get that back ...
    return false;
  }
  static bool writeLocalData(const osg::Object& obj, osgDB::Output& fw)
  {
    const Transform& trans = static_cast<const Transform&>(obj);

    fw.indent() << (trans._spherical ? "true" : "false") << "\n";
    return true;
  }
private:
  bool _spherical;
};


SGBillboardAnimation::SGBillboardAnimation(simgear::SGTransientModelData &modelData) :
    SGAnimation(modelData)
{
}

osg::Group*
SGBillboardAnimation::createAnimationGroup(osg::Group& parent)
{
  Transform* transform = new Transform(getConfig());
  parent.addChild(transform);
  return transform;
}

namespace
{
  osgDB::RegisterDotOsgWrapperProxy billboardAnimationTransformProxy
  (
   new SGBillboardAnimation::Transform,
   "SGBillboardAnimation::Transform",
   "Object Node Transform SGBillboardAnimation::Transform Group",
   0,
   &SGBillboardAnimation::Transform::writeLocalData
   );
}

////////////////////////////////////////////////////////////////////////
// Implementation of a range animation
////////////////////////////////////////////////////////////////////////

class SGRangeAnimation::UpdateCallback : public osg::NodeCallback {
public:
  UpdateCallback(const SGCondition* condition,
                 const SGExpressiond* minAnimationValue,
                 const SGExpressiond* maxAnimationValue,
                 double minValue, double maxValue) :
    _condition(condition),
    _minAnimationValue(minAnimationValue),
    _maxAnimationValue(maxAnimationValue),
    _minStaticValue(minValue),
    _maxStaticValue(maxValue)
  {
      setName("SGRangeAnimation::UpdateCallback");
  }
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    osg::LOD* lod = static_cast<osg::LOD*>(node);
    if (!_condition || _condition->test()) {
      double minRange;
      if (_minAnimationValue)
        minRange = _minAnimationValue->getValue();
      else
        minRange = _minStaticValue;
      double maxRange;
      if (_maxAnimationValue)
        maxRange = _maxAnimationValue->getValue();
      else
        maxRange = _maxStaticValue;
      lod->setRange(0, minRange, maxRange);
    } else {
      lod->setRange(0, 0, SGLimitsf::max());
    }
    traverse(node, nv);
  }

private:
  SGSharedPtr<const SGCondition> _condition;
  SGSharedPtr<const SGExpressiond> _minAnimationValue;
  SGSharedPtr<const SGExpressiond> _maxAnimationValue;
  double _minStaticValue;
  double _maxStaticValue;
};

SGRangeAnimation::SGRangeAnimation(simgear::SGTransientModelData &modelData) :
    SGAnimation(modelData)
{
  _condition = getCondition();

  std::string inputPropertyName;
  inputPropertyName = modelData.getConfigNode()->getStringValue("min-property", "");
  if (!inputPropertyName.empty()) {
    SGPropertyNode* inputProperty;
    inputProperty = modelData.getModelRoot()->getNode(inputPropertyName, true);
    SGSharedPtr<SGExpressiond> value;
    value = new SGPropertyExpression<double>(inputProperty);

    value = read_factor_offset(modelData.getConfigNode(), value, "min-factor", "min-offset");
    _minAnimationValue = value->simplify();
  }
  inputPropertyName = modelData.getConfigNode()->getStringValue("max-property", "");
  if (!inputPropertyName.empty()) {
    SGPropertyNode* inputProperty;
    inputProperty = modelData.getModelRoot()->getNode(inputPropertyName.c_str(), true);

    SGSharedPtr<SGExpressiond> value;
    value = new SGPropertyExpression<double>(inputProperty);

    value = read_factor_offset(modelData.getConfigNode(), value, "max-factor", "max-offset");
    _maxAnimationValue = value->simplify();
  }

  _initialValue[0] = modelData.getConfigNode()->getDoubleValue("min-m", 0);
  _initialValue[0] *= modelData.getConfigNode()->getDoubleValue("min-factor", 1);
  _initialValue[1] = modelData.getConfigNode()->getDoubleValue("max-m", SGLimitsf::max());
  _initialValue[1] *= modelData.getConfigNode()->getDoubleValue("max-factor", 1);
}

osg::Group*
SGRangeAnimation::createAnimationGroup(osg::Group& parent)
{
  osg::Group* group = new osg::Group;
  group->setName("range animation group");

  osg::LOD* lod = new osg::LOD;
  lod->setName("range animation node");
  parent.addChild(lod);

  lod->addChild(group, _initialValue[0], _initialValue[1]);
  lod->setCenterMode(osg::LOD::USE_BOUNDING_SPHERE_CENTER);
  lod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
  if (_minAnimationValue || _maxAnimationValue || _condition) {
    UpdateCallback* uc;
    uc = new UpdateCallback(_condition, _minAnimationValue, _maxAnimationValue,
                            _initialValue[0], _initialValue[1]);
    lod->setUpdateCallback(uc);
  }
  return group;
}


////////////////////////////////////////////////////////////////////////
// Implementation of a select animation
////////////////////////////////////////////////////////////////////////

SGSelectAnimation::SGSelectAnimation(simgear::SGTransientModelData &modelData) :
    SGAnimation(modelData)
{
}

osg::Group*
SGSelectAnimation::createAnimationGroup(osg::Group& parent)
{
  // if no condition given, this is a noop.
  SGSharedPtr<SGCondition const> condition = getCondition();
  // trick, gets deleted with all its 'animated' children
  // when the animation installer returns
  if (!condition)
    return new osg::Group;
  simgear::ConditionNode* cn = new simgear::ConditionNode;
  cn->setName("select animation node");
  cn->setCondition(condition.ptr());
  osg::Group* grp = new osg::Group;
  cn->addChild(grp);
  parent.addChild(cn);
  return grp;
}



////////////////////////////////////////////////////////////////////////
// Implementation of alpha test animation
////////////////////////////////////////////////////////////////////////

SGAlphaTestAnimation::SGAlphaTestAnimation(simgear::SGTransientModelData &modelData) :
    SGAnimation(modelData)
{
}

namespace
{
// Keep one copy of the most common alpha test its state set.
ReentrantMutex alphaTestMutex;
osg::ref_ptr<osg::AlphaFunc> standardAlphaFunc;
osg::ref_ptr<osg::StateSet> alphaFuncStateSet;

osg::AlphaFunc* makeAlphaFunc(float clamp)
{
    ScopedLock<ReentrantMutex> lock(alphaTestMutex);
    if (osg::equivalent(clamp, 0.01f)) {
        if (standardAlphaFunc.valid())
            return standardAlphaFunc.get();
        clamp = .01;
    }
    osg::AlphaFunc* alphaFunc = new osg::AlphaFunc;
    alphaFunc->setFunction(osg::AlphaFunc::GREATER);
    alphaFunc->setReferenceValue(clamp);
    alphaFunc->setDataVariance(osg::Object::STATIC);
    if (osg::equivalent(clamp, 0.01f))
        standardAlphaFunc = alphaFunc;
    return alphaFunc;
}

osg::StateSet* makeAlphaTestStateSet(float clamp)
{
    using namespace OpenThreads;
    ScopedLock<ReentrantMutex> lock(alphaTestMutex);
    if (osg::equivalent(clamp, 0.01f)) {
        if (alphaFuncStateSet.valid())
            return alphaFuncStateSet.get();
    }
    osg::AlphaFunc* alphaFunc = makeAlphaFunc(clamp);
    osg::StateSet* stateSet = new osg::StateSet;
    stateSet->setAttributeAndModes(alphaFunc,
                                   (osg::StateAttribute::ON
                                    | osg::StateAttribute::OVERRIDE));
    stateSet->setDataVariance(osg::Object::STATIC);
    if (osg::equivalent(clamp, 0.01f))
        alphaFuncStateSet = stateSet;
    return stateSet;
}
}
void
SGAlphaTestAnimation::install(osg::Node& node)
{
  SGAnimation::install(node);

  float alphaClamp = getConfig()->getFloatValue("alpha-factor", 0);
  osg::StateSet* stateSet = node.getStateSet();
  if (!stateSet) {
      node.setStateSet(makeAlphaTestStateSet(alphaClamp));
  } else {
      stateSet->setAttributeAndModes(makeAlphaFunc(alphaClamp),
                                     (osg::StateAttribute::ON
                                      | osg::StateAttribute::OVERRIDE));
  }
}


//////////////////////////////////////////////////////////////////////
// Blend animation installer
//////////////////////////////////////////////////////////////////////

// XXX This needs to be replaced by something using TexEnvCombine to
// change the blend factor. Changing the alpha values in the geometry
// is bogus.
class SGBlendAnimation::BlendVisitor : public osg::NodeVisitor {
public:
  BlendVisitor(float blend) :
    osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
    _blend(blend)
  { setVisitorType(osg::NodeVisitor::NODE_VISITOR); }
  virtual void apply(osg::Node& node)
  {
    updateStateSet(node.getStateSet());
    traverse(node);
  }
  virtual void apply(osg::Geode& node)
  {
    apply((osg::Node&)node);
    unsigned nDrawables = node.getNumDrawables();
    for (unsigned i = 0; i < nDrawables; ++i) {
      osg::Drawable* drawable = node.getDrawable(i);
      osg::Geometry* geometry = drawable->asGeometry();
      if (!geometry)
        continue;
      osg::Array* array = geometry->getColorArray();
      if (!array)
        continue;
      osg::Vec4Array* vec4Array = dynamic_cast<osg::Vec4Array*>(array);
      if (!vec4Array)
        continue;
      for (unsigned k = 0; k < vec4Array->size(); ++k) {
        (*vec4Array)[k][3] = _blend;
      }
      vec4Array->dirty();
      updateStateSet(drawable->getStateSet());
    }
  }
  void updateStateSet(osg::StateSet* stateSet)
  {
    if (!stateSet)
      return;
    osg::StateAttribute* stateAttribute;
    stateAttribute = stateSet->getAttribute(osg::StateAttribute::MATERIAL);
    if (!stateAttribute)
      return;
    osg::Material* material = dynamic_cast<osg::Material*>(stateAttribute);
    if (!material)
      return;
    material->setAlpha(osg::Material::FRONT_AND_BACK, _blend);
    if (_blend < 1) {
      stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
      stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    } else {
      stateSet->setRenderingHint(osg::StateSet::DEFAULT_BIN);
    }
  }
private:
  float _blend;
};

class SGBlendAnimation::UpdateCallback : public osg::NodeCallback {
public:
  UpdateCallback(const SGPropertyNode* configNode, const SGExpressiond* v) :
    _prev_value(-1),
    _animationValue(v)
  {
      setName("SGBlendAnimation::UpdateCallback");
  }
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    double blend = _animationValue->getValue();
    if (blend != _prev_value) {
      _prev_value = blend;
      BlendVisitor visitor(1-blend);
      node->accept(visitor);
    }
    traverse(node, nv);
  }
public:
  double _prev_value;
  SGSharedPtr<SGExpressiond const> _animationValue;
};


SGBlendAnimation::SGBlendAnimation(simgear::SGTransientModelData &modelData) :
    SGAnimation(modelData), _animationValue(read_value(modelData.getConfigNode(), modelData.getModelRoot(), "", 0, 1))
{
}

osg::Group*
SGBlendAnimation::createAnimationGroup(osg::Group& parent)
{
  if (!_animationValue)
    return 0;

  osg::Group* group = new osg::Switch;
  group->setName("blend animation node");
  group->setUpdateCallback(new UpdateCallback(getConfig(), _animationValue));
  parent.addChild(group);
  return group;
}

void
SGBlendAnimation::install(osg::Node& node)
{
  SGAnimation::install(node);
  // make sure we do not change common geometries,
  // that also creates new display lists for these subgeometries.
  cloneDrawables(node);
  DoDrawArraysVisitor visitor;
  node.accept(visitor);
}


//////////////////////////////////////////////////////////////////////
// Timed animation installer
//////////////////////////////////////////////////////////////////////



class SGTimedAnimation::UpdateCallback : public osg::NodeCallback {
public:
  UpdateCallback(const SGPropertyNode* configNode) :
    _current_index(0),
    _reminder(0),
    _duration_sec(configNode->getDoubleValue("duration-sec", 1)),
    _last_time_sec(SGLimitsd::max()),
    _use_personality(configNode->getBoolValue("use-personality", false))
  {
    std::vector<SGSharedPtr<SGPropertyNode> > nodes;
    nodes = configNode->getChildren("branch-duration-sec");
    for (size_t i = 0; i < nodes.size(); ++i) {
      unsigned ind = nodes[ i ]->getIndex();
      while ( ind >= _durations.size() ) {
        _durations.push_back(DurationSpec(_duration_sec));
      }
      SGPropertyNode_ptr rNode = nodes[i]->getChild("random");
      if ( rNode == 0 ) {
        _durations[ind] = DurationSpec(nodes[ i ]->getDoubleValue());
      } else {
        _durations[ind] = DurationSpec(rNode->getDoubleValue( "min", 0),
                                       rNode->getDoubleValue( "max", 1));
      }
    }
    setName("SGTimedAnimation::UpdateCallback");
  }
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<osg::Switch*>(node));
    osg::Switch* sw = static_cast<osg::Switch*>(node);

    unsigned nChildren = sw->getNumChildren();

    // blow up the durations vector to the required size
    while (_durations.size() < nChildren) {
      _durations.push_back(_duration_sec);
    }
    // make sure the current index is an duration that really exists
    _current_index = _current_index % nChildren;

    // update the time and compute the current systems time value
    double t = nv->getFrameStamp()->getSimulationTime();
    if (_last_time_sec == SGLimitsd::max()) {
      _last_time_sec = t;
    } else {
      double dt = t - _last_time_sec;
      if (_use_personality)
        dt *= 1 + 0.2*(0.5 - sg_random());
      _reminder += dt;
      _last_time_sec = t;
    }

    double currentDuration = _durations[_current_index].get();
    while (currentDuration < _reminder) {
      _reminder -= currentDuration;
      _current_index = (_current_index + 1) % nChildren;
      currentDuration = _durations[_current_index].get();
    }

    sw->setSingleChildOn(_current_index);

    traverse(node, nv);
  }

private:
  struct DurationSpec {
    DurationSpec(double t) :
      minTime(SGMiscd::max(0.01, t)),
      maxTime(SGMiscd::max(0.01, t))
    {}
    DurationSpec(double t0, double t1) :
      minTime(SGMiscd::max(0.01, t0)),
      maxTime(SGMiscd::max(0.01, t1))
    {}
    double get() const
    { return minTime + sg_random()*(maxTime - minTime); }
    double minTime;
    double maxTime;
  };
  std::vector<DurationSpec> _durations;
  unsigned _current_index;
  double _reminder;
  double _duration_sec;
  double _last_time_sec;
  bool _use_personality;
};


SGTimedAnimation::SGTimedAnimation(simgear::SGTransientModelData &modelData) :
    SGAnimation(modelData)
{
}

osg::Group*
SGTimedAnimation::createAnimationGroup(osg::Group& parent)
{
  osg::Switch* sw = new osg::Switch;
  sw->setName("timed animation node");
  sw->setUpdateCallback(new UpdateCallback(getConfig()));
  parent.addChild(sw);
  return sw;
}


////////////////////////////////////////////////////////////////////////
// dynamically switch on/off shadows
////////////////////////////////////////////////////////////////////////

class SGShadowAnimation::UpdateCallback : public osg::NodeCallback {
public:
  UpdateCallback(const SGCondition* condition) :
    _condition(condition)
  {
      setName("SGShadowAnimation::UpdateCallback");
  }
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    if (_condition->test())
      node->setNodeMask( SG_NODEMASK_CASTSHADOW_BIT | node->getNodeMask());
    else
      node->setNodeMask(~SG_NODEMASK_CASTSHADOW_BIT & node->getNodeMask());
    traverse(node, nv);
  }

private:
  SGSharedPtr<const SGCondition> _condition;
};

SGShadowAnimation::SGShadowAnimation(simgear::SGTransientModelData &modelData) :
    SGAnimation(modelData)
{
}

osg::Group*
SGShadowAnimation::createAnimationGroup(osg::Group& parent)
{
  SGSharedPtr<SGCondition const> condition = getCondition();

  osg::Group* group = new osg::Group;
  group->setName("shadow animation");
  if (condition)
    group->setUpdateCallback(new UpdateCallback(condition));
  else
    group->setNodeMask(~SG_NODEMASK_CASTSHADOW_BIT & group->getNodeMask());
  parent.addChild(group);
  return group;
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGTexTransformAnimation
////////////////////////////////////////////////////////////////////////

class SGTexTransformAnimation::Transform : public SGReferenced {
public:
  Transform() :
    _value(0)
  {}
  virtual ~Transform()
  { }
  void setValue(double value)
  { _value = value; }
  virtual void transform(osg::Matrix&) = 0;
protected:
  double _value;
};

class SGTexTransformAnimation::Translation :
  public SGTexTransformAnimation::Transform {
public:
  Translation(const SGVec3d& axis) :
    _axis(axis)
  { }
  virtual void transform(osg::Matrix& matrix)
  {
    osg::Matrix tmp;
    set_translation(tmp, _value, _axis);
    matrix.preMult(tmp);
  }
private:
  SGVec3d _axis;
};

class SGTexTransformAnimation::Rotation :
  public SGTexTransformAnimation::Transform {
public:
  Rotation(const SGVec3d& axis, const SGVec3d& center) :
    _axis(axis),
    _center(center)
  { }
  virtual void transform(osg::Matrix& matrix)
  {
    osg::Matrix tmp;
    SGRotateTransform::set_rotation(tmp, SGMiscd::deg2rad(_value), _center,
                                    _axis);
    matrix.preMult(tmp);
  }
private:
  SGVec3d _axis;
  SGVec3d _center;
};

class SGTexTransformAnimation::Trapezoid :
  public SGTexTransformAnimation::Transform {
public:

  enum Side { TOP, RIGHT, BOTTOM, LEFT };

  Trapezoid(Side side):
    _side(side)
  { }
  virtual void transform(osg::Matrix& matrix)
  {
    VGfloat sx0 = 0, sy0 = 0,
            sx1 = 1, sy1 = 0,
            sx2 = 0, sy2 = 1,
            sx3 = 1, sy3 = 1;
    switch( _side )
    {
      case TOP:
        sx0 -= _value;
        sx1 += _value;
        break;
      case RIGHT:
        sy1 -= _value;
        sy3 += _value;
        break;
      case BOTTOM:
        sx2 -= _value;
        sx3 += _value;
        break;
      case LEFT:
        sy0 -= _value;
        sy2 += _value;
        break;
    }
    VGfloat mat[3][3];
    VGUErrorCode err = vguComputeWarpQuadToSquare( sx0, sy0,
                                                   sx1, sy1,
                                                   sx2, sy2,
                                                   sx3, sy3,
                                                   (VGfloat*)mat );
    if( err != VGU_NO_ERROR )
      return;

    matrix.preMult( osg::Matrix(
      mat[0][0], mat[0][1], 0, mat[0][2],
      mat[1][0], mat[1][1], 0, mat[1][2],
              0,         0, 1,         0,
      mat[2][0], mat[2][1], 0, mat[2][2]
    ));
  }

protected:
  Side _side;
};

class SGTexTransformAnimation::UpdateCallback :
  public osg::StateAttribute::Callback {
public:
  UpdateCallback(const SGCondition* condition) :
    _condition(condition)
  {
      setName("SGTexTransformAnimation::UpdateCallback");
  }
  virtual void operator () (osg::StateAttribute* sa, osg::NodeVisitor*)
  {
    if (!_condition || _condition->test()) {
      TransformList::const_iterator i;
      for (i = _transforms.begin(); i != _transforms.end(); ++i)
        i->transform->setValue(i->value->getValue());
    }
    assert(dynamic_cast<osg::TexMat*>(sa));
    osg::TexMat* texMat = static_cast<osg::TexMat*>(sa);
    texMat->getMatrix().makeIdentity();
    TransformList::const_iterator i;
    for (i = _transforms.begin(); i != _transforms.end(); ++i)
      i->transform->transform(texMat->getMatrix());
  }
  void appendTransform(Transform* transform, SGExpressiond* value)
  {
    Entry entry = { transform, value };
    transform->transform(_matrix);
    _transforms.push_back(entry);
  }

private:
  struct Entry {
    SGSharedPtr<Transform> transform;
    SGSharedPtr<const SGExpressiond> value;
  };
  typedef std::vector<Entry> TransformList;
  TransformList _transforms;
  SGSharedPtr<const SGCondition> _condition;
  osg::Matrix _matrix;
};

SGTexTransformAnimation::SGTexTransformAnimation(simgear::SGTransientModelData &modelData) :
    SGAnimation(modelData)
{
}

osg::Group*
SGTexTransformAnimation::createAnimationGroup(osg::Group& parent)
{
  osg::Group* group = new osg::Group;
  group->setName("texture transform group");
  osg::StateSet* stateSet = group->getOrCreateStateSet();
  stateSet->setDataVariance(osg::Object::STATIC/*osg::Object::DYNAMIC*/);  
  osg::TexMat* texMat = new osg::TexMat;
  UpdateCallback* updateCallback = new UpdateCallback(getCondition());
  // interpret the configs ...
  std::string type = getType();

  if (type == "textranslate") {
    appendTexTranslate(*getConfig(), updateCallback);
  } else if (type == "texrotate") {
    appendTexRotate(*getConfig(), updateCallback);
  } else if (type == "textrapezoid") {
    appendTexTrapezoid(*getConfig(), updateCallback);
  } else if (type == "texmultiple") {
    std::vector<SGSharedPtr<SGPropertyNode> > transformConfigs;
    transformConfigs = getConfig()->getChildren("transform");
    for (unsigned i = 0; i < transformConfigs.size(); ++i) {
      std::string subtype = transformConfigs[i]->getStringValue("subtype", "");
      if (subtype == "textranslate")
        appendTexTranslate(*transformConfigs[i], updateCallback);
      else if (subtype == "texrotate")
        appendTexRotate(*transformConfigs[i], updateCallback);
      else if (subtype == "textrapezoid")
        appendTexTrapezoid(*transformConfigs[i], updateCallback);
      else {
        SG_LOG(SG_IO, SG_DEV_ALERT,
               "Ignoring unknown texture transform subtype in file: " << _modelData.getPath());
      }
    }
  } else {
    SG_LOG(SG_IO, SG_DEV_ALERT, "Ignoring unknown texture transform type in file: " << _modelData.getPath());
  }

  texMat->setUpdateCallback(updateCallback);
  stateSet->setTextureAttribute(0, texMat);
  parent.addChild(group);
  return group;
}

SGExpressiond*
SGTexTransformAnimation::readValue( const SGPropertyNode& cfg,
                                    const std::string& suffix )
{
  std::string prop_name = cfg.getStringValue("property");
  SGSharedPtr<SGExpressiond> value;
  if( prop_name.empty() )
    value = new SGConstExpression<double>(0);
  else
    value = new SGPropertyExpression<double>
    (
      getModelRoot()->getNode(prop_name, true)
    );

  SGInterpTable* table = read_interpolation_table(&cfg);
  if( table )
  {
    value = new SGInterpTableExpression<double>(value, table);
    double biasValue = cfg.getDoubleValue("bias", 0);
    if( biasValue )
      value = new SGBiasExpression<double>(value, biasValue);
    value = new SGStepExpression<double>( value,
                                          cfg.getDoubleValue("step", 0),
                                          cfg.getDoubleValue("scroll", 0) );
  }
  else
  {
    double biasValue = cfg.getDoubleValue("bias", 0);
    if( biasValue )
      value = new SGBiasExpression<double>(value, biasValue);
    value = new SGStepExpression<double>(value,
                                         cfg.getDoubleValue("step", 0),
                                         cfg.getDoubleValue("scroll", 0));
    value = read_offset_factor(&cfg, value, "factor", "offset" + suffix);

    if(    cfg.hasChild("min" + suffix)
        || cfg.hasChild("max" + suffix) )
    {
      double minClip = cfg.getDoubleValue("min" + suffix, -SGLimitsd::max());
      double maxClip = cfg.getDoubleValue("max" + suffix, SGLimitsd::max());
      value = new SGClipExpression<double>(value, minClip, maxClip);
    }
  }

  return value.release()->simplify();
}

void
SGTexTransformAnimation::appendTexTranslate( const SGPropertyNode& cfg,
                                             UpdateCallback* updateCallback )
{
  Translation* translation = new Translation(normalize(readVec3(cfg, "axis")));
  translation->setValue(cfg.getDoubleValue("starting-position", 0));
  updateCallback->appendTransform(translation, readValue(cfg));
}

void
SGTexTransformAnimation::appendTexRotate( const SGPropertyNode& cfg,
                                          UpdateCallback* updateCallback )
{
  Rotation* rotation = new Rotation( normalize(readVec3(cfg, "axis")),
                                     readVec3(cfg, "center") );
  rotation->setValue(cfg.getDoubleValue("starting-position-deg", 0));
  updateCallback->appendTransform(rotation, readValue(cfg, "-deg"));
}

void
SGTexTransformAnimation::appendTexTrapezoid( const SGPropertyNode& cfg,
                                             UpdateCallback* updateCallback )
{
  Trapezoid::Side side = Trapezoid::TOP;
  const std::string side_str = cfg.getStringValue("side");
  if( side_str == "right" )
    side = Trapezoid::RIGHT;
  else if( side_str == "bottom" )
    side = Trapezoid::BOTTOM;
  else if( side_str == "left" )
    side = Trapezoid::LEFT;

  Trapezoid* trapezoid = new Trapezoid(side);
  trapezoid->setValue(cfg.getDoubleValue("starting-position", 0));
  updateCallback->appendTransform(trapezoid, readValue(cfg));
}
