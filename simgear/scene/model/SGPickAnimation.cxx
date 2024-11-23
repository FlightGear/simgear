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
     
 #include <simgear_config.h>

#include <simgear/scene/model/SGPickAnimation.hxx>

#include <algorithm>

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


osg::Vec2d eventToWindowCoords(const osgGA::GUIEventAdapter& ea)
{
    using namespace osg;
    const GraphicsContext* gc = ea.getGraphicsContext();
    const GraphicsContext::Traits* traits = gc->getTraits() ;
    // Scale x, y to the dimensions of the window
    double x = (((ea.getX() - ea.getXmin()) / (ea.getXmax() - ea.getXmin()))
         * (double)traits->width);
    double y = (((ea.getY() - ea.getYmin()) / (ea.getYmax() - ea.getYmin()))
         * (double)traits->height);
    if (ea.getMouseYOrientation() == osgGA::GUIEventAdapter::Y_INCREASING_DOWNWARDS)
        y = (double)traits->height - y;
    
    return osg::Vec2d(x, y);
}

 class SGPickAnimation::PickCallback : public SGPickCallback {
 public:
   PickCallback(const SGPropertyNode* configNode,
                SGPropertyNode* modelRoot,
                SGSharedPtr<SGCondition const> condition) :
     SGPickCallback(PriorityPanel),
     _condition(condition),
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
     
     
     if (configNode->hasChild("cursor")) {
       _cursorName = configNode->getStringValue("cursor");
     }
   }
     
     void addHoverBindings(const SGPropertyNode* hoverNode,
                             SGPropertyNode* modelRoot)
     {
         if (!_condition || _condition->test()) {
             _hover = readBindingList(hoverNode->getChildren("binding"), modelRoot);
         }
     }
     
   virtual bool buttonPressed( int button,
                               const osgGA::GUIEventAdapter&,
                               const Info& )
   {
       if (!_condition || _condition->test()) {
           if (_buttons.find(button) == _buttons.end()) {
               return false;
           }

           if (!anyBindingEnabled(_bindingsDown)) {
               return false;
           }

           fireBindingList(_bindingsDown);
           _repeatTime = -_repeatInterval;    // anti-bobble: delay start of repeat
           return true;
       }
       return false;
   }
   virtual void buttonReleased( int keyModState,
                                const osgGA::GUIEventAdapter&,
                                const Info* )
   {
       if (!_condition || _condition->test()) {
           SG_UNUSED(keyModState);
           fireBindingList(_bindingsUp);
       }
   }
     
   virtual void update(double dt, int keyModState)
   {
       SG_UNUSED(keyModState);
       if (_condition && !_condition->test()) {
           return;
       }

       if (!_repeatable)
           return;

       const bool zeroInterval = (_repeatInterval <= 0.0);
       if (zeroInterval) {
           // fire once per frame
           fireBindingList(_bindingsDown);
       } else {
           _repeatTime += dt;
           while (_repeatInterval < _repeatTime) {
               _repeatTime -= _repeatInterval;
               fireBindingList(_bindingsDown);
           }
       }
   }
   
   virtual bool hover( const osg::Vec2d& windowPos,
                       const Info& )
   {
       if (!_condition || _condition->test()) {
           if (!anyBindingEnabled(_hover)) {
               return false;
           }

           SGPropertyNode_ptr params(new SGPropertyNode);
           params->setDoubleValue("x", windowPos.x());
           params->setDoubleValue("y", windowPos.y());
           fireBindingList(_hover, params.ptr());
           return true;
       }
       return false;
   }
   
   std::string getCursor() const
   { return _cursorName; }
 private:
   SGSharedPtr<SGCondition const> _condition;
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
     SG_LOG(SG_IO, SG_DEBUG, "VncVisitor constructor "
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
       SG_LOG(SG_IO, SG_DEBUG, "VncVisitor image said " << _done
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
              osg::Group *node,
              SGSharedPtr<SGCondition const> condition)
     : _condition(condition)
     , _node(node)
 {
   SG_LOG(SG_IO, SG_DEBUG, "Configuring VNC callback");
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

 virtual bool buttonPressed( int button,
                             const osgGA::GUIEventAdapter&,
                             const Info& info )
 {
     if (!_condition || _condition->test()) {
         SGVec3d loc(info.local);
         SG_LOG(SG_IO, SG_DEBUG, "VNC pressed " << button << ": " << loc);
         loc -= _topLeft;
         _x = dot(loc, _toRight) / _squaredRight;
         _y = dot(loc, _toDown) / _squaredDown;
         if (_x < 0) _x = 0; else if (_x > 1) _x = 1;
         if (_y < 0) _y = 0; else if (_y > 1) _y = 1;
         VncVisitor vv(_x, _y, 1 << button);
         _node->accept(vv);
         return vv.wasSuccessful();
     }
     return false;
 }
 virtual void buttonReleased( int keyModState,
                              const osgGA::GUIEventAdapter&,
                              const Info* )
 {
     if (!_condition || _condition->test()) {
         SG_UNUSED(keyModState);
         SG_LOG(SG_IO, SG_DEBUG, "VNC release");
         VncVisitor vv(_x, _y, 0);
         _node->accept(vv);
     }
 }

private:
 SGSharedPtr<SGCondition const> _condition;

 double _x, _y;
 osg::ref_ptr<osg::Group> _node;
 SGVec3d _topLeft, _toRight, _toDown;
 double _squaredRight, _squaredDown;
};

///////////////////////////////////////////////////////////////////////////////

SGPickAnimation::SGPickAnimation(simgear::SGTransientModelData &modelData) :
    SGAnimation(modelData)
{
    _condition = getCondition();

  std::vector<SGPropertyNode_ptr> names = modelData.getConfigNode()->getChildren("proxy-name");
  for (unsigned i = 0; i < names.size(); ++i) {
    _proxyNames.push_back(names[i]->getStringValue());
  }
}

void SGPickAnimation::apply(osg::Node* node)
{
    SGAnimation::apply(node);
}

void
SGPickAnimation::apply(osg::Group& group)
{
  if (_objectNames.empty() && _proxyNames.empty()) {
    return;
  }

  group.traverse(*this);

  // Find whether this animation can be harmlessly repeated
  bool repeatable = isRepeatable();

  // iterate over all group children
  int i = group.getNumChildren() - 1;
  for (; 0 <= i; --i) {
    osg::Node* child = group.getChild(i);
    if (child->getName().empty()) {
        continue;
    }
      
    std::list<std::string>::iterator it = std::find(_objectNames.begin(), _objectNames.end(), child->getName());
    if (it != _objectNames.end()) {
      // Erasing child name from _objectNames should never have been commented
      // out. Now animations are installed multiple times if the objects they
      // refer to occur multiple times in the scene graph.
      if (repeatable) {
        // Repeating some animations is harmless, so we can stop doing it
        // without breaking compatibility.
        _objectNames.erase(it);
      } else {
        // Repeating other animations however multiplies their effect.
        // Detect if we've already installed the animation on this object name,
        // and warn loudly that this is deprecated behaviour that will change.
        auto it2 = _objectNamesHandled.find(child->getName());
        unsigned int* timesHandled;
        if (it2 != _objectNamesHandled.end())
          timesHandled = &(*it2).second;
        else
          timesHandled = &(_objectNamesHandled[child->getName()] = 0);
        if (++*timesHandled == 2) {
          // Second install, Search upwards for a node with multiple parents
          unsigned int foundMultiParents = 0;
          unsigned int numParents;
          for (osg::Node* cur = child; (numParents = cur->getNumParents());
               cur = cur->getParent(0)) {
            if (numParents > 1)
              ++foundMultiParents;
          }
          if (!foundMultiParents) {
            // Its inadvisable to have duplicated object names with animations
            SG_LOG(SG_GENERAL, SG_DEV_ALERT,
                   "Warning: " << getType() << " animation applies to multiple distinct objects named \"" << child->getName() << "\".");
          } else {
            static bool secondaryWarning = false;
            if (!secondaryWarning) {
              SG_LOG(SG_GENERAL, SG_DEV_ALERT, "Deprecation Alert: The knob/slider animations listed below are getting duplicated.");
              SG_LOG(SG_GENERAL, SG_DEV_ALERT, "    This will be fixed in a future version. Please apply workarounds to ensure compatibility.");
              SG_LOG(SG_GENERAL, SG_DEV_ALERT, "    For details see: https://wiki.flightgear.org/Knob_and_Slider_Duplication");
              secondaryWarning = true;
            }

            SG_LOG(SG_GENERAL, SG_DEV_ALERT, "Warning: " << (1 << foundMultiParents) << "x duplication of " << getType() << " animation on object \"" << child->getName() << "\" is deprecated behaviour (see above)");
            SG_LOG(SG_GENERAL, SG_DEV_ALERT, "    Animation node at: " << getConfig()->getLocation());

            // Search upwards for a node with multiple parents
            for (osg::Node* cur = child; (numParents = cur->getNumParents());
                 cur = cur->getParent(0)) {
              if (numParents > 1) {
                // This is the one, look for a parent with a location
                unsigned int i;
                for (i = 0; i < numParents; ++i) {
                  auto* userData = SGSceneUserData::getSceneUserData(cur->getParent(i));
                  if (userData && userData->getLocation().isValid()) {
                    SG_LOG(SG_GENERAL, SG_DEV_ALERT, "    Duplicated due to: " << userData->getLocation());
                    break;
                  }
                }
                if (i == numParents) {
                  SG_LOG(SG_GENERAL, SG_DEV_ALERT, "    Duplicated scene node '" << cur->getName() << "' at unknown location");
                  for (i = 0; i < numParents; ++i)
                    SG_LOG(SG_GENERAL, SG_DEV_ALERT, "        parent " << i << " named '" << cur->getParent(i)->getName() << "'");
                }
              }
            }
          }
        }
      }

      install(*child);
      
      osg::ref_ptr<osg::Group> renderGroup, pickGroup;      
      osg::Group* mainGroup = createMainGroup(&group);
      mainGroup->setName(child->getName());
      child->setName(""); // don't apply other animations twice
      
      if (getConfig()->getBoolValue("visible", true)) {
          renderGroup = new osg::Group;
          renderGroup->setName("pick render group");
          SGSceneUserData::getOrCreateSceneUserData(renderGroup)->setLocation(getConfig()->getLocation());
          renderGroup->addChild(child);
          mainGroup->addChild(renderGroup);
      }
      
      pickGroup = new osg::Group;
      pickGroup->setName("pick highlight group");
      SGSceneUserData::getOrCreateSceneUserData(pickGroup)->setLocation(getConfig()->getLocation());
      pickGroup->setNodeMask(simgear::PICK_BIT);
      mainGroup->addChild(pickGroup);
      
      setupCallbacks(SGSceneUserData::getOrCreateSceneUserData(mainGroup), mainGroup);

      pickGroup->addChild(child);
      group.removeChild(child);
      continue;
    }
    
    string_list::iterator j = std::find(_proxyNames.begin(), _proxyNames.end(), child->getName());
    if (j == _proxyNames.end()) {
      continue;
    }
    
    _proxyNames.erase(j);
    osg::ref_ptr<osg::Group> proxyGroup = new osg::Group;
    group.addChild(proxyGroup);
    proxyGroup->setNodeMask(simgear::PICK_BIT);
      
    setupCallbacks(SGSceneUserData::getOrCreateSceneUserData(proxyGroup), proxyGroup);
    proxyGroup->addChild(child);
    group.removeChild(child);
  } // of group children iteration
}

osg::Group*
SGPickAnimation::createMainGroup(osg::Group* pr)
{
  osg::Group* g = new osg::Group;
  pr->addChild(g);
  return g;
}

void
SGPickAnimation::setupCallbacks(SGSceneUserData* ud, osg::Group* parent)
{
  PickCallback* pickCb = NULL;
  
  // add actions that become macro and command invocations
  std::vector<SGPropertyNode_ptr> actions;
  actions = getConfig()->getChildren("action");
  for (unsigned int i = 0; i < actions.size(); ++i) {
    pickCb = new PickCallback(actions[i], getModelRoot(), _condition);
    ud->addPickCallback(pickCb);
  }
  
  if (getConfig()->hasChild("hovered")) {
    if (!pickCb) {
      // make a trivial PickCallback to hang the hovered off of
      SGPropertyNode_ptr dummyNode(new SGPropertyNode);
      pickCb = new PickCallback(dummyNode.ptr(), getModelRoot(), _condition);
      ud->addPickCallback(pickCb);
    }
    
    pickCb->addHoverBindings(getConfig()->getNode("hovered"), getModelRoot());
  }
  
  // Look for the VNC sessions that want raw mouse input
  actions = getConfig()->getChildren("vncaction");
  for (unsigned int i = 0; i < actions.size(); ++i) {
    ud->addPickCallback(new VncCallback(actions[i], getModelRoot(), parent, _condition));
  }
}

bool SGPickAnimation::isRepeatable() const
{
    // Pure pick handling doesn't move anything
    return true;
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
                 SGPropertyNode* modelRoot,
        SGSharedPtr<SGCondition const> condition) :
        SGPickCallback(PriorityPanel),
        _direction(DIRECTION_NONE),
        _repeatInterval(configNode->getDoubleValue("interval-sec", 0.1)),
        _dragDirection(DRAG_DEFAULT),
        _condition(condition)
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

    bool buttonPressed( int button, const osgGA::GUIEventAdapter& ea, const Info& ) override
    {
        if (!_condition || _condition->test()) {
            // the 'be nice to Mac / laptop' users option; alt-clicking spins the
            // opposite direction. Should make this configurable
            if ((button == 0) && (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT)) {
                button = 1;
            }

            const int increaseMouseWheel = static_knobMouseWheelAlternateDirection ? 4 : 3;
            const int decreaseMouseWheel = static_knobMouseWheelAlternateDirection ? 3 : 4;

            _direction = DIRECTION_NONE;
            if ((button == 0) || (button == increaseMouseWheel)) {
                _direction = DIRECTION_INCREASE;
            }
            else if ((button == 1) || (button == decreaseMouseWheel)) {
                _direction = DIRECTION_DECREASE;
            }
            else {
                return false;
            }

            _lastFirePos = eventToWindowCoords(ea);
            // delay start of repeat, makes dragging more usable
            _repeatTime = -_repeatInterval;
            _hasDragged = false;
            return true;
        }
        return false;
    }
    
    void buttonReleased( int keyModState, const osgGA::GUIEventAdapter&,
                                const Info* ) override
    {
        if (!_condition || _condition->test()) {

            // for *clicks*, we only fire on button release
            if (!_hasDragged) {
                fire(keyModState & osgGA::GUIEventAdapter::MODKEY_SHIFT, _direction);
            }

            fireBindingList(_releaseAction);
        }
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
  
    void mouseMoved( const osgGA::GUIEventAdapter& ea, const Info* ) override
    {
        if (!_condition || _condition->test()) {
            _mousePos = eventToWindowCoords(ea);
            osg::Vec2d deltaMouse = _mousePos - _lastFirePos;

            if (!_hasDragged) {

                double manhattanDist = deltaMouse.x() * deltaMouse.x() + deltaMouse.y() * deltaMouse.y();
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
                fire(ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT, dir);
                _lastFirePos = _mousePos;
            }
        }
    }
    
    void update(double dt, int keyModState) override
    {
        if (_hasDragged) {
            return;
        }

        const bool zeroInterval = (_repeatInterval <= 0.0);
        if (zeroInterval) {
            // fire once per frame
            fire(keyModState & osgGA::GUIEventAdapter::MODKEY_SHIFT, _direction);
        } else {
            _repeatTime += dt;
            while (_repeatInterval < _repeatTime) {
                _repeatTime -= _repeatInterval;
                fire(keyModState & osgGA::GUIEventAdapter::MODKEY_SHIFT, _direction);
            } // of repeat iteration
        }
    }

    bool hover( const osg::Vec2d& windowPos, const Info& ) override
    {
        if (!_condition || _condition->test()) {

            if (_hover.empty()) {
                return false;
            }

            SGPropertyNode_ptr params(new SGPropertyNode);
            params->setDoubleValue("x", windowPos.x());
            params->setDoubleValue("y", windowPos.y());
            fireBindingList(_hover, params.ptr());
            return true;
        }
        return false;
    }
  
    void setCursor(const std::string& aName)
    {
      _cursorName = aName;
    }
  
    std::string getCursor() const override
    { return _cursorName; }
  
private:
    void fire(bool isShifted, Direction dir)
    {
        if (!_condition || _condition->test()) {

            const SGBindingList& act(isShifted ? _shiftedAction : _action);
            const SGBindingList& incr(isShifted ? _shiftedIncrease : _bindingsIncrease);
            const SGBindingList& decr(isShifted ? _shiftedDecrease : _bindingsDecrease);

            switch (dir) {
            case DIRECTION_INCREASE:
                fireBindingListWithOffset(act, 1, 1);
                fireBindingList(incr);
                break;
            case DIRECTION_DECREASE:
                fireBindingListWithOffset(act, -1, 1);
                fireBindingList(decr);
                break;
            default: break;
            }
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
    SGSharedPtr<SGCondition const> _condition;

    bool _hasDragged; ///< has the mouse been dragged since the press?
    osg::Vec2d _mousePos, ///< current window coords location of the mouse
        _lastFirePos; ///< mouse location where we last fired the bindings
    double _dragScale;
  
    std::string _cursorName;
};

///////////////////////////////////////////////////////////////////////////////

class SGKnobAnimation::UpdateCallback : public osg::NodeCallback {
public:
    UpdateCallback(SGExpressiond const* animationValue, SGSharedPtr<SGCondition const> condition) :
        _condition(condition),
        _animationValue(animationValue)
    {
        setName("SGKnobAnimation::UpdateCallback");
    }
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (!_condition || _condition->test()) {
            SGRotateTransform* transform = static_cast<SGRotateTransform*>(node);
            transform->setAngleDeg(_animationValue->getValue());
            traverse(node, nv);
        }
    }
    
private:
    SGSharedPtr<SGCondition const> _condition;
    SGSharedPtr<SGExpressiond const> _animationValue;
};


SGKnobAnimation::SGKnobAnimation(simgear::SGTransientModelData &modelData) :
    SGPickAnimation(modelData)
{
    _condition = getCondition();
    SGSharedPtr<SGExpressiond> value = read_value(modelData.getConfigNode(), modelData.getModelRoot(), "-deg",
                                                  -SGLimitsd::max(), SGLimitsd::max());
    _animationValue = value->simplify();
    
    
    readRotationCenterAndAxis(modelData.getNode(), _center, _axis, modelData);
}

osg::Group*
SGKnobAnimation::createMainGroup(osg::Group* pr)
{  
  SGRotateTransform* transform = new SGRotateTransform();
  
  UpdateCallback* uc = new UpdateCallback(_animationValue, _condition);
  transform->setUpdateCallback(uc);
  transform->setCenter(_center);
  transform->setAxis(_axis);
  
  pr->addChild(transform);
  return transform;
}

void
SGKnobAnimation::setupCallbacks(SGSceneUserData* ud, osg::Group*)
{
  ud->setPickCallback(new KnobSliderPickCallback(getConfig(), getModelRoot(), _condition));
}

bool SGKnobAnimation::isRepeatable() const
{
    // For the animation to move anything, there must be an axis and a
    // non-const-zero animation value.
    return (!_axis.x() && !_axis.y() && !_axis.z()) ||
           (_animationValue->isConst() && _animationValue->getValue() == 0);
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


SGSliderAnimation::SGSliderAnimation(simgear::SGTransientModelData &modelData) :
    SGPickAnimation(modelData)
{
    SGSharedPtr<SGExpressiond> value = read_value(modelData.getConfigNode(), modelData.getModelRoot(), "-m",
                                                  -SGLimitsd::max(), SGLimitsd::max());
    _animationValue = value->simplify();
    SGVec3d garbage = SGVec3d::zeros();
    readRotationCenterAndAxis(modelData.getNode(), garbage, _axis, modelData);
}

osg::Group*
SGSliderAnimation::createMainGroup(osg::Group* pr)
{  
  SGTranslateTransform* transform = new SGTranslateTransform();
  
  UpdateCallback* uc = new UpdateCallback(_animationValue);
  transform->setUpdateCallback(uc);
  transform->setAxis(_axis);
  
  pr->addChild(transform);
  return transform;
}

void
SGSliderAnimation::setupCallbacks(SGSceneUserData* ud, osg::Group*)
{
  ud->setPickCallback(new KnobSliderPickCallback(getConfig(), getModelRoot(), _condition));
}

bool SGSliderAnimation::isRepeatable() const
{
    // For the animation to move anything, there must be an axis and a
    // non-const-zero animation value.
    return (!_axis.x() && !_axis.y() && !_axis.z()) ||
           (_animationValue->isConst() && _animationValue->getValue() == 0);
}

/*
 * touch screen is a 2d surface that will pass parameters to the callbacks indicating the 
 * normalized coordinates of hover or touch. Touch is defined as a button click. 
 * For compatibility with touchscreen operations this class does not differentiate between
 * which buttons are touched, simply because this isn't how touchscreens work.
 * Some touchscreens (e.g. SAW) can have a Z-axis indicating the pressure. This is not
 * simulated.
 */


/**
* Handle picking events on object with a canvas placed onto
*/
class TouchPickCallback : public SGPickCallback {
    public:
        TouchPickCallback(const SGPropertyNode* configNode,
            SGPropertyNode* modelRoot) :
            SGPickCallback(PriorityPanel),
            _repeatable(configNode->getBoolValue("repeatable", false)),
            _repeatInterval(configNode->getDoubleValue("interval-sec", 0.1))
        {
            std::vector<SGPropertyNode_ptr> bindings;

            bindings = configNode->getChildren("touch");
            for (unsigned int i = 0; i < bindings.size(); ++i) {
                _touches.insert(bindings[i]->getIntValue());
            }

            _bindingsTouched = readBindingList(configNode->getChildren("binding"), modelRoot);
            readOptionalBindingList(configNode, modelRoot, "mod-up", _bindingsReleased);

            if (configNode->hasChild("cursor")) {
                _cursorName = configNode->getStringValue("cursor");
            }
        }

        void addHoverBindings(const SGPropertyNode* hoverNode,
            SGPropertyNode* modelRoot)
        {
            _hover = readBindingList(hoverNode->getChildren("binding"), modelRoot);
        }

        bool buttonPressed(int touchIdx,
            const osgGA::GUIEventAdapter& event,
            const Info& info) override
        {
            if (_touches.find(touchIdx) == _touches.end()) {
                return false;
            }

            if (!anyBindingEnabled(_bindingsTouched)) {
                return false;
            }
            SGPropertyNode_ptr params(new SGPropertyNode); 
            params->setDoubleValue("x", info.uv[0]);
            params->setDoubleValue("y", info.uv[1]);

            _repeatTime = -_repeatInterval;    // anti-bobble: delay start of repeat
            fireBindingList(_bindingsTouched, params.ptr());
            return true;
        }
        
        void buttonReleased(int keyModState,
            const osgGA::GUIEventAdapter&,
            const Info* info) override
        {
            SG_UNUSED(keyModState);
            SGPropertyNode_ptr params(new SGPropertyNode);
            if (info) {
                params->setDoubleValue("x", info->uv[0]);
                params->setDoubleValue("y", info->uv[1]);
            }
            fireBindingList(_bindingsReleased, params.ptr());
        }

        void update(double dt, int keyModState) override
        {
            SG_UNUSED(keyModState);
            if (!_repeatable)
                return;

            _repeatTime += dt;
            while (_repeatInterval < _repeatTime) {
                _repeatTime -= _repeatInterval;
                fireBindingList(_bindingsTouched);
            }
        }

        bool hover(const osg::Vec2d& windowPos,
            const Info& info) override
        {
            if (!anyBindingEnabled(_hover)) {
                return false;
            }

            SGPropertyNode_ptr params(new SGPropertyNode);
            params->setDoubleValue("x", info.uv[0]);
            params->setDoubleValue("y", info.uv[1]);
            fireBindingList(_hover, params.ptr());
            return true;
        }

        std::string getCursor() const override
        {
            return _cursorName;
        }
        
        bool needsUV() const override { return true; }

    private:
        SGBindingList _bindingsTouched;
        SGBindingList _bindingsReleased;
        SGBindingList _hover;
        std::set<int> _touches;
        std::string _cursorName;
        bool _repeatable;
        double _repeatInterval;
        double _repeatTime;
};

SGTouchAnimation::SGTouchAnimation(simgear::SGTransientModelData &modelData) :
    SGPickAnimation(modelData)
{
}

osg::Group* SGTouchAnimation::createMainGroup(osg::Group* pr)
{
    SGRotateTransform* transform = new SGRotateTransform();
    pr->addChild(transform);
    return transform;
}

void SGTouchAnimation::setupCallbacks(SGSceneUserData* ud, osg::Group*)
{
    TouchPickCallback* touchCb = nullptr;

    // add actions that become macro and command invocations
    std::vector<SGPropertyNode_ptr> actions;
    actions = getConfig()->getChildren("action");
    for (unsigned int i = 0; i < actions.size(); ++i) {
        touchCb = new TouchPickCallback(actions[i], getModelRoot());
        ud->addPickCallback(touchCb);
    }

    if (getConfig()->hasChild("hovered")) {
        if (!touchCb) {
            // make a trivial PickCallback to hang the hovered off of
            SGPropertyNode_ptr dummyNode(new SGPropertyNode);
            touchCb = new TouchPickCallback(dummyNode.ptr(), getModelRoot());
            ud->addPickCallback(touchCb);
        }

        touchCb->addHoverBindings(getConfig()->getNode("hovered"), getModelRoot());
    }
//    ud->setPickCallback(new TouchPickCallback(getConfig(), getModelRoot()));
}
