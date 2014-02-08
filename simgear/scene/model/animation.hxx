
// animation.hxx - classes to manage model animation.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef _SG_ANIMATION_HXX
#define _SG_ANIMATION_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/Texture2D>
#include <osgDB/ReaderWriter>

#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/structure/SGExpression.hxx>

// Has anyone done anything *really* stupid, like making min and max macros?
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

SGExpressiond*
read_value(const SGPropertyNode* configNode, SGPropertyNode* modelRoot,
           const char* unit, double defMin, double defMax);

SGVec3d readTranslateAxis(const SGPropertyNode* configNode);

/**
 * Base class for animation installers
 */
class SGAnimation : protected osg::NodeVisitor {
public:
  SGAnimation(const SGPropertyNode* configNode, SGPropertyNode* modelRoot);
  virtual ~SGAnimation();

  static bool animate(osg::Node* node, const SGPropertyNode* configNode,
                      SGPropertyNode* modelRoot,
                      const osgDB::Options* options,
                      const std::string &path, int i);

protected:
  void apply(osg::Node* node);

  virtual void install(osg::Node& node);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);

  virtual void apply(osg::Group& group);

  /**
   * Read a 3d vector from the configuration property node.
   *
   * Reads values from @a name/[xyz]@a prefix and defaults to the according
   * value of @a def for each value which is not set.
   *
   * @param name    Name of the root node containing all coordinates
   * @param suffix  Suffix appended to each child node (x,y,z)
   * @param def     Vector containing default values
   */
  SGVec3d readVec3( const SGPropertyNode& cfg,
                    const std::string& name,
                    const std::string& suffix = "",
                    const SGVec3d& def = SGVec3d::zeros() ) const;

  SGVec3d readVec3( const std::string& name,
                    const std::string& suffix = "",
                    const SGVec3d& def = SGVec3d::zeros() ) const;

  void readRotationCenterAndAxis(SGVec3d& center, SGVec3d& axis) const;

  SGExpressiond* readOffsetValue(const char* tag_name) const;

  void removeMode(osg::Node& node, osg::StateAttribute::GLMode mode);
  void removeAttribute(osg::Node& node, osg::StateAttribute::Type type);
  void removeTextureMode(osg::Node& node, unsigned unit,
                         osg::StateAttribute::GLMode mode);
  void removeTextureAttribute(osg::Node& node, unsigned unit,
                              osg::StateAttribute::Type type);
  void setRenderBinToInherit(osg::Node& node);
  void cloneDrawables(osg::Node& node);

  std::string getType() const
  { return std::string(_configNode->getStringValue("type", "")); }

  const SGPropertyNode* getConfig() const
  { return _configNode; }
  SGPropertyNode* getModelRoot() const
  { return _modelRoot; }

  const SGCondition* getCondition() const;

  std::list<std::string> _objectNames;
private:
  void installInGroup(const std::string& name, osg::Group& group,
                      osg::ref_ptr<osg::Group>& animationGroup);

  class RemoveModeVisitor;
  class RemoveAttributeVisitor;
  class RemoveTextureModeVisitor;
  class RemoveTextureAttributeVisitor;
  class BinToInheritVisitor;
  class DrawableCloneVisitor;

  bool _found;
  std::string _name;
  SGSharedPtr<SGPropertyNode const> _configNode;
  SGPropertyNode* _modelRoot;
  
  std::list<osg::ref_ptr<osg::Node> > _installedAnimations;
  bool _enableHOT;
};


//////////////////////////////////////////////////////////////////////
// Null animation installer
//////////////////////////////////////////////////////////////////////

class SGGroupAnimation : public SGAnimation {
public:
  SGGroupAnimation(const SGPropertyNode*, SGPropertyNode*);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
};


//////////////////////////////////////////////////////////////////////
// Translate animation installer
//////////////////////////////////////////////////////////////////////

class SGTranslateAnimation : public SGAnimation {
public:
  SGTranslateAnimation(const SGPropertyNode* configNode,
                       SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
private:
  class UpdateCallback;
  SGSharedPtr<const SGCondition> _condition;
  SGSharedPtr<const SGExpressiond> _animationValue;
  SGVec3d _axis;
  double _initialValue;
};


//////////////////////////////////////////////////////////////////////
// Rotate/Spin animation installer
//////////////////////////////////////////////////////////////////////

class SGRotateAnimation : public SGAnimation {
public:
  SGRotateAnimation(const SGPropertyNode* configNode,
                    SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
private:
  SGSharedPtr<const SGCondition> _condition;
  SGSharedPtr<const SGExpressiond> _animationValue;
  SGVec3d _axis;
  SGVec3d _center;
  double _initialValue;
  bool _isSpin;
};


//////////////////////////////////////////////////////////////////////
// Scale animation installer
//////////////////////////////////////////////////////////////////////

class SGScaleAnimation : public SGAnimation {
public:
  SGScaleAnimation(const SGPropertyNode* configNode,
                   SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
private:
  class UpdateCallback;
  SGSharedPtr<const SGCondition> _condition;
  SGSharedPtr<const SGExpressiond> _animationValue[3];
  SGVec3d _initialValue;
  SGVec3d _center;
};


//////////////////////////////////////////////////////////////////////
// dist scale animation installer
//////////////////////////////////////////////////////////////////////

class SGDistScaleAnimation : public SGAnimation {
public:
  SGDistScaleAnimation(const SGPropertyNode* configNode,
                       SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
  class Transform;
};


//////////////////////////////////////////////////////////////////////
// dist scale animation installer
//////////////////////////////////////////////////////////////////////

class SGFlashAnimation : public SGAnimation {
public:
  SGFlashAnimation(const SGPropertyNode* configNode,
                   SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
public:
  class Transform;
};


//////////////////////////////////////////////////////////////////////
// dist scale animation installer
//////////////////////////////////////////////////////////////////////

class SGBillboardAnimation : public SGAnimation {
public:
  SGBillboardAnimation(const SGPropertyNode* configNode,
                       SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
  class Transform;
};


//////////////////////////////////////////////////////////////////////
// Range animation installer
//////////////////////////////////////////////////////////////////////

class SGRangeAnimation : public SGAnimation {
public:
  SGRangeAnimation(const SGPropertyNode* configNode,
                   SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
private:
  class UpdateCallback;
  SGSharedPtr<const SGCondition> _condition;
  SGSharedPtr<const SGExpressiond> _minAnimationValue;
  SGSharedPtr<const SGExpressiond> _maxAnimationValue;
  SGVec2d _initialValue;
};


//////////////////////////////////////////////////////////////////////
// Select animation installer
//////////////////////////////////////////////////////////////////////

class SGSelectAnimation : public SGAnimation {
public:
  SGSelectAnimation(const SGPropertyNode* configNode,
                    SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
};


//////////////////////////////////////////////////////////////////////
// Alpha test animation installer
//////////////////////////////////////////////////////////////////////

class SGAlphaTestAnimation : public SGAnimation {
public:
  SGAlphaTestAnimation(const SGPropertyNode* configNode,
                       SGPropertyNode* modelRoot);
  virtual void install(osg::Node& node);
};


//////////////////////////////////////////////////////////////////////
// Blend animation installer
//////////////////////////////////////////////////////////////////////

class SGBlendAnimation : public SGAnimation {
public:
  SGBlendAnimation(const SGPropertyNode* configNode,
                   SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
  virtual void install(osg::Node& node);
private:
  class BlendVisitor;
  class UpdateCallback;
  SGSharedPtr<SGExpressiond> _animationValue;
};


//////////////////////////////////////////////////////////////////////
// Timed animation installer
//////////////////////////////////////////////////////////////////////

class SGTimedAnimation : public SGAnimation {
public:
  SGTimedAnimation(const SGPropertyNode* configNode,
                   SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
private:
  class UpdateCallback;
};


//////////////////////////////////////////////////////////////////////
// Shadow animation installer
//////////////////////////////////////////////////////////////////////

class SGShadowAnimation : public SGAnimation {
public:
  SGShadowAnimation(const SGPropertyNode* configNode,
                    SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
private:
  class UpdateCallback;
};


//////////////////////////////////////////////////////////////////////
// TextureTransform animation
//////////////////////////////////////////////////////////////////////

class SGTexTransformAnimation : public SGAnimation {
public:
  SGTexTransformAnimation(const SGPropertyNode* configNode,
                          SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
private:
  class Transform;
  class Translation;
  class Rotation;
  class Trapezoid;
  class UpdateCallback;

  SGExpressiond* readValue( const SGPropertyNode& cfg,
                            const std::string& suffix = "" );

  void appendTexTranslate( const SGPropertyNode& cfg,
                           UpdateCallback* updateCallback);
  void appendTexRotate(    const SGPropertyNode& cfg,
                           UpdateCallback* updateCallback);
  void appendTexTrapezoid( const SGPropertyNode& cfg,
                           UpdateCallback* updateCallback);
};


//////////////////////////////////////////////////////////////////////
// Shader animation
//////////////////////////////////////////////////////////////////////

class SGShaderAnimation : public SGAnimation {
public:
  SGShaderAnimation(const SGPropertyNode* configNode,
                    SGPropertyNode* modelRoot,
                    const osgDB::Options* options);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
private:
  class UpdateCallback;
  osg::ref_ptr<osg::Texture2D> _effect_texture;
};

//////////////////////////////////////////////////////////////////////
// Light animation
//////////////////////////////////////////////////////////////////////

class SGLightAnimation : public SGAnimation {
public:
  SGLightAnimation(const SGPropertyNode* configNode,
                   SGPropertyNode* modelRoot,
                   const osgDB::Options* options,
                   const std::string &path, int i);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
  virtual void install(osg::Node& node);
private:
  std::string _light_type;
  SGVec3d _position;
  SGVec3d _direction;
  SGVec4d _ambient;
  SGVec4d _diffuse;
  SGVec4d _specular;
  SGVec3d _attenuation;
  double _exponent;
  double _cutoff;
  double _near;
  double _far;
  std::string _key;
  class UpdateCallback;
  friend class UpdateCallback;
  SGSharedPtr<SGExpressiond> _animationValue;
  osg::ref_ptr<const osgDB::Options> _options;
};

#endif // _SG_ANIMATION_HXX
