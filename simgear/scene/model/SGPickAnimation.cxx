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
#include <osgGA/GUIEventAdapter>

#include <simgear/sg_inlines.h>
#include <simgear/scene/util/SGPickCallback.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/structure/SGBinding.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/scene/model/SGRotateTransform.hxx>
#include <simgear/scene/model/SGTranslateTransform.hxx>

using namespace simgear;

using OpenThreads::Mutex;
using OpenThreads::ScopedLock;

static void readOptionalBindingList(const SGPropertyNode* aNode, SGPropertyNode* modelRoot,
    const std::string& aName, SGBindingList& aBindings)
{
    const SGPropertyNode* n = aNode->getChild(aName);
    if (n)
        aBindings = readBindingList(n->getChildren("binding"), modelRoot);
    
}


osg::Vec2d eventToWindowCoords(const osgGA::GUIEventAdapter* ea)
{
    using namespace osg;
    const GraphicsContext* gc = ea->getGraphicsContext();
    const GraphicsContext::Traits* traits = gc->getTraits() ;
    // Scale x, y to the dimensions of the window
    double x = (((ea->getX() - ea->getXmin()) / (ea->getXmax() - ea->getXmin()))
         * (double)traits->width);
    double y = (((ea->getY() - ea->getYmin()) / (ea->getYmax() - ea->getYmin()))
         * (double)traits->height);
    if (ea->getMouseYOrientation() == osgGA::GUIEventAdapter::Y_INCREASING_DOWNWARDS)
        y = (double)traits->height - y;
    
    return osg::Vec2d(x, y);
}

 class SGPickAnimation::PickCallback : public SGPickCallback {
 public:
   PickCallback(const SGPropertyNode* configNode,
                SGPropertyNode* modelRoot) :
     SGPickCallback(PriorityPanel),
     _repeatable(configNode->getBoolValue("repeatable", false)),
     _repeatInterval(configNode->getDoubleValue("interval-sec", 0.1))
   {
     std::vector<SGPropertyNode_ptr> bindings;

     bindings = configNode->getChildren("button");
     for (unsigned int i = 0; i < bindings.size(); ++i) {
       _buttons.insert( bindings[i]->getIntValue() );
     }

     _bindingsDown = readBindingList(configNode->getChildren("binding"), modelRoot);
     readOptionalBindingList(configNode, modelRoot, "mod-up", _bindingsUp);
     readOptionalBindingList(configNode, modelRoot, "hovered", _hover);
     
     if (configNode->hasChild("cursor")) {
       _cursorName = configNode->getStringValue("cursor");
     }
   }
   
   virtual bool buttonPressed(int button, const osgGA::GUIEventAdapter* ea, const Info&)
   {
       if (_buttons.find(button) == _buttons.end()) {
           return false;
       }
       
     fireBindingList(_bindingsDown);
     _repeatTime = -_repeatInterval;    // anti-bobble: delay start of repeat
     return true;
   }
   virtual void buttonReleased(int keyModState)
   {
       SG_UNUSED(keyModState);
       fireBindingList(_bindingsUp);
   }
     
   virtual void update(double dt, int keyModState)
   {
     SG_UNUSED(keyModState);
     if (!_repeatable)
       return;

     _repeatTime += dt;
     while (_repeatInterval < _repeatTime) {
       _repeatTime -= _repeatInterval;
         fireBindingList(_bindingsDown);
     }
   }
   
   virtual bool hover(const osg::Vec2d& windowPos, const Info& info)
   {
       if (_hover.empty()) {
           return false;
       }
       
       SGPropertyNode_ptr params(new SGPropertyNode);
       params->setDoubleValue("x", windowPos.x());
       params->setDoubleValue("y", windowPos.y());
       fireBindingList(_hover, params.ptr());
       return true;
   }
   
   std::string getCursor() const
   { return _cursorName; }
 private:
   SGBindingList _bindingsDown;
   SGBindingList _bindingsUp;
   SGBindingList _hover;
   std::set<int> _buttons;
   bool _repeatable;
   double _repeatInterval;
   double _repeatTime;
   std::string _cursorName;
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

///////////////////////////////////////////////////////////////////////////////

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

   virtual bool buttonPressed(int button, const osgGA::GUIEventAdapter* ea, const Info& info)
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
   virtual void buttonReleased(int keyModState)
   {
     SG_UNUSED(keyModState);
     SG_LOG(SG_INPUT, SG_DEBUG, "VNC release");
     VncVisitor vv(_x, _y, 0);
     _node->accept(vv);
   }

 private:
   double _x, _y;
   osg::ref_ptr<osg::Group> _node;
   SGVec3d _topLeft, _toRight, _toDown;
   double _squaredRight, _squaredDown;
 };

///////////////////////////////////////////////////////////////////////////////

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

// insert count copies of binding list A, into the output list.
// effectively makes the output list fire binding A multiple times
// in sequence
static void repeatBindings(const SGBindingList& a, SGBindingList& out, int count)
{
    out.clear();
    for (int i=0; i<count; ++i) {
        out.insert(out.end(), a.begin(), a.end());
    }
}

static bool static_knobMouseWheelAlternateDirection = false;
static bool static_knobDragAlternateAxis = false;
static double static_dragSensitivity = 1.0;

class KnobSliderPickCallback : public SGPickCallback {
public:
    enum Direction
    {
        DIRECTION_NONE,
        DIRECTION_INCREASE,
        DIRECTION_DECREASE
    };
    
    enum DragDirection
    {
        DRAG_DEFAULT = 0,
        DRAG_VERTICAL,
        DRAG_HORIZONTAL
    };

    
    KnobSliderPickCallback(const SGPropertyNode* configNode,
                 SGPropertyNode* modelRoot) :
        SGPickCallback(PriorityPanel),
        _direction(DIRECTION_NONE),
        _repeatInterval(configNode->getDoubleValue("interval-sec", 0.1)),
        _dragDirection(DRAG_DEFAULT)
    {
        readOptionalBindingList(configNode, modelRoot, "action", _action);
        readOptionalBindingList(configNode, modelRoot, "increase", _bindingsIncrease);
        readOptionalBindingList(configNode, modelRoot, "decrease", _bindingsDecrease);
        
        readOptionalBindingList(configNode, modelRoot, "release", _releaseAction);
        readOptionalBindingList(configNode, modelRoot, "hovered", _hover);
        
        if (configNode->hasChild("shift-action") || configNode->hasChild("shift-increase") ||
            configNode->hasChild("shift-decrease"))
        {
        // explicit shifted behaviour - just do exactly what was provided
            readOptionalBindingList(configNode, modelRoot, "shift-action", _shiftedAction);
            readOptionalBindingList(configNode, modelRoot, "shift-increase", _shiftedIncrease);
            readOptionalBindingList(configNode, modelRoot, "shift-decrease", _shiftedDecrease);
        } else {
            // default shifted behaviour - repeat normal bindings N times.
            int shiftRepeat = configNode->getIntValue("shift-repeat", 10);
            repeatBindings(_action, _shiftedAction, shiftRepeat);
            repeatBindings(_bindingsIncrease, _shiftedIncrease, shiftRepeat);
            repeatBindings(_bindingsDecrease, _shiftedDecrease, shiftRepeat);
        } // of default shifted behaviour
        
        _dragScale = configNode->getDoubleValue("drag-scale-px", 10.0);
        std::string dragDir = configNode->getStringValue("drag-direction");
        if (dragDir == "vertical") {
            _dragDirection = DRAG_VERTICAL;
        } else if (dragDir == "horizontal") {
            _dragDirection = DRAG_HORIZONTAL;
        }
      
        if (configNode->hasChild("cursor")) {
            _cursorName = configNode->getStringValue("cursor");
        } else {
          DragDirection dir = effectiveDragDirection();
          if (dir == DRAG_VERTICAL) {
            _cursorName = "drag-vertical";
          } else if (dir == DRAG_HORIZONTAL) {
            _cursorName = "drag-horizontal";
          }
        }
    }
    
    virtual bool buttonPressed(int button, const osgGA::GUIEventAdapter* ea, const Info&)
    {        
        // the 'be nice to Mac / laptop' users option; alt-clicking spins the
        // opposite direction. Should make this configurable
        if ((button == 0) && (ea->getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT)) {
            button = 1;
        }
        
        int increaseMouseWheel = static_knobMouseWheelAlternateDirection ? 3 : 4;
        int decreaseMouseWheel = static_knobMouseWheelAlternateDirection ? 4 : 3;
            
        _direction = DIRECTION_NONE;
        if ((button == 0) || (button == increaseMouseWheel)) {
            _direction = DIRECTION_INCREASE;
        } else if ((button == 1) || (button == decreaseMouseWheel)) {
            _direction = DIRECTION_DECREASE;
        } else {
            return false;
        }
        
        _lastFirePos = eventToWindowCoords(ea);
    // delay start of repeat, makes dragging more usable
        _repeatTime = -_repeatInterval;    
        _hasDragged = false;
        return true;
    }
    
    virtual void buttonReleased(int keyModState)
    {
        // for *clicks*, we only fire on button release
        if (!_hasDragged) {
            fire(keyModState & osgGA::GUIEventAdapter::MODKEY_SHIFT, _direction);
        }
        
        fireBindingList(_releaseAction);
    }
  
    DragDirection effectiveDragDirection() const
    {
      if (_dragDirection == DRAG_DEFAULT) {
        // respect the current default settings - this allows runtime
        // setting of the default drag direction.
        return static_knobDragAlternateAxis ? DRAG_VERTICAL : DRAG_HORIZONTAL;
      }
      
      return _dragDirection;
  }
  
    virtual void mouseMoved(const osgGA::GUIEventAdapter* ea)
    {
        _mousePos = eventToWindowCoords(ea);
        osg::Vec2d deltaMouse = _mousePos - _lastFirePos;
        
        if (!_hasDragged) {
            
            double manhattanDist = deltaMouse.x() * deltaMouse.x()  + deltaMouse.y() * deltaMouse.y();
            if (manhattanDist < 5) {
                // don't do anything, just input noise
                return;
            }
            
        // user is dragging, disable repeat behaviour
            _hasDragged = true;
        }
      
        double delta = (effectiveDragDirection() == DRAG_VERTICAL) ? deltaMouse.y() : deltaMouse.x();
    // per-animation scale factor lets the aircraft author tune for expectations,
    // eg heading setting vs 5-state switch.
    // then we scale by a global sensitivity, which the user can set.
        delta *= static_dragSensitivity / _dragScale;
        
        if (fabs(delta) >= 1.0) {
            // determine direction from sign of delta
            Direction dir = (delta > 0.0) ? DIRECTION_INCREASE : DIRECTION_DECREASE;
            fire(ea->getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT, dir);
            _lastFirePos = _mousePos;
        }
    }
    
    virtual void update(double dt, int keyModState)
    {
        if (_hasDragged) {
            return;
        }
        
        _repeatTime += dt;
        while (_repeatInterval < _repeatTime) {
            _repeatTime -= _repeatInterval;
            fire(keyModState & osgGA::GUIEventAdapter::MODKEY_SHIFT, _direction);
        } // of repeat iteration
    }

    virtual bool hover(const osg::Vec2d& windowPos, const Info& info)
    {
        if (_hover.empty()) {
            return false;
        }
       
        SGPropertyNode_ptr params(new SGPropertyNode);
        params->setDoubleValue("x", windowPos.x());
        params->setDoubleValue("y", windowPos.y());
        fireBindingList(_hover, params.ptr());
        return true;
    }
  
    void setCursor(const std::string& aName)
    {
      _cursorName = aName;
    }
  
    virtual std::string getCursor() const
    { return _cursorName; }
  
private:
    void fire(bool isShifted, Direction dir)
    {
        const SGBindingList& act(isShifted ? _shiftedAction : _action);
        const SGBindingList& incr(isShifted ? _shiftedIncrease : _bindingsIncrease);
        const SGBindingList& decr(isShifted ? _shiftedDecrease : _bindingsDecrease);
        
        switch (dir) {
            case DIRECTION_INCREASE:
                fireBindingListWithOffset(act,  1, 1);
                fireBindingList(incr);
                break;
            case DIRECTION_DECREASE:
                fireBindingListWithOffset(act, -1, 1);
                fireBindingList(decr);
                break;
            default: break;
        }
    }
    
    SGBindingList _action, _shiftedAction;
    SGBindingList _releaseAction;
    SGBindingList _bindingsIncrease, _shiftedIncrease,
        _bindingsDecrease, _shiftedDecrease;
    SGBindingList _hover;
    
        
    Direction _direction;
    double _repeatInterval;
    double _repeatTime;
    
    DragDirection _dragDirection;
    bool _hasDragged; ///< has the mouse been dragged since the press?
    osg::Vec2d _mousePos, ///< current window coords location of the mouse
        _lastFirePos; ///< mouse location where we last fired the bindings
    double _dragScale;
  
    std::string _cursorName;
};

///////////////////////////////////////////////////////////////////////////////

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
    ud->setPickCallback(new KnobSliderPickCallback(getConfig(), getModelRoot()));
    
    return transform;
}

void SGKnobAnimation::setAlternateMouseWheelDirection(bool aToggle)
{
    static_knobMouseWheelAlternateDirection = aToggle;
}

void SGKnobAnimation::setAlternateDragAxis(bool aToggle)
{
    static_knobDragAlternateAxis = aToggle;
}

void SGKnobAnimation::setDragSensitivity(double aFactor)
{
    static_dragSensitivity = aFactor;
}

///////////////////////////////////////////////////////////////////////////////

class SGSliderAnimation::UpdateCallback : public osg::NodeCallback {
public:
    UpdateCallback(SGExpressiond const* animationValue) :
    _animationValue(animationValue)
    {
        setName("SGSliderAnimation::UpdateCallback");
    }
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        SGTranslateTransform* transform = static_cast<SGTranslateTransform*>(node);
        transform->setValue(_animationValue->getValue());

        traverse(node, nv);
    }
    
private:
    SGSharedPtr<SGExpressiond const> _animationValue;
};


SGSliderAnimation::SGSliderAnimation(const SGPropertyNode* configNode,
                                 SGPropertyNode* modelRoot) :
    SGPickAnimation(configNode, modelRoot)
{
    SGSharedPtr<SGExpressiond> value = read_value(configNode, modelRoot, "-m",
                                                  -SGLimitsd::max(), SGLimitsd::max());
    _animationValue = value->simplify();
    
    _axis = readTranslateAxis(configNode);
}

osg::Group*
SGSliderAnimation::createAnimationGroup(osg::Group& parent)
{
    SGTranslateTransform* transform = new SGTranslateTransform();
    innerSetupPickGroup(transform, parent);
    
    UpdateCallback* uc = new UpdateCallback(_animationValue);
    transform->setUpdateCallback(uc);
    transform->setAxis(_axis);
    
    SGSceneUserData* ud = SGSceneUserData::getOrCreateSceneUserData(transform);
    ud->setPickCallback(new KnobSliderPickCallback(getConfig(), getModelRoot()));
    
    return transform;
}
