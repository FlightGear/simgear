// animation.cxx - classes to manage model animation.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "SGMaterialAnimation.hxx"

#include <osg/AlphaFunc>
#include <osg/Drawable>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/StateSet>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/model/model.hxx>

struct SGMaterialAnimation::ColorSpec {
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
  SGVec4f &initialRgba() {
    v[0] = SGMiscf::clip(red*factor + offset, 0, 1);
    v[1] = SGMiscf::clip(green*factor + offset, 0, 1);
    v[2] = SGMiscf::clip(blue*factor + offset, 0, 1);
    v[3] = 1;
    return v;
  }
};


struct SGMaterialAnimation::PropSpec {
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

class SGMaterialAnimation::MaterialVisitor : public osg::NodeVisitor {
public:
  enum {
    DIFFUSE = 1,
    AMBIENT = 2,
    SPECULAR = 4,
    EMISSION = 8,
    SHININESS = 16,
    TRANSPARENCY = 32
  };

  MaterialVisitor() :
    osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
    _updateMask(0),
    _ambient(-1, -1, -1, -1),
    _diffuse(-1, -1, -1, -1),
    _specular(-1, -1, -1, -1),
    _emission(-1, -1, -1, -1),
    _shininess(-1),
    _alpha(-1)
  {
    setVisitorType(osg::NodeVisitor::NODE_VISITOR);
  }

  void setDiffuse(const SGVec4f& diffuse)
  {
    if (diffuse != _diffuse)
      _diffuse = diffuse;
    _updateMask |= DIFFUSE;
  }
  void setAmbient(const SGVec4f& ambient)
  {
    if (ambient != _ambient)
      _ambient = ambient;
    _updateMask |= AMBIENT;
  }
  void setSpecular(const SGVec4f& specular)
  {
    if (specular != _specular)
      _specular = specular;
    _updateMask |= SPECULAR;
  }
  void setEmission(const SGVec4f& emission)
  {
    if (emission != _emission)
      _emission = emission;
    _updateMask |= EMISSION;
  }
  void setShininess(float shininess)
  {
    if (shininess != _shininess)
      _shininess = shininess;
    _updateMask |= SHININESS;
  }

  void setAlpha(float alpha)
  {
    if (alpha != _alpha)
      _alpha = alpha;
    _updateMask |= TRANSPARENCY;
  }

  virtual void reset()
  {
    _updateMask = 0;
  }
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
      updateStateSet(drawable->getStateSet());

      if (_updateMask&TRANSPARENCY) {
        osg::Geometry* geometry = drawable->asGeometry();
        if (!geometry)
          continue;
        osg::Array* array = geometry->getColorArray();
        if (!array)
          continue;
        osg::Vec4Array* vec4Array = dynamic_cast<osg::Vec4Array*>(array);
        if (!vec4Array)
          continue;

        // FIXME, according to the colormode in the material
        // we might incorporate the apropriate color value
        geometry->dirtyDisplayList();
        vec4Array->dirty();
        for (unsigned k = 0; k < vec4Array->size(); ++k) {
          (*vec4Array)[k][3] = _alpha;
        }
      }
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
    if (_updateMask&AMBIENT)
      material->setAmbient(osg::Material::FRONT_AND_BACK, _ambient.osg());
    if (_updateMask&DIFFUSE)
      material->setDiffuse(osg::Material::FRONT_AND_BACK, _diffuse.osg());
    if (_updateMask&SPECULAR)
      material->setSpecular(osg::Material::FRONT_AND_BACK, _specular.osg());
    if (_updateMask&EMISSION)
      material->setEmission(osg::Material::FRONT_AND_BACK, _emission.osg());
    if (_updateMask&SHININESS)
      material->setShininess(osg::Material::FRONT_AND_BACK, _shininess);
    if (_updateMask&TRANSPARENCY) {
      material->setAlpha(osg::Material::FRONT_AND_BACK, _alpha);
      if (_alpha < 1) {
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
      } else {
        stateSet->setRenderingHint(osg::StateSet::DEFAULT_BIN);
      }
    }
  }
private:
  unsigned _updateMask;
  SGVec4f _ambient;
  SGVec4f _diffuse;
  SGVec4f _specular;
  SGVec4f _emission;
  float _shininess;
  float _alpha;
};

class SGMaterialAnimation::UpdateCallback : public osg::NodeCallback {
public:
  UpdateCallback(const osgDB::FilePathList& texturePathList,
                 const SGCondition* condition,
                 const SGPropertyNode* configNode, SGPropertyNode* modelRoot) :
    _condition(condition),
    _ambient(configNode->getChild("ambient"), modelRoot),
    _diffuse(configNode->getChild("diffuse"), modelRoot),
    _specular(configNode->getChild("specular"), modelRoot),
    _emission(configNode->getChild("emission"), modelRoot),
    _shininess("shininess", "shininess-prop",
               configNode->getChild("shininess"), modelRoot),
    _transparency("alpha", "alpha-prop",
                  configNode->getChild("transparency"), modelRoot),
    _texturePathList(texturePathList)
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
    if (!_condition || _condition->test()) {
      if (_textureProp) {
        std::string textureName = _textureProp->getStringValue();
        if (_textureName != textureName) {
          osg::StateSet* stateSet = node->getOrCreateStateSet();
          while (stateSet->getTextureAttribute(0, osg::StateAttribute::TEXTURE)) {
            stateSet->removeTextureAttribute(0, osg::StateAttribute::TEXTURE);
          }
          std::string textureFile;
          textureFile = osgDB::findFileInPath(textureName, _texturePathList);
          if (!textureFile.empty()) {
            osg::Texture2D* texture2D = SGLoadTexture2D(textureFile);
            if (texture2D) {
              stateSet->setTextureAttribute(0, texture2D);
              stateSet->setTextureMode(0, GL_TEXTURE_2D,
                                       osg::StateAttribute::ON);
              _textureName = textureName;
            }
          }
        }
      }
      if (_thresholdProp) {
        osg::StateSet* stateSet = node->getOrCreateStateSet();
        osg::StateAttribute* stateAttribute;
        stateAttribute = stateSet->getAttribute(osg::StateAttribute::ALPHAFUNC);
        assert(dynamic_cast<osg::AlphaFunc*>(stateAttribute));
        osg::AlphaFunc* alphaFunc = static_cast<osg::AlphaFunc*>(stateAttribute);
        alphaFunc->setReferenceValue(_thresholdProp->getFloatValue());
      }
      
      _visitor.reset();
      if (_ambient.live())
        _visitor.setAmbient(_ambient.rgba());
      if (_diffuse.live())
        _visitor.setDiffuse(_diffuse.rgba());
      if (_specular.live())
        _visitor.setSpecular(_specular.rgba());
      if (_emission.live())
        _visitor.setEmission(_emission.rgba());
      if (_shininess.live())
        _visitor.setShininess(_shininess.getValue());
      if (_transparency.live())
        _visitor.setAlpha(_transparency.getValue());
      
      node->accept(_visitor);
    }

    traverse(node, nv);
  }
private:
  SGSharedPtr<const SGCondition> _condition;
  SGSharedPtr<const SGPropertyNode> _textureProp;
  SGSharedPtr<const SGPropertyNode> _thresholdProp;
  MaterialVisitor _visitor;
  std::string _textureName;
  ColorSpec _ambient;
  ColorSpec _diffuse;
  ColorSpec _specular;
  ColorSpec _emission;
  PropSpec _shininess;
  PropSpec _transparency;
  osgDB::FilePathList _texturePathList;
};

SGMaterialAnimation::SGMaterialAnimation(const SGPropertyNode* configNode,
                                         SGPropertyNode* modelRoot) :
  SGAnimation(configNode, modelRoot)
{
  if (configNode->hasChild("global"))
    SG_LOG(SG_IO, SG_ALERT, "Using global material animation that can "
           "no longer work");
}

osg::Group*
SGMaterialAnimation::createAnimationGroup(osg::Group& parent)
{
  osg::Group* group = new osg::Group;
  group->setName("material animation group");

  SGPropertyNode* inputRoot = getModelRoot();
  const SGPropertyNode* node = getConfig()->getChild("property-base");
  if (node)
    inputRoot = getModelRoot()->getRootNode()->getNode(node->getStringValue(),
                                                       true);

  osgDB::FilePathList texturePathList = osgDB::getDataFilePathList();

  if (getConfig()->hasChild("texture")) {
    std::string textureName = getConfig()->getStringValue("texture");
    std::string textureFile;
    textureFile = osgDB::findFileInPath(textureName, texturePathList);
    if (!textureFile.empty()) {
      osg::StateSet* stateSet = group->getOrCreateStateSet();
      osg::Texture2D* texture2D = SGLoadTexture2D(textureFile);
      if (texture2D) {
        stateSet->setTextureAttribute(0, texture2D);
        stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
        if (texture2D->getImage()->isImageTranslucent()) {
          stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
          stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        }
      }
    }
  }
  if (getConfig()->hasChild("threshold-prop") ||
      getConfig()->hasChild("threshold")) {
    osg::StateSet* stateSet = group->getOrCreateStateSet();
    osg::AlphaFunc* alphaFunc = new osg::AlphaFunc;
    alphaFunc->setFunction(osg::AlphaFunc::GREATER);
    float threshold = getConfig()->getFloatValue("threshold", 0);
    alphaFunc->setReferenceValue(threshold);
    stateSet->setAttributeAndModes(alphaFunc);
  }

  UpdateCallback* updateCallback;
  updateCallback = new UpdateCallback(texturePathList, getCondition(),
                                      getConfig(), inputRoot);
  group->setUpdateCallback(updateCallback);
  parent.addChild(group);
  return group;
}

void
SGMaterialAnimation::install(osg::Node& node)
{
  SGAnimation::install(node);
  // make sure everything (except the texture attributes)
  // below is private to our model
  cloneDrawables(node);

  // Remove all textures if required, they get replaced later on
  if (getConfig()->hasChild("texture") ||
      getConfig()->hasChild("texture-prop")) {
    removeTextureAttribute(node, 0, osg::StateAttribute::TEXTURE);
    removeTextureMode(node, 0, GL_TEXTURE_2D);
  }
  // Remove all nested alphaFuncs
  if (getConfig()->hasChild("threshold") ||
      getConfig()->hasChild("threshold-prop"))
    removeAttribute(node, osg::StateAttribute::ALPHAFUNC);

  ColorSpec ambient(getConfig()->getChild("ambient"), getModelRoot());
  ColorSpec diffuse(getConfig()->getChild("diffuse"), getModelRoot());
  ColorSpec specular(getConfig()->getChild("specular"), getModelRoot());
  ColorSpec emission(getConfig()->getChild("emission"), getModelRoot());
  PropSpec shininess("shininess", "shininess-prop",
                     getConfig()->getChild("shininess"), getModelRoot());
  PropSpec transparency("alpha", "alpha-prop",
                        getConfig()->getChild("transparency"), getModelRoot());

  MaterialVisitor visitor;
  if (ambient.dirty())
    visitor.setAmbient(ambient.initialRgba());
  if (diffuse.dirty())
    visitor.setDiffuse(diffuse.initialRgba());
  if (specular.dirty())
    visitor.setSpecular(specular.initialRgba());
  if (emission.dirty())
    visitor.setEmission(emission.initialRgba());
  if (shininess.dirty())
    visitor.setShininess(shininess.getInitialValue());
  if (transparency.dirty())
    visitor.setAlpha(transparency.getInitialValue());
  node.accept(visitor);
}

