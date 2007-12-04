
// animation.hxx - classes to manage model animation.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef _SG_ANIMATION_HXX
#define _SG_ANIMATION_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <vector>
#include <map>

#include <osg/Vec3>
#include <osg/Vec4>

#include <osg/ref_ptr>
#include <osg/AlphaFunc>
#include <osg/Group>
#include <osg/Material>
#include <osg/Node>
#include <osg/NodeCallback>
#include <osg/NodeVisitor>
#include <osg/StateSet>
#include <osg/Texture2D>
#include <osg/TexMat>

#include <osgDB/ReaderWriter>
#include <simgear/props/props.hxx>
#include <simgear/misc/sg_path.hxx>

#include <simgear/math/interpolater.hxx>
#include <simgear/scene/model/persparam.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>


SG_USING_STD(vector);
SG_USING_STD(map);

// Don't pull in the headers, since we don't need them here.
class SGInterpTable;
class SGCondition;

// Has anyone done anything *really* stupid, like making min and max macros?
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif



//////////////////////////////////////////////////////////////////////
// Helper classes, FIXME: factor out
//////////////////////////////////////////////////////////////////////

class SGDoubleValue : public SGReferenced {
public:
  virtual ~SGDoubleValue() {}
  virtual double getValue() const = 0;
};


//////////////////////////////////////////////////////////////////////
// Base class for animation installers
//////////////////////////////////////////////////////////////////////

class SGAnimation : protected osg::NodeVisitor {
public:
  SGAnimation(const SGPropertyNode* configNode, SGPropertyNode* modelRoot);
  virtual ~SGAnimation();

  static bool animate(osg::Node* node, const SGPropertyNode* configNode,
                      SGPropertyNode* modelRoot,
                      const osgDB::ReaderWriter::Options* options);

protected:
  void apply(osg::Node* node);

  virtual void install(osg::Node& node);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);

  virtual void apply(osg::Group& group);

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
  std::list<std::string> _objectNames;
  std::list<osg::ref_ptr<osg::Node> > _installedAnimations;
  bool _enableHOT;
  bool _disableShadow;
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
  SGSharedPtr<const SGDoubleValue> _animationValue;
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
  class UpdateCallback;
  class SpinUpdateCallback;
  SGSharedPtr<const SGCondition> _condition;
  SGSharedPtr<const SGDoubleValue> _animationValue;
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
  SGSharedPtr<const SGDoubleValue> _animationValue[3];
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
private:
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
private:
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
private:
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
  SGSharedPtr<const SGDoubleValue> _minAnimationValue;
  SGSharedPtr<const SGDoubleValue> _maxAnimationValue;
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
private:
  class UpdateCallback;
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
  SGSharedPtr<SGDoubleValue> _animationValue;
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
  class UpdateCallback;
  void appendTexTranslate(const SGPropertyNode* config,
                          UpdateCallback* updateCallback);
  void appendTexRotate(const SGPropertyNode* config,
                       UpdateCallback* updateCallback);
};


//////////////////////////////////////////////////////////////////////
// Shader animation
//////////////////////////////////////////////////////////////////////

class SGShaderAnimation : public SGAnimation {
public:
  SGShaderAnimation(const SGPropertyNode* configNode,
                    SGPropertyNode* modelRoot,
                    const osgDB::ReaderWriter::Options* options);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
private:
  class UpdateCallback;
  osg::ref_ptr<osg::Texture2D> _effect_texture;
};


//////////////////////////////////////////////////////////////////////
// Pick animation
//////////////////////////////////////////////////////////////////////

class SGPickAnimation : public SGAnimation {
public:
  SGPickAnimation(const SGPropertyNode* configNode,
                  SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
private:
  class PickCallback;
};

#endif // _SG_ANIMATION_HXX
