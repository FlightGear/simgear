// animation.cxx - classes to manage model animation.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "SGMaterialAnimation.hxx"

#include <osg/AlphaFunc>
#include <osg/Array>
#include <osg/Drawable>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/StateSet>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/Pass.hxx>
#include <simgear/scene/material/Technique.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/model/ConditionNode.hxx>
#include <simgear/scene/util/OsgMath.hxx>

using namespace std;
using namespace simgear;

namespace {
/**
 * Get a color from properties.
 */
struct ColorSpec {
  float red, green, blue;
  float factor;
  float offset;
  SGPropertyNode_ptr red_prop;
  SGPropertyNode_ptr green_prop;
  SGPropertyNode_ptr blue_prop;
  SGPropertyNode_ptr factor_prop;
  SGPropertyNode_ptr offset_prop;
  SGVec4f v;
  
  ColorSpec(const SGPropertyNode* configNode, SGPropertyNode* modelRoot)
  {
    red = -1.0;
    green = -1.0;
    blue = -1.0;
    if (!configNode)
      return;
    
    red = configNode->getFloatValue("red", -1.0);
    green = configNode->getFloatValue("green", -1.0);
    blue = configNode->getFloatValue("blue", -1.0);
    factor = configNode->getFloatValue("factor", 1.0);
    offset = configNode->getFloatValue("offset", 0.0);
    
    if (!modelRoot)
      return;
    const SGPropertyNode *node;
    node = configNode->getChild("red-prop");
    if (node)
      red_prop = modelRoot->getNode(node->getStringValue(), true);
    node = configNode->getChild("green-prop");
    if (node)
      green_prop = modelRoot->getNode(node->getStringValue(), true);
    node = configNode->getChild("blue-prop");
    if (node)
      blue_prop = modelRoot->getNode(node->getStringValue(), true);
    node = configNode->getChild("factor-prop");
    if (node)
      factor_prop = modelRoot->getNode(node->getStringValue(), true);
    node = configNode->getChild("offset-prop");
    if (node)
      offset_prop = modelRoot->getNode(node->getStringValue(), true);
  }
  
  bool dirty() {
    return red >= 0 || green >= 0 || blue >= 0;
  }
  bool live() {
    return red_prop || green_prop || blue_prop
      || factor_prop || offset_prop;
  }
  SGVec4f &rgba() {
    if (red_prop)
      red = red_prop->getFloatValue();
    if (green_prop)
      green = green_prop->getFloatValue();
    if (blue_prop)
      blue = blue_prop->getFloatValue();
    if (factor_prop)
      factor = factor_prop->getFloatValue();
    if (offset_prop)
      offset = offset_prop->getFloatValue();
    v[0] = SGMiscf::clip(red*factor + offset, 0, 1);
    v[1] = SGMiscf::clip(green*factor + offset, 0, 1);
    v[2] = SGMiscf::clip(blue*factor + offset, 0, 1);
    v[3] = 1;
    return v;
  }

  osg::Vec4 rgbaVec4()
  {
    return toOsg(rgba());
  }
  
  SGVec4f &initialRgba() {
    v[0] = SGMiscf::clip(red*factor + offset, 0, 1);
    v[1] = SGMiscf::clip(green*factor + offset, 0, 1);
    v[2] = SGMiscf::clip(blue*factor + offset, 0, 1);
    v[3] = 1;
    return v;
  }
};

/**
 * Get a property value from a property.
 */
struct PropSpec {
  float value;
  float factor;
  float offset;
  float min;
  float max;
  SGPropertyNode_ptr value_prop;
  SGPropertyNode_ptr factor_prop;
  SGPropertyNode_ptr offset_prop;
  
  PropSpec(const char* valueName, const char* valuePropName,
           const SGPropertyNode* configNode, SGPropertyNode* modelRoot)
  {
    value = -1;
    if (!configNode)
      return;
    
    value = configNode->getFloatValue(valueName, -1);
    factor = configNode->getFloatValue("factor", 1);
    offset = configNode->getFloatValue("offset", 0);
    min = configNode->getFloatValue("min", 0);
    max = configNode->getFloatValue("max", 1);
    
    if (!modelRoot)
      return;
    const SGPropertyNode *node;
    node = configNode->getChild(valuePropName);
    if (node)
      value_prop = modelRoot->getNode(node->getStringValue(), true);
    node = configNode->getChild("factor-prop");
    if (node)
      factor_prop = modelRoot->getNode(node->getStringValue(), true);
    node = configNode->getChild("offset-prop");
    if (node)
      offset_prop = modelRoot->getNode(node->getStringValue(), true);
  }
  bool dirty() { return value >= 0.0; }
  bool live() { return value_prop || factor_prop || offset_prop; }
  float getValue()
  {
    if (value_prop)
      value = value_prop->getFloatValue();
    if (offset_prop)
      offset = offset_prop->getFloatValue();
    if (factor_prop)
      factor = factor_prop->getFloatValue();
    return SGMiscf::clip(value*factor + offset, min, max);
  }
  float getInitialValue()
  {
    return SGMiscf::clip(value*factor + offset, min, max);
  }
};

/**
 * The possible color properties supplied by a material animation.
 */
enum SuppliedColor {
  DIFFUSE = 1,
  AMBIENT = 2,
  SPECULAR = 4,
  EMISSION = 8,
  SHININESS = 16,
  TRANSPARENCY = 32
};

const unsigned AMBIENT_DIFFUSE = AMBIENT | DIFFUSE;

const int allMaterialColors = (DIFFUSE | AMBIENT | SPECULAR | EMISSION
			       | SHININESS);

// Visitor for finding default material colors in the animation node's
// subgraph. This makes some assumptions about the subgraph i.e.,
// there will be one material and one color value found. This is
// probably true for ac3d models and most uses of material animations,
// but will break down if, for example, you animate the transparency
// of a vertex colored model.
class MaterialDefaultsVisitor : public osg::NodeVisitor {
public:
  MaterialDefaultsVisitor()
    : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
      ambientDiffuse(-1.0f, -1.0f, -1.0f, -1.0f)
  {
    setVisitorType(osg::NodeVisitor::NODE_VISITOR);
  }

  virtual void apply(osg::Node& node)
  {
    maybeGetMaterialValues(node.getStateSet());
    traverse(node);
  }

  virtual void apply(osg::Geode& node)
  {
    using namespace simgear;
    EffectGeode* eg = dynamic_cast<EffectGeode*>(&node);
    if (eg) {
      const Effect* effect = eg->getEffect();
      if (effect)
        for (vector<osg::ref_ptr<Technique> >::const_iterator itr
               = effect->techniques.begin(), end = effect->techniques.end();
             itr != end;
             ++itr) {
          const Technique* tniq = itr->get();
          for (vector<osg::ref_ptr<Pass> >::const_iterator pitr
                 = tniq->passes.begin(), pend = tniq->passes.end();
               pitr != pend;
               ++pitr)
            maybeGetMaterialValues(pitr->get());
        }
    } else {
      maybeGetMaterialValues(node.getStateSet());
    }
    int numDrawables = node.getNumDrawables();
    for (int i = 0; i < numDrawables; i++) {
      osg::Geometry* geom = dynamic_cast<osg::Geometry*>(node.getDrawable(i));
      if (!geom || geom->getColorBinding() != osg::Geometry::BIND_OVERALL)
	continue;
      maybeGetMaterialValues(geom->getStateSet());
      osg::Array* colorArray = geom->getColorArray();
      osg::Vec4Array* colorVec4 = dynamic_cast<osg::Vec4Array*>(colorArray);
      if (colorVec4) {
	ambientDiffuse = (*colorVec4)[0];
	break;
      }
      osg::Vec3Array* colorVec3 = dynamic_cast<osg::Vec3Array*>(colorArray);
      if (colorVec3) {
	ambientDiffuse = osg::Vec4((*colorVec3)[0], 1.0f);
	break;
      }
    }
  }
  
  void maybeGetMaterialValues(const osg::StateSet* stateSet)
  {
    if (!stateSet)
      return;
    const osg::Material* nodeMat
      = dynamic_cast<const osg::Material*>(stateSet
                                           ->getAttribute(osg::StateAttribute
                                                          ::MATERIAL));
    if (!nodeMat)
      return;
    material = nodeMat;
  }

  osg::ref_ptr<const osg::Material> material;
  osg::Vec4 ambientDiffuse;
};

class MaterialPropertyAdapter
{
public:
    MaterialPropertyAdapter(const SGPropertyNode* configNode,
                            SGPropertyNode* modelRoot) :
        _ambient(configNode->getChild("ambient"), modelRoot),
        _diffuse(configNode->getChild("diffuse"), modelRoot),
        _specular(configNode->getChild("specular"), modelRoot),
        _emission(configNode->getChild("emission"), modelRoot),
        _shininess("shininess", "shininess-prop",
                   configNode/*->getChild("shininess")*/, modelRoot),
        _transparency("alpha", "alpha-prop",
                      configNode->getChild("transparency"), modelRoot)
    {
        _shininess.max = 128;
        _isAnimated = (_ambient.live() || _diffuse.live() || _specular.live()
                       || _emission.live() || _shininess.live()
                       || _transparency.live());
    }
    bool isAnimated() { return _isAnimated; }
    // This takes a StateSet argument because the rendering bin will
    // be changed if there is transparency.
    void setMaterialValues(osg::StateSet* stateSet)
    {
        osg::StateAttribute* stateAttribute
            = stateSet->getAttribute(osg::StateAttribute::MATERIAL);
        osg::Material* material = dynamic_cast<osg::Material*>(stateAttribute);
        if (material) {
            if (_ambient.live() || _ambient.dirty())
                material->setAmbient(osg::Material::FRONT_AND_BACK,
                                     _ambient.rgbaVec4());	
            if (_diffuse.live() || _diffuse.dirty())
                material->setDiffuse(osg::Material::FRONT_AND_BACK,
                                     _diffuse.rgbaVec4());
            if (_specular.live() || _specular.dirty())
                material->setSpecular(osg::Material::FRONT_AND_BACK,
                                      _specular.rgbaVec4());
            if (_emission.live() || _emission.dirty())
                material->setEmission(osg::Material::FRONT_AND_BACK,
                                      _emission.rgbaVec4());
            if (_shininess.live() || _shininess.dirty())
                material->setShininess(osg::Material::FRONT_AND_BACK,
                                       _shininess.getValue());
            if (_transparency.live() || _transparency.dirty()) {
                float alpha = _transparency.getValue();
                material->setAlpha(osg::Material::FRONT_AND_BACK, alpha);
                if (alpha < 1.0f) {
                    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
                } else {
                    stateSet->setRenderingHint(osg::StateSet::DEFAULT_BIN);
                }
            }
        }
    }
    ColorSpec _ambient;
    ColorSpec _diffuse;
    ColorSpec _specular;
    ColorSpec _emission;
    PropSpec _shininess;
    PropSpec _transparency;
    bool _isAnimated;

};

class UpdateCallback : public osg::NodeCallback {
public:
  UpdateCallback(const osgDB::FilePathList& texturePathList,
                 const SGPropertyNode* configNode, SGPropertyNode* modelRoot) :
    _materialProps(configNode, modelRoot),
    _texturePathList(texturePathList),
    _prevState(false)
  {
    const SGPropertyNode* node;

    node = configNode->getChild("threshold-prop");
    if (node)
      _thresholdProp = modelRoot->getNode(node->getStringValue(), true);
    node = configNode->getChild("texture-prop");
    if (node)
      _textureProp = modelRoot->getNode(node->getStringValue(), true);
  }

  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    osg::StateSet* stateSet = node->getStateSet();
    if (stateSet) {
      if (_textureProp) {
        std::string textureName = _textureProp->getStringValue();
        if (_textureName != textureName) {
          _textureName = textureName;

          while (stateSet->getTextureAttribute(0,
					       osg::StateAttribute::TEXTURE)) {
            stateSet->removeTextureAttribute(0, osg::StateAttribute::TEXTURE);
          }
          std::string textureFile;
          textureFile = osgDB::findFileInPath(textureName, _texturePathList);
          if (!textureFile.empty()) {
            osg::Texture2D* texture2D = SGLoadTexture2D(textureFile);
            if (texture2D) {
              stateSet->setTextureAttribute(0, texture2D,
					    osg::StateAttribute::OVERRIDE);
              stateSet->setTextureMode(0, GL_TEXTURE_2D,
                                       osg::StateAttribute::ON);
            }
          } else {
              SG_LOG(SG_IO, SG_DEV_WARN, "texture animation: requested texture : " << textureName << " not found. Searched paths:" );
              for( osgDB::FilePathList::iterator it = _texturePathList.begin(); it != _texturePathList.end(); ++it ) {
                SG_LOG(SG_IO, SG_DEV_WARN, " - " << *it );
              }
          }
        }
      }
      if (_thresholdProp) {
        osg::StateSet* stateSet = node->getOrCreateStateSet();
        osg::StateAttribute* stateAttribute;
        stateAttribute = stateSet->getAttribute(osg::StateAttribute::ALPHAFUNC);
        osg::AlphaFunc* alphaFunc
            = dynamic_cast<osg::AlphaFunc*>(stateAttribute);
	assert(alphaFunc);
        alphaFunc->setReferenceValue(_thresholdProp->getFloatValue());
      }
      if (_materialProps.isAnimated() || !_prevState)
          _materialProps.setMaterialValues(stateSet);
      _prevState = true;
    } else {
      _prevState = false;
    }
    traverse(node, nv);
  }
private:
  SGSharedPtr<const SGPropertyNode> _textureProp;
  SGSharedPtr<const SGPropertyNode> _thresholdProp;
  std::string _textureName;
  MaterialPropertyAdapter _materialProps;
  osgDB::FilePathList _texturePathList;
  bool _prevState;
};
} // namespace


SGMaterialAnimation::SGMaterialAnimation(simgear::SGTransientModelData &modelData) :
    SGAnimation(modelData), texturePathList(modelData.getOptions()->getDatabasePathList())
{
  if (modelData.getConfigNode()->hasChild("global"))
    SG_LOG(SG_IO, SG_DEV_ALERT, modelData.getPath() <<
           ": Use of <global> in material animation is "
           "no longer supported.");
}

osg::Group*
SGMaterialAnimation::createAnimationGroup(osg::Group& parent)
{
  osg::Group* group = new osg::Group;
  group->setName("material animation group");

  SGPropertyNode* inputRoot = getModelRoot();
  const SGPropertyNode* node = getConfig()->getChild("property-base");
  if (node)
    inputRoot = getModelRoot()->getNode(node->getStringValue(), true);
  osg::StateSet* stateSet = group->getOrCreateStateSet();  
  if (getConfig()->hasChild("texture")) {
    std::string textureName = getConfig()->getStringValue("texture");
    std::string textureFile;
    textureFile = osgDB::findFileInPath(textureName, texturePathList);
    if (!textureFile.empty()) {
      osg::Texture2D* texture2D = SGLoadTexture2D(textureFile);
      if (texture2D) {
        stateSet->setTextureAttribute(0, texture2D,
				      osg::StateAttribute::OVERRIDE);
        stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
		osg::ref_ptr<osg::Image> textureImage = texture2D->getImage();
        if (textureImage && textureImage->isImageTranslucent()) {
          stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
          stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        }
      }
    }
  }
  if (getConfig()->hasChild("threshold-prop") ||
      getConfig()->hasChild("threshold")) {
    osg::AlphaFunc* alphaFunc = new osg::AlphaFunc;
    alphaFunc->setFunction(osg::AlphaFunc::GREATER);
    float threshold = getConfig()->getFloatValue("threshold", 0);
    alphaFunc->setReferenceValue(threshold);
    stateSet->setAttribute(alphaFunc, osg::StateAttribute::OVERRIDE);
  }

  unsigned suppliedColors = 0;
  if (getConfig()->hasChild("ambient"))
    suppliedColors |= AMBIENT;
  if (getConfig()->hasChild("diffuse"))
    suppliedColors |= DIFFUSE;
  if (getConfig()->hasChild("specular"))
    suppliedColors |= SPECULAR;
  if (getConfig()->hasChild("emission"))
    suppliedColors |= EMISSION;
  if (getConfig()->hasChild("shininess")
      || getConfig()->hasChild("shininess-prop"))
    suppliedColors |= SHININESS;
  if (getConfig()->hasChild("transparency"))
    suppliedColors |= TRANSPARENCY;
  osg::Material* mat = 0;
  if (suppliedColors != 0) {
    if (defaultMaterial.valid()) {
      mat = defaultMaterial.get();

    } else {
      mat = new osg::Material;
      mat->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    }
    mat->setDataVariance(osg::Object::DYNAMIC);
    unsigned defaultColorModeMask = 0;
    mat->setUpdateCallback(0); // Just to make sure.
    // XXX This should probably go away, as ac3d models always have a
    // DIFFUSE color mode.
    switch (mat->getColorMode()) {
    case osg::Material::OFF:
      defaultColorModeMask = 0;
      break;
    case osg::Material::AMBIENT:
      defaultColorModeMask = AMBIENT;
      break;
    case osg::Material::DIFFUSE:
      defaultColorModeMask = DIFFUSE;
      break;
    case osg::Material::AMBIENT_AND_DIFFUSE:
      defaultColorModeMask = AMBIENT | DIFFUSE;
      break;
    case osg::Material::SPECULAR:
      defaultColorModeMask = SPECULAR;
      break;
    case osg::Material::EMISSION:
      defaultColorModeMask = EMISSION;
      break;
    }
    // Copy the color found by traversing geometry into the material
    // in case we need to specify it (e.g., transparency) and it is
    // not specified by the animation.
    if (defaultAmbientDiffuse.x() >= 0) {
      if (defaultColorModeMask & AMBIENT)
        mat->setAmbient(osg::Material::FRONT_AND_BACK, defaultAmbientDiffuse);
      if (defaultColorModeMask & DIFFUSE)
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, defaultAmbientDiffuse);
    }
    // Compute which colors in the animation override colors set via
    // colorMode / glColor, and set the colorMode for the animation's
    // material accordingly. 
    if (suppliedColors & TRANSPARENCY) {
      // All colors will be affected by the material. Hope all the
      // defaults are fine, if needed.
      mat->setColorMode(osg::Material::OFF);
    } else if ((suppliedColors & defaultColorModeMask) != 0) {
      // First deal with the complicated AMBIENT/DIFFUSE case.
      if ((defaultColorModeMask & AMBIENT_DIFFUSE) != 0) {
        // glColor can supply colors not specified by the animation.
        unsigned matColorModeMask = ((~suppliedColors & defaultColorModeMask)
                                     & AMBIENT_DIFFUSE);
        if ((matColorModeMask & DIFFUSE) != 0)
          mat->setColorMode(osg::Material::DIFFUSE);
        else if ((matColorModeMask & AMBIENT) != 0)
          mat->setColorMode(osg::Material::AMBIENT);
        else
          mat->setColorMode(osg::Material::OFF);
      } else {
        // The animation overrides the glColor color.
        mat->setColorMode(osg::Material::OFF);
      }
    } else {
      // No overlap between the animation and color mode, so leave
      // the color mode alone.
    }
    stateSet->setAttribute(mat,(osg::StateAttribute::ON
                                | osg::StateAttribute::OVERRIDE));
  }
  bool matAnimated = false;
  if (mat) {
    MaterialPropertyAdapter adapter(getConfig(), inputRoot);
    adapter.setMaterialValues(stateSet);
    matAnimated = adapter.isAnimated();
  }
  if (matAnimated || getConfig()->hasChild("texture-prop")
      || getConfig()->hasChild("threshold-prop") || getCondition()) {
    stateSet->setDataVariance(osg::Object::DYNAMIC);
    group->setUpdateCallback(new UpdateCallback(texturePathList,
                                                getConfig(), inputRoot));
  } else {
    stateSet->setDataVariance(osg::Object::STATIC);
  }
  if (getCondition()) {
    ConditionNode* cn = new ConditionNode;
    cn->setCondition(getCondition());
    osg::Group* modelGroup = new osg::Group;
    group->addChild(modelGroup);
    cn->addChild(group);
    cn->addChild(modelGroup);
    parent.addChild(cn);
    return modelGroup;
  } else {
    parent.addChild(group);
    return group;
  }
}

void
SGMaterialAnimation::install(osg::Node& node)
{
  SGAnimation::install(node);

    MaterialDefaultsVisitor defaultsVisitor;
    node.accept(defaultsVisitor);
    if (defaultsVisitor.material.valid()) {
      defaultMaterial
	= static_cast<osg::Material*>(defaultsVisitor.material->clone(osg::CopyOp::SHALLOW_COPY));
    }
    defaultAmbientDiffuse = defaultsVisitor.ambientDiffuse;
}

const char* colorNames[] =
{
    "ambient",
    "diffuse",
    "specular",
    "emission"
};

// Build an effect which mimics the material color mode in a
// shader. The OpenGL material values will be overridden by the
// material animation's material.
//
// This is a hack to get the effect to respect the values set in the
// material, set up by the animation, which overrides the values in
// the effect's material attributes. Things will be different when
// material animations are implemented purely by manipulating effects.

SGPropertyNode_ptr
SGMaterialAnimation::makeEffectProperties(const SGPropertyNode* animProp)
{
    SGPropertyNode_ptr eRoot = new SGPropertyNode;
    SGPropertyNode* inherit = makeNode(eRoot, "inherits-from");
    if (animProp->hasChild("diffuse") || animProp->hasChild("transparency"))
        inherit->setStringValue("Effects/material-off");
    else
        inherit->setStringValue("Effects/material-diffuse");
    return eRoot;
}
