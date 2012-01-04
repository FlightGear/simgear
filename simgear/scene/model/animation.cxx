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
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
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


#include <simgear/math/interpolater.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/SGBinding.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/util/SGStateAttributeVisitor.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>

#include "animation.hxx"
#include "model.hxx"

#include "SGTranslateTransform.hxx"
#include "SGMaterialAnimation.hxx"
#include "SGRotateTransform.hxx"
#include "SGScaleTransform.hxx"
#include "SGInteractionAnimation.hxx"

#include "ConditionNode.hxx"

using OpenThreads::Mutex;
using OpenThreads::ReentrantMutex;
using OpenThreads::ScopedLock;

using namespace simgear;

////////////////////////////////////////////////////////////////////////
// Static utility functions.
////////////////////////////////////////////////////////////////////////

/**
 * Set up the transform matrix for a spin or rotation.
 */
static void
set_rotation (osg::Matrix &matrix, double position_deg,
              const SGVec3d &center, const SGVec3d &axis)
{
  double temp_angle = -SGMiscd::deg2rad(position_deg);
  
  double s = sin(temp_angle);
  double c = cos(temp_angle);
  double t = 1 - c;
  
  // axis was normalized at load time 
  // hint to the compiler to put these into FP registers
  double x = axis[0];
  double y = axis[1];
  double z = axis[2];
  
  matrix(0, 0) = t * x * x + c ;
  matrix(0, 1) = t * y * x - s * z ;
  matrix(0, 2) = t * z * x + s * y ;
  matrix(0, 3) = 0;
  
  matrix(1, 0) = t * x * y + s * z ;
  matrix(1, 1) = t * y * y + c ;
  matrix(1, 2) = t * z * y - s * x ;
  matrix(1, 3) = 0;
  
  matrix(2, 0) = t * x * z - s * y ;
  matrix(2, 1) = t * y * z + s * x ;
  matrix(2, 2) = t * z * z + c ;
  matrix(2, 3) = 0;
  
  // hint to the compiler to put these into FP registers
  x = center[0];
  y = center[1];
  z = center[2];
  
  matrix(3, 0) = x - x*matrix(0, 0) - y*matrix(1, 0) - z*matrix(2, 0);
  matrix(3, 1) = y - x*matrix(0, 1) - y*matrix(1, 1) - z*matrix(2, 1);
  matrix(3, 2) = z - x*matrix(0, 2) - y*matrix(1, 2) - z*matrix(2, 2);
  matrix(3, 3) = 1;
}

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

SGAnimation::SGAnimation(const SGPropertyNode* configNode,
                                           SGPropertyNode* modelRoot) :
  osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
  _found(false),
  _configNode(configNode),
  _modelRoot(modelRoot)
{
  _name = configNode->getStringValue("name", "");
  _enableHOT = configNode->getBoolValue("enable-hot", true);
  _disableShadow = configNode->getBoolValue("disable-shadow", false);
  std::vector<SGPropertyNode_ptr> objectNames =
    configNode->getChildren("object-name");
  for (unsigned i = 0; i < objectNames.size(); ++i)
    _objectNames.push_back(objectNames[i]->getStringValue());
}

SGAnimation::~SGAnimation()
{
  if (!_found)
  {
      std::list<std::string>::const_iterator i;
      string info;
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
          SG_LOG(SG_IO, SG_ALERT, "Could not find at least one of the following"
                  " objects for animation: " << info);
      }
  }
}

bool
SGAnimation::animate(osg::Node* node, const SGPropertyNode* configNode,
                     SGPropertyNode* modelRoot,
                     const osgDB::Options* options,
                     const string &path, int i)
{
  std::string type = configNode->getStringValue("type", "none");
  if (type == "alpha-test") {
    SGAlphaTestAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "billboard") {
    SGBillboardAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "blend") {
    SGBlendAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "dist-scale") {
    SGDistScaleAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "flash") {
    SGFlashAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "interaction") {
    SGInteractionAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "material") {
    SGMaterialAnimation animInst(configNode, modelRoot, options);
    animInst.apply(node);
  } else if (type == "noshadow") {
    SGShadowAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "pick") {
    SGPickAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "range") {
    SGRangeAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "rotate" || type == "spin") {
    SGRotateAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "scale") {
    SGScaleAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "select") {
    SGSelectAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "shader") {
    SGShaderAnimation animInst(configNode, modelRoot, options);
    animInst.apply(node);
  } else if (type == "textranslate" || type == "texrotate" ||
             type == "texmultiple") {
    SGTexTransformAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "timed") {
    SGTimedAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "translate") {
    SGTranslateAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
  } else if (type == "light") {
    SGLightAnimation animInst(configNode, modelRoot, path, i);
    animInst.apply(node);
  } else if (type == "null" || type == "none" || type.empty()) {
    SGGroupAnimation animInst(configNode, modelRoot);
    animInst.apply(node);
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
  if (!_disableShadow)
    node.setNodeMask( SG_NODEMASK_CASTSHADOW_BIT | node.getNodeMask());
  else
    node.setNodeMask(~SG_NODEMASK_CASTSHADOW_BIT & node.getNodeMask());
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
SGGroupAnimation::SGGroupAnimation(const SGPropertyNode* configNode,
                                   SGPropertyNode* modelRoot):
  SGAnimation(configNode, modelRoot)
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
  { }
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

SGTranslateAnimation::SGTranslateAnimation(const SGPropertyNode* configNode,
                                           SGPropertyNode* modelRoot) :
  SGAnimation(configNode, modelRoot)
{
  _condition = getCondition();
  SGSharedPtr<SGExpressiond> value;
  value = read_value(configNode, modelRoot, "-m",
                     -SGLimitsd::max(), SGLimitsd::max());
  _animationValue = value->simplify();
  if (_animationValue)
    _initialValue = _animationValue->getValue();
  else
    _initialValue = 0;

  if (configNode->hasValue("axis/x1-m")) {
    SGVec3d v1, v2;
    v1[0] = configNode->getDoubleValue("axis/x1-m", 0);
    v1[1] = configNode->getDoubleValue("axis/y1-m", 0);
    v1[2] = configNode->getDoubleValue("axis/z1-m", 0);
    v2[0] = configNode->getDoubleValue("axis/x2-m", 0);
    v2[1] = configNode->getDoubleValue("axis/y2-m", 0);
    v2[2] = configNode->getDoubleValue("axis/z2-m", 0);
    _axis = v2 - v1;
  } else {
    _axis[0] = configNode->getDoubleValue("axis/x", 0);
    _axis[1] = configNode->getDoubleValue("axis/y", 0);
    _axis[2] = configNode->getDoubleValue("axis/z", 0);
  }
  if (8*SGLimitsd::min() < norm(_axis))
    _axis = normalize(_axis);
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

class SGRotateAnimation::UpdateCallback : public osg::NodeCallback {
public:
  UpdateCallback(SGCondition const* condition,
                 SGExpressiond const* animationValue) :
    _condition(condition),
    _animationValue(animationValue)
  { }
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    if (!_condition || _condition->test()) {
      SGRotateTransform* transform;
      transform = static_cast<SGRotateTransform*>(node);
      transform->setAngleDeg(_animationValue->getValue());
    }
    traverse(node, nv);
  }
public:
  SGSharedPtr<SGCondition const> _condition;
  SGSharedPtr<SGExpressiond const> _animationValue;
};

class SGRotateAnimation::SpinUpdateCallback : public osg::NodeCallback {
public:
  SpinUpdateCallback(SGCondition const* condition,
                     SGExpressiond const* animationValue) :
    _condition(condition),
    _animationValue(animationValue),
    _lastTime(-1)
  { }
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    if (!_condition || _condition->test()) {
      SGRotateTransform* transform;
      transform = static_cast<SGRotateTransform*>(node);

      double t = nv->getFrameStamp()->getReferenceTime();
      double dt = 0;
      if (0 <= _lastTime)
        dt = t - _lastTime;
      _lastTime = t;
      double velocity_rpms = _animationValue->getValue()/60;
      double angle = transform->getAngleDeg();
      angle += dt*velocity_rpms*360;
      angle -= 360*floor(angle/360);
      transform->setAngleDeg(angle);
    }
    traverse(node, nv);
  }
public:
  SGSharedPtr<SGCondition const> _condition;
  SGSharedPtr<SGExpressiond const> _animationValue;
  double _lastTime;
};

SGRotateAnimation::SGRotateAnimation(const SGPropertyNode* configNode,
                                     SGPropertyNode* modelRoot) :
  SGAnimation(configNode, modelRoot)
{
  std::string type = configNode->getStringValue("type", "");
  _isSpin = (type == "spin");

  _condition = getCondition();
  SGSharedPtr<SGExpressiond> value;
  value = read_value(configNode, modelRoot, "-deg",
                     -SGLimitsd::max(), SGLimitsd::max());
  _animationValue = value->simplify();
  if (_animationValue)
    _initialValue = _animationValue->getValue();
  else
    _initialValue = 0;

  _center = SGVec3d::zeros();
  if (configNode->hasValue("axis/x1-m")) {
    SGVec3d v1, v2;
    v1[0] = configNode->getDoubleValue("axis/x1-m", 0);
    v1[1] = configNode->getDoubleValue("axis/y1-m", 0);
    v1[2] = configNode->getDoubleValue("axis/z1-m", 0);
    v2[0] = configNode->getDoubleValue("axis/x2-m", 0);
    v2[1] = configNode->getDoubleValue("axis/y2-m", 0);
    v2[2] = configNode->getDoubleValue("axis/z2-m", 0);
    _center = 0.5*(v1+v2);
    _axis = v2 - v1;
  } else {
    _axis[0] = configNode->getDoubleValue("axis/x", 0);
    _axis[1] = configNode->getDoubleValue("axis/y", 0);
    _axis[2] = configNode->getDoubleValue("axis/z", 0);
  }
  if (8*SGLimitsd::min() < norm(_axis))
    _axis = normalize(_axis);

  _center[0] = configNode->getDoubleValue("center/x-m", _center[0]);
  _center[1] = configNode->getDoubleValue("center/y-m", _center[1]);
  _center[2] = configNode->getDoubleValue("center/z-m", _center[2]);
}

osg::Group*
SGRotateAnimation::createAnimationGroup(osg::Group& parent)
{
  SGRotateTransform* transform = new SGRotateTransform;
  transform->setName("rotate animation");
  if (_isSpin) {
    SpinUpdateCallback* uc;
    uc = new SpinUpdateCallback(_condition, _animationValue);
    transform->setUpdateCallback(uc);
  } else if (_animationValue || !_animationValue->isConst()) {
    UpdateCallback* uc = new UpdateCallback(_condition, _animationValue);
    transform->setUpdateCallback(uc);
  }
  transform->setCenter(_center);
  transform->setAxis(_axis);
  transform->setAngleDeg(_initialValue);
  parent.addChild(transform);
  return transform;
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

SGScaleAnimation::SGScaleAnimation(const SGPropertyNode* configNode,
                                   SGPropertyNode* modelRoot) :
  SGAnimation(configNode, modelRoot)
{
  _condition = getCondition();

  // default offset/factor for all directions
  double offset = configNode->getDoubleValue("offset", 0);
  double factor = configNode->getDoubleValue("factor", 1);

  SGSharedPtr<SGExpressiond> inPropExpr;

  std::string inputPropertyName;
  inputPropertyName = configNode->getStringValue("property", "");
  if (inputPropertyName.empty()) {
    inPropExpr = new SGConstExpression<double>(0);
  } else {
    SGPropertyNode* inputProperty;
    inputProperty = modelRoot->getNode(inputPropertyName, true);
    inPropExpr = new SGPropertyExpression<double>(inputProperty);
  }

  SGInterpTable* interpTable = read_interpolation_table(configNode);
  if (interpTable) {
    SGSharedPtr<SGExpressiond> value;
    value = new SGInterpTableExpression<double>(inPropExpr, interpTable);
    _animationValue[0] = value->simplify();
    _animationValue[1] = value->simplify();
    _animationValue[2] = value->simplify();
  } else if (configNode->getBoolValue("use-personality", false)) {
    SGSharedPtr<SGExpressiond> value;
    value = new SGPersonalityScaleOffsetExpression(inPropExpr, configNode,
                                                   "x-factor", "x-offset",
                                                   factor, offset);
    double minClip = configNode->getDoubleValue("x-min", 0);
    double maxClip = configNode->getDoubleValue("x-max", SGLimitsd::max());
    value = new SGClipExpression<double>(value, minClip, maxClip);
    _animationValue[0] = value->simplify();
    
    value = new SGPersonalityScaleOffsetExpression(inPropExpr, configNode,
                                                   "y-factor", "y-offset",
                                                   factor, offset);
    minClip = configNode->getDoubleValue("y-min", 0);
    maxClip = configNode->getDoubleValue("y-max", SGLimitsd::max());
    value = new SGClipExpression<double>(value, minClip, maxClip);
    _animationValue[1] = value->simplify();
    
    value = new SGPersonalityScaleOffsetExpression(inPropExpr, configNode,
                                                   "z-factor", "z-offset",
                                                   factor, offset);
    minClip = configNode->getDoubleValue("z-min", 0);
    maxClip = configNode->getDoubleValue("z-max", SGLimitsd::max());
    value = new SGClipExpression<double>(value, minClip, maxClip);
    _animationValue[2] = value->simplify();
  } else {
    SGSharedPtr<SGExpressiond> value;
    value = read_factor_offset(configNode, inPropExpr, "x-factor", "x-offset");
    double minClip = configNode->getDoubleValue("x-min", 0);
    double maxClip = configNode->getDoubleValue("x-max", SGLimitsd::max());
    value = new SGClipExpression<double>(value, minClip, maxClip);
    _animationValue[0] = value->simplify();

    value = read_factor_offset(configNode, inPropExpr, "y-factor", "y-offset");
    minClip = configNode->getDoubleValue("y-min", 0);
    maxClip = configNode->getDoubleValue("y-max", SGLimitsd::max());
    value = new SGClipExpression<double>(value, minClip, maxClip);
    _animationValue[1] = value->simplify();

    value = read_factor_offset(configNode, inPropExpr, "z-factor", "z-offset");
    minClip = configNode->getDoubleValue("z-min", 0);
    maxClip = configNode->getDoubleValue("z-max", SGLimitsd::max());
    value = new SGClipExpression<double>(value, minClip, maxClip);
    _animationValue[2] = value->simplify();
  }
  _initialValue[0] = configNode->getDoubleValue("x-starting-scale", 1);
  _initialValue[0] *= configNode->getDoubleValue("x-factor", factor);
  _initialValue[0] += configNode->getDoubleValue("x-offset", offset);
  _initialValue[1] = configNode->getDoubleValue("y-starting-scale", 1);
  _initialValue[1] *= configNode->getDoubleValue("y-factor", factor);
  _initialValue[1] += configNode->getDoubleValue("y-offset", offset);
  _initialValue[2] = configNode->getDoubleValue("z-starting-scale", 1);
  _initialValue[2] *= configNode->getDoubleValue("z-factor", factor);
  _initialValue[2] += configNode->getDoubleValue("z-offset", offset);
  _center[0] = configNode->getDoubleValue("center/x-m", 0);
  _center[1] = configNode->getDoubleValue("center/y-m", 0);
  _center[2] = configNode->getDoubleValue("center/z-m", 0);
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


SGDistScaleAnimation::SGDistScaleAnimation(const SGPropertyNode* configNode,
                                           SGPropertyNode* modelRoot) :
  SGAnimation(configNode, modelRoot)
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


SGFlashAnimation::SGFlashAnimation(const SGPropertyNode* configNode,
                                   SGPropertyNode* modelRoot) :
  SGAnimation(configNode, modelRoot)
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


SGBillboardAnimation::SGBillboardAnimation(const SGPropertyNode* configNode,
                                           SGPropertyNode* modelRoot) :
  SGAnimation(configNode, modelRoot)
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
  {}
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

SGRangeAnimation::SGRangeAnimation(const SGPropertyNode* configNode,
                                   SGPropertyNode* modelRoot) :
  SGAnimation(configNode, modelRoot)
{
  _condition = getCondition();

  std::string inputPropertyName;
  inputPropertyName = configNode->getStringValue("min-property", "");
  if (!inputPropertyName.empty()) {
    SGPropertyNode* inputProperty;
    inputProperty = modelRoot->getNode(inputPropertyName, true);
    SGSharedPtr<SGExpressiond> value;
    value = new SGPropertyExpression<double>(inputProperty);

    value = read_factor_offset(configNode, value, "min-factor", "min-offset");
    _minAnimationValue = value->simplify();
  }
  inputPropertyName = configNode->getStringValue("max-property", "");
  if (!inputPropertyName.empty()) {
    SGPropertyNode* inputProperty;
    inputProperty = modelRoot->getNode(inputPropertyName.c_str(), true);

    SGSharedPtr<SGExpressiond> value;
    value = new SGPropertyExpression<double>(inputProperty);

    value = read_factor_offset(configNode, value, "max-factor", "max-offset");
    _maxAnimationValue = value->simplify();
  }

  _initialValue[0] = configNode->getDoubleValue("min-m", 0);
  _initialValue[0] *= configNode->getDoubleValue("min-factor", 1);
  _initialValue[1] = configNode->getDoubleValue("max-m", SGLimitsf::max());
  _initialValue[1] *= configNode->getDoubleValue("max-factor", 1);
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

SGSelectAnimation::SGSelectAnimation(const SGPropertyNode* configNode,
                                     SGPropertyNode* modelRoot) :
  SGAnimation(configNode, modelRoot)
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

SGAlphaTestAnimation::SGAlphaTestAnimation(const SGPropertyNode* configNode,
                                           SGPropertyNode* modelRoot) :
  SGAnimation(configNode, modelRoot)
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
  { }
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


SGBlendAnimation::SGBlendAnimation(const SGPropertyNode* configNode,
                                   SGPropertyNode* modelRoot)
  : SGAnimation(configNode, modelRoot),
    _animationValue(read_value(configNode, modelRoot, "", 0, 1))
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
    double t = nv->getFrameStamp()->getReferenceTime();
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


SGTimedAnimation::SGTimedAnimation(const SGPropertyNode* configNode,
                                   SGPropertyNode* modelRoot)
  : SGAnimation(configNode, modelRoot)
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
  {}
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

SGShadowAnimation::SGShadowAnimation(const SGPropertyNode* configNode,
                                     SGPropertyNode* modelRoot) :
  SGAnimation(configNode, modelRoot)
{
}

osg::Group*
SGShadowAnimation::createAnimationGroup(osg::Group& parent)
{
  SGSharedPtr<SGCondition const> condition = getCondition();
  if (!condition)
    return 0;

  osg::Group* group = new osg::Group;
  group->setName("shadow animation");
  group->setUpdateCallback(new UpdateCallback(condition));
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
    set_rotation(tmp, _value, _center, _axis);
    matrix.preMult(tmp);
  }
private:
  SGVec3d _axis;
  SGVec3d _center;
};

class SGTexTransformAnimation::UpdateCallback :
  public osg::StateAttribute::Callback {
public:
  UpdateCallback(const SGCondition* condition) :
    _condition(condition)
  { }
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

SGTexTransformAnimation::SGTexTransformAnimation(const SGPropertyNode* configNode,
                                                 SGPropertyNode* modelRoot) :
  SGAnimation(configNode, modelRoot)
{
}

osg::Group*
SGTexTransformAnimation::createAnimationGroup(osg::Group& parent)
{
  osg::Group* group = new osg::Group;
  group->setName("texture transform group");
  osg::StateSet* stateSet = group->getOrCreateStateSet();
  stateSet->setDataVariance(osg::Object::DYNAMIC);  
  osg::TexMat* texMat = new osg::TexMat;
  UpdateCallback* updateCallback = new UpdateCallback(getCondition());
  // interpret the configs ...
  std::string type = getType();

  if (type == "textranslate") {
    appendTexTranslate(getConfig(), updateCallback);
  } else if (type == "texrotate") {
    appendTexRotate(getConfig(), updateCallback);
  } else if (type == "texmultiple") {
    std::vector<SGSharedPtr<SGPropertyNode> > transformConfigs;
    transformConfigs = getConfig()->getChildren("transform");
    for (unsigned i = 0; i < transformConfigs.size(); ++i) {
      std::string subtype = transformConfigs[i]->getStringValue("subtype", "");
      if (subtype == "textranslate")
        appendTexTranslate(transformConfigs[i], updateCallback);
      else if (subtype == "texrotate")
        appendTexRotate(transformConfigs[i], updateCallback);
      else
        SG_LOG(SG_INPUT, SG_ALERT,
               "Ignoring unknown texture transform subtype");
    }
  } else {
    SG_LOG(SG_INPUT, SG_ALERT, "Ignoring unknown texture transform type");
  }

  texMat->setUpdateCallback(updateCallback);
  stateSet->setTextureAttribute(0, texMat);
  parent.addChild(group);
  return group;
}

void
SGTexTransformAnimation::appendTexTranslate(const SGPropertyNode* config,
                                            UpdateCallback* updateCallback)
{
  std::string propertyName = config->getStringValue("property", "");
  SGSharedPtr<SGExpressiond> value;
  if (propertyName.empty())
    value = new SGConstExpression<double>(0);
  else {
    SGPropertyNode* inputProperty = getModelRoot()->getNode(propertyName, true);
    value = new SGPropertyExpression<double>(inputProperty);
  }

  SGInterpTable* table = read_interpolation_table(config);
  if (table) {
    value = new SGInterpTableExpression<double>(value, table);
    double biasValue = config->getDoubleValue("bias", 0);
    if (biasValue != 0)
      value = new SGBiasExpression<double>(value, biasValue);
    value = new SGStepExpression<double>(value,
                                         config->getDoubleValue("step", 0),
                                         config->getDoubleValue("scroll", 0));
    value = value->simplify();
  } else {
    double biasValue = config->getDoubleValue("bias", 0);
    if (biasValue != 0)
      value = new SGBiasExpression<double>(value, biasValue);
    value = new SGStepExpression<double>(value,
                                         config->getDoubleValue("step", 0),
                                         config->getDoubleValue("scroll", 0));
    value = read_offset_factor(config, value, "factor", "offset");

    if (config->hasChild("min") || config->hasChild("max")) {
      double minClip = config->getDoubleValue("min", -SGLimitsd::max());
      double maxClip = config->getDoubleValue("max", SGLimitsd::max());
      value = new SGClipExpression<double>(value, minClip, maxClip);
    }
    value = value->simplify();
  }
  SGVec3d axis(config->getDoubleValue("axis/x", 0),
               config->getDoubleValue("axis/y", 0),
               config->getDoubleValue("axis/z", 0));
  Translation* translation;
  translation = new Translation(normalize(axis));
  translation->setValue(config->getDoubleValue("starting-position", 0));
  updateCallback->appendTransform(translation, value);
}

void
SGTexTransformAnimation::appendTexRotate(const SGPropertyNode* config,
                                         UpdateCallback* updateCallback)
{
  std::string propertyName = config->getStringValue("property", "");
  SGSharedPtr<SGExpressiond> value;
  if (propertyName.empty())
    value = new SGConstExpression<double>(0);
  else {
    SGPropertyNode* inputProperty = getModelRoot()->getNode(propertyName, true);
    value = new SGPropertyExpression<double>(inputProperty);
  }

  SGInterpTable* table = read_interpolation_table(config);
  if (table) {
    value = new SGInterpTableExpression<double>(value, table);
    double biasValue = config->getDoubleValue("bias", 0);
    if (biasValue != 0)
      value = new SGBiasExpression<double>(value, biasValue);
    value = new SGStepExpression<double>(value,
                                         config->getDoubleValue("step", 0),
                                         config->getDoubleValue("scroll", 0));
    value = value->simplify();
  } else {
    double biasValue = config->getDoubleValue("bias", 0);
    if (biasValue != 0)
      value = new SGBiasExpression<double>(value, biasValue);
    value = new SGStepExpression<double>(value,
                                         config->getDoubleValue("step", 0),
                                         config->getDoubleValue("scroll", 0));
    value = read_offset_factor(config, value, "factor", "offset-deg");

    if (config->hasChild("min-deg") || config->hasChild("max-deg")) {
      double minClip = config->getDoubleValue("min-deg", -SGLimitsd::max());
      double maxClip = config->getDoubleValue("max-deg", SGLimitsd::max());
      value = new SGClipExpression<double>(value, minClip, maxClip);
    }
    value = value->simplify();
  }
  SGVec3d axis(config->getDoubleValue("axis/x", 0),
               config->getDoubleValue("axis/y", 0),
               config->getDoubleValue("axis/z", 0));
  SGVec3d center(config->getDoubleValue("center/x", 0),
                 config->getDoubleValue("center/y", 0),
                 config->getDoubleValue("center/z", 0));
  Rotation* rotation;
  rotation = new Rotation(normalize(axis), center);
  rotation->setValue(config->getDoubleValue("starting-position-deg", 0));
  updateCallback->appendTransform(rotation, value);
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGPickAnimation
////////////////////////////////////////////////////////////////////////

class SGPickAnimation::PickCallback : public SGPickCallback {
public:
  PickCallback(const SGPropertyNode* configNode,
               SGPropertyNode* modelRoot) :
    _repeatable(configNode->getBoolValue("repeatable", false)),
    _repeatInterval(configNode->getDoubleValue("interval-sec", 0.1))
  {
    SG_LOG(SG_INPUT, SG_DEBUG, "Reading all bindings");
    std::vector<SGPropertyNode_ptr> bindings;

    bindings = configNode->getChildren("button");
    for (unsigned int i = 0; i < bindings.size(); ++i) {
      _buttons.push_back( bindings[i]->getIntValue() );
    }
    bindings = configNode->getChildren("binding");
    for (unsigned int i = 0; i < bindings.size(); ++i) {
      _bindingsDown.push_back(new SGBinding(bindings[i], modelRoot));
    }

    const SGPropertyNode* upNode = configNode->getChild("mod-up");
    if (!upNode)
      return;
    bindings = upNode->getChildren("binding");
    for (unsigned int i = 0; i < bindings.size(); ++i) {
      _bindingsUp.push_back(new SGBinding(bindings[i], modelRoot));
    }
  }
  virtual bool buttonPressed(int button, const Info&)
  {
    bool found = false;
    for( std::vector<int>::iterator it = _buttons.begin(); it != _buttons.end(); ++it ) {
      if( *it == button ) {
        found = true;
        break;
      }
    }
    if (!found )
      return false;
    SGBindingList::const_iterator i;
    for (i = _bindingsDown.begin(); i != _bindingsDown.end(); ++i)
      (*i)->fire();
    _repeatTime = -_repeatInterval;    // anti-bobble: delay start of repeat
    return true;
  }
  virtual void buttonReleased(void)
  {
    SGBindingList::const_iterator i;
    for (i = _bindingsUp.begin(); i != _bindingsUp.end(); ++i)
      (*i)->fire();
  }
  virtual void update(double dt)
  {
    if (!_repeatable)
      return;

    _repeatTime += dt;
    while (_repeatInterval < _repeatTime) {
      _repeatTime -= _repeatInterval;
      SGBindingList::const_iterator i;
      for (i = _bindingsDown.begin(); i != _bindingsDown.end(); ++i)
        (*i)->fire();
    }
  }
private:
  SGBindingList _bindingsDown;
  SGBindingList _bindingsUp;
  std::vector<int> _buttons;
  bool _repeatable;
  double _repeatInterval;
  double _repeatTime;
};

class VncVisitor : public osg::NodeVisitor {
 public:
  VncVisitor(double x, double y, int mask) :
    osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
    _texX(x), _texY(y), _mask(mask), _done(false)
  {
    SG_LOG(SG_INPUT, SG_DEBUG, "VncVisitor constructor "
      << x << "," << y << " mask " << mask);
  }

  virtual void apply(osg::Node &node)
  {
    // Some nodes have state sets attached
    touchStateSet(node.getStateSet());
    if (!_done)
      traverse(node);
    if (_done) return;
    // See whether we are a geode worth exploring
    osg::Geode *g = dynamic_cast<osg::Geode*>(&node);
    if (!g) return;
    // Go find all its drawables
    int i = g->getNumDrawables();
    while (--i >= 0) {
      osg::Drawable *d = g->getDrawable(i);
      if (d) touchDrawable(*d);
    }
    // Out of optimism, do the same for EffectGeode
    simgear::EffectGeode *eg = dynamic_cast<simgear::EffectGeode*>(&node);
    if (!eg) return;
    for (simgear::EffectGeode::DrawablesIterator di = eg->drawablesBegin();
         di != eg->drawablesEnd(); di++) {
      touchDrawable(**di);
    }
    // Now see whether the EffectGeode has an Effect
    simgear::Effect *e = eg->getEffect();
    if (e) {
      touchStateSet(e->getDefaultStateSet());
    }
  }

  inline void touchDrawable(osg::Drawable &d)
  {
    osg::StateSet *ss = d.getStateSet();
    touchStateSet(ss);
  }

  void touchStateSet(osg::StateSet *ss)
  {
    if (!ss) return;
    osg::StateAttribute *sa = ss->getTextureAttribute(0,
      osg::StateAttribute::TEXTURE);
    if (!sa) return;
    osg::Texture *t = sa->asTexture();
    if (!t) return;
    osg::Image *img = t->getImage(0);
    if (!img) return;
    if (!_done) {
      int pixX = _texX * img->s();
      int pixY = _texY * img->t();
      _done = img->sendPointerEvent(pixX, pixY, _mask);
      SG_LOG(SG_INPUT, SG_DEBUG, "VncVisitor image said " << _done
        << " to coord " << pixX << "," << pixY);
    }
  }

  inline bool wasSuccessful()
  {
    return _done;
  }

 private:
  double _texX, _texY;
  int _mask;
  bool _done;
};


class SGPickAnimation::VncCallback : public SGPickCallback {
public:
  VncCallback(const SGPropertyNode* configNode,
               SGPropertyNode* modelRoot,
               osg::Group *node)
      : _node(node)
  {
    SG_LOG(SG_INPUT, SG_DEBUG, "Configuring VNC callback");
    const char *cornernames[3] = {"top-left", "top-right", "bottom-left"};
    SGVec3d *cornercoords[3] = {&_topLeft, &_toRight, &_toDown};
    for (int c =0; c < 3; c++) {
      const SGPropertyNode* cornerNode = configNode->getChild(cornernames[c]);
      *cornercoords[c] = SGVec3d(
        cornerNode->getDoubleValue("x"),
        cornerNode->getDoubleValue("y"),
        cornerNode->getDoubleValue("z"));
    }
    _toRight -= _topLeft;
    _toDown -= _topLeft;
    _squaredRight = dot(_toRight, _toRight);
    _squaredDown = dot(_toDown, _toDown);
  }

  virtual bool buttonPressed(int button, const Info& info)
  {
    SGVec3d loc(info.local);
    SG_LOG(SG_INPUT, SG_DEBUG, "VNC pressed " << button << ": " << loc);
    loc -= _topLeft;
    _x = dot(loc, _toRight) / _squaredRight;
    _y = dot(loc, _toDown) / _squaredDown;
    if (_x<0) _x = 0; else if (_x > 1) _x = 1;
    if (_y<0) _y = 0; else if (_y > 1) _y = 1;
    VncVisitor vv(_x, _y, 1 << button);
    _node->accept(vv);
    return vv.wasSuccessful();

  }
  virtual void buttonReleased(void)
  {
    SG_LOG(SG_INPUT, SG_DEBUG, "VNC release");
    VncVisitor vv(_x, _y, 0);
    _node->accept(vv);
  }
  virtual void update(double dt)
  {
  }
private:
  double _x, _y;
  osg::ref_ptr<osg::Group> _node;
  SGVec3d _topLeft, _toRight, _toDown;
  double _squaredRight, _squaredDown;
};

SGPickAnimation::SGPickAnimation(const SGPropertyNode* configNode,
                                 SGPropertyNode* modelRoot) :
  SGAnimation(configNode, modelRoot)
{
}

namespace
{
Mutex colorModeUniformMutex;
osg::ref_ptr<osg::Uniform> colorModeUniform;
}

osg::Group*
SGPickAnimation::createAnimationGroup(osg::Group& parent)
{
  osg::Group* commonGroup = new osg::Group;

  // Contains the normal geometry that is interactive
  osg::ref_ptr<osg::Group> normalGroup = new osg::Group;
  normalGroup->setName("pick normal group");
  normalGroup->addChild(commonGroup);

  // Used to render the geometry with just yellow edges
  osg::Group* highlightGroup = new osg::Group;
  highlightGroup->setName("pick highlight group");
  highlightGroup->setNodeMask(SG_NODEMASK_PICK_BIT);
  highlightGroup->addChild(commonGroup);
  SGSceneUserData* ud;
  ud = SGSceneUserData::getOrCreateSceneUserData(commonGroup);

  // add actions that become macro and command invocations
  std::vector<SGPropertyNode_ptr> actions;
  actions = getConfig()->getChildren("action");
  for (unsigned int i = 0; i < actions.size(); ++i)
    ud->addPickCallback(new PickCallback(actions[i], getModelRoot()));
  // Look for the VNC sessions that want raw mouse input
  actions = getConfig()->getChildren("vncaction");
  for (unsigned int i = 0; i < actions.size(); ++i)
    ud->addPickCallback(new VncCallback(actions[i], getModelRoot(),
      &parent));

  // prepare a state set that paints the edges of this object yellow
  // The material and texture attributes are set with
  // OVERRIDE|PROTECTED in case there is a material animation on a
  // higher node in the scene graph, which would have its material
  // attribute set with OVERRIDE.
  osg::StateSet* stateSet = highlightGroup->getOrCreateStateSet();
  osg::Texture2D* white = StateAttributeFactory::instance()->getWhiteTexture();
  stateSet->setTextureAttributeAndModes(0, white,
                                        (osg::StateAttribute::ON
                                         | osg::StateAttribute::OVERRIDE
                                         | osg::StateAttribute::PROTECTED));
  osg::PolygonOffset* polygonOffset = new osg::PolygonOffset;
  polygonOffset->setFactor(-1);
  polygonOffset->setUnits(-1);
  stateSet->setAttribute(polygonOffset, osg::StateAttribute::OVERRIDE);
  stateSet->setMode(GL_POLYGON_OFFSET_LINE,
                    osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
  osg::PolygonMode* polygonMode = new osg::PolygonMode;
  polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK,
                       osg::PolygonMode::LINE);
  stateSet->setAttribute(polygonMode, osg::StateAttribute::OVERRIDE);
  osg::Material* material = new osg::Material;
  material->setColorMode(osg::Material::OFF);
  material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0, 0, 0, 1));
  // XXX Alpha < 1.0 in the diffuse material value is a signal to the
  // default shader to take the alpha value from the material value
  // and not the glColor. In many cases the pick animation geometry is
  // transparent, so the outline would not be visible without this hack.
  material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0, 0, 0, .95));
  material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 0, 1));
  material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0, 0, 0, 0));
  stateSet->setAttribute(
      material, osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED);
  // The default shader has a colorMode uniform that mimics the
  // behavior of Material color mode.
  osg::Uniform* cmUniform = 0;
  {
      ScopedLock<Mutex> lock(colorModeUniformMutex);
      if (!colorModeUniform.valid()) {
          colorModeUniform = new osg::Uniform(osg::Uniform::INT, "colorMode");
          colorModeUniform->set(0); // MODE_OFF
          colorModeUniform->setDataVariance(osg::Object::STATIC);
      }
      cmUniform = colorModeUniform.get();
  }
  stateSet->addUniform(cmUniform,
                       osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
  // Only add normal geometry if configured
  if (getConfig()->getBoolValue("visible", true))
    parent.addChild(normalGroup.get());
  parent.addChild(highlightGroup);

  return commonGroup;
}
