/* -*-c++-*-
 *
 * Copyright (C) 2013 James Turner
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
     
     
#include <simgear/scene/model/SGPickAnimation.hxx>

#include <osg/Geode>
#include <osg/PolygonOffset>
#include <osg/PolygonMode>
#include <osg/Material>

#include <simgear/scene/util/SGPickCallback.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/structure/SGBinding.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/scene/model/SGRotateTransform.hxx>

using namespace simgear;

using OpenThreads::Mutex;
using OpenThreads::ScopedLock;

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
       
     fireBindingList(_bindingsDown);
     _repeatTime = -_repeatInterval;    // anti-bobble: delay start of repeat
     return true;
   }
   virtual void buttonReleased(void)
   {
       fireBindingList(_bindingsUp);
   }
   virtual void update(double dt)
   {
     if (!_repeatable)
       return;

     _repeatTime += dt;
     while (_repeatInterval < _repeatTime) {
       _repeatTime -= _repeatInterval;
         fireBindingList(_bindingsDown);
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
 OpenThreads::Mutex colorModeUniformMutex;
 osg::ref_ptr<osg::Uniform> colorModeUniform;
 }


void 
SGPickAnimation::innerSetupPickGroup(osg::Group* commonGroup, osg::Group& parent)
{
    // Contains the normal geometry that is interactive
    osg::ref_ptr<osg::Group> normalGroup = new osg::Group;
    normalGroup->setName("pick normal group");
    normalGroup->addChild(commonGroup);
    
    // Used to render the geometry with just yellow edges
    osg::Group* highlightGroup = new osg::Group;
    highlightGroup->setName("pick highlight group");
    highlightGroup->setNodeMask(simgear::PICK_BIT);
    highlightGroup->addChild(commonGroup);
    
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
}

 osg::Group*
 SGPickAnimation::createAnimationGroup(osg::Group& parent)
 {
     osg::Group* commonGroup = new osg::Group;
     innerSetupPickGroup(commonGroup, parent);
     SGSceneUserData* ud = SGSceneUserData::getOrCreateSceneUserData(commonGroup);

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

   return commonGroup;
}

///////////////////////////////////////////////////////////////////////////

class SGKnobAnimation::KnobPickCallback : public SGPickCallback {
public:
    KnobPickCallback(const SGPropertyNode* configNode,
                 SGPropertyNode* modelRoot) :
        _direction(DIRECTION_NONE),
        _repeatInterval(configNode->getDoubleValue("interval-sec", 0.1))
    {
        const SGPropertyNode* act = configNode->getChild("action");
        if (act)
            _action = readBindingList(act->getChildren("binding"), modelRoot);
        
        const SGPropertyNode* cw = configNode->getChild("cw");
        if (cw)
            _bindingsCW = readBindingList(cw->getChildren("binding"), modelRoot);
        
        const SGPropertyNode* ccw = configNode->getChild("ccw");
        if (ccw)
            _bindingsCCW = readBindingList(ccw->getChildren("binding"), modelRoot);
    }
    
    virtual bool buttonPressed(int button, const Info&)
    {
        _direction = DIRECTION_NONE;
        if ((button == 0) || (button == 4)) {
            _direction = DIRECTION_CLOCKWISE;
        } else if ((button == 1) || (button == 3)) {
            _direction = DIRECTION_ANTICLOCKWISE;
        } else {
            return false;
        }
        
        _repeatTime = -_repeatInterval;    // anti-bobble: delay start of repeat
        fire();
        return true;
    }
    
    virtual void buttonReleased(void)
    {
    }
    
    virtual void update(double dt)
    {
        _repeatTime += dt;
        while (_repeatInterval < _repeatTime) {
            _repeatTime -= _repeatInterval;
            fire();
        } // of repeat iteration
    }
private:
    void fire()
    {
        switch (_direction) {
            case DIRECTION_CLOCKWISE:
                fireBindingListWithOffset(_action,  1, 1);
                fireBindingList(_bindingsCW);
                break;
            case DIRECTION_ANTICLOCKWISE:
                fireBindingListWithOffset(_action, -1, 1);
                fireBindingList(_bindingsCCW);
                break;
            default: break;
        }
    }
    
    SGBindingList _action;
    SGBindingList _bindingsCW,
        _bindingsCCW;
    
    enum Direction
    {
        DIRECTION_NONE,
        DIRECTION_CLOCKWISE,
        DIRECTION_ANTICLOCKWISE
    };
    
    Direction _direction;
    double _repeatInterval;
    double _repeatTime;
};

class SGKnobAnimation::UpdateCallback : public osg::NodeCallback {
public:
    UpdateCallback(SGExpressiond const* animationValue) :
        _animationValue(animationValue)
    {
        setName("SGKnobAnimation::UpdateCallback");
    }
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        SGRotateTransform* transform = static_cast<SGRotateTransform*>(node);
        transform->setAngleDeg(_animationValue->getValue());
        traverse(node, nv);
    }
    
private:
    SGSharedPtr<SGExpressiond const> _animationValue;
};


SGKnobAnimation::SGKnobAnimation(const SGPropertyNode* configNode,
                                 SGPropertyNode* modelRoot) :
    SGPickAnimation(configNode, modelRoot)
{
    SGSharedPtr<SGExpressiond> value = read_value(configNode, modelRoot, "-deg",
                                                  -SGLimitsd::max(), SGLimitsd::max());
    _animationValue = value->simplify();
    
    
    readRotationCenterAndAxis(configNode, _center, _axis);
}


osg::Group*
SGKnobAnimation::createAnimationGroup(osg::Group& parent)
{
    SGRotateTransform* transform = new SGRotateTransform();
    innerSetupPickGroup(transform, parent);
    
    UpdateCallback* uc = new UpdateCallback(_animationValue);
    transform->setUpdateCallback(uc);
    transform->setCenter(_center);
    transform->setAxis(_axis);
        
    SGSceneUserData* ud = SGSceneUserData::getOrCreateSceneUserData(transform);
    ud->setPickCallback(new KnobPickCallback(getConfig(), getModelRoot()));
    
    return transform;
}

