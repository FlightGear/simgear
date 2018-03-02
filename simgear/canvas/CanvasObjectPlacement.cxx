///@file
/// Canvas placement for placing a canvas texture onto osg objects
//
// It also provides a SGPickCallback for passing mouse events to the canvas and
// manages emissive lighting of the placed canvas.
//
// Copyright (C) 2013  Thomas Geymayer <tomgey@gmail.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#include <simgear_config.h>

#include "Canvas.hxx"
#include "CanvasObjectPlacement.hxx"
#include <simgear/canvas/events/MouseEvent.hxx>

#include <simgear/props/props.hxx>
#include <simgear/scene/util/SGPickCallback.hxx>

#include <osgGA/GUIEventAdapter>

namespace simgear
{
namespace canvas
{

  /**
   * Handle picking events on object with a canvas placed onto
   */
  class ObjectPickCallback:
    public SGPickCallback
  {
    public:

      ObjectPickCallback(const CanvasWeakPtr& canvas):
        _canvas(canvas)
      {}

      virtual bool needsUV() const { return true; }

      virtual bool buttonPressed( int,
                                  const osgGA::GUIEventAdapter& ea,
                                  const Info& info )
      {
        MouseEventPtr event(new MouseEvent(ea));
        updatePosFromUV(event, info.uv);

        if( ea.getEventType() == osgGA::GUIEventAdapter::SCROLL )
        {
          event->type = Event::WHEEL;
          event->delta.set(0,0);
          switch( ea.getScrollingMotion() )
          {
            case osgGA::GUIEventAdapter::SCROLL_UP:
              event->delta.y() = 1;
              break;
            case osgGA::GUIEventAdapter::SCROLL_DOWN:
              event->delta.y() = -1;
              break;
            default:
              return false;
          }
        }
        else
        {
          event->type = Event::MOUSE_DOWN;
        }

        return handleEvent(event);
      }

      virtual void buttonReleased( int,
                                   const osgGA::GUIEventAdapter& ea,
                                   const Info* info )
      {
        if( ea.getEventType() != osgGA::GUIEventAdapter::RELEASE )
          return;

        MouseEventPtr event(new MouseEvent(ea));
        event->type = Event::MOUSE_UP;
        updatePosFromUV(event, info ? info->uv : SGVec2d(-1,-1));

        handleEvent(event);
      }

      virtual void mouseMoved( const osgGA::GUIEventAdapter& ea,
                               const Info* info )
      {
        // drag (currently only with LMB)
        if( ea.getEventType() != osgGA::GUIEventAdapter::DRAG )
          return;

        MouseEventPtr event(new MouseEvent(ea));
        event->type = Event::DRAG;
        updatePosFromUV(event, info ? info->uv : SGVec2d(-1,-1));

        handleEvent(event);
      }

      virtual bool hover( const osg::Vec2d& windowPos,
                          const Info& info )
      {
        // TODO somehow get more info about event (time, modifiers, pressed
        // buttons, ...)
        MouseEventPtr event(new MouseEvent);
        event->type = Event::MOUSE_MOVE;
        event->screen_pos = windowPos;
        updatePosFromUV(event, info.uv);

        return handleEvent(event);
      }

      virtual void mouseLeave( const osg::Vec2d& windowPos )
      {
        MouseEventPtr event(new MouseEvent);
        event->type = Event::MOUSE_LEAVE;
        event->screen_pos = windowPos;

        handleEvent(event);
      }

    protected:
      CanvasWeakPtr _canvas;
      osg::Vec2f    _last_pos,
                    _last_delta;

      void updatePosFromUV(const MouseEventPtr& event, const SGVec2d& uv)
      {
        CanvasPtr canvas = _canvas.lock();
        if( !canvas )
          return;

        osg::Vec2d pos( uv.x() * canvas->getViewWidth(),
                        (1 - uv.y()) * canvas->getViewHeight() );

        _last_delta = pos - _last_pos;
        _last_pos = pos;

        event->client_pos = pos;
        event->delta = _last_delta;
      }

      bool handleEvent(const MouseEventPtr& event)
      {
        CanvasPtr canvas = _canvas.lock();
        if( !canvas )
          return false;

        return canvas->handleMouseEvent(event);
      }
  };

  //----------------------------------------------------------------------------
  ObjectPlacement::ObjectPlacement( SGPropertyNode* node,
                                    const GroupPtr& group,
                                    const CanvasWeakPtr& canvas ):
    Placement(node),
    _group(group),
    _canvas(canvas)
  {
    // TODO make more generic and extendable for more properties
    if( node->hasValue("emission") )
      setEmission( node->getFloatValue("emission") );
    if( node->hasValue("capture-events") )
      setCaptureEvents( node->getBoolValue("capture-events") );
  }

  //----------------------------------------------------------------------------
  ObjectPlacement::~ObjectPlacement()
  {
    assert( _group->getNumChildren() == 1 );
    osg::Node *child = _group->getChild(0);

    if( _group->getNumParents() )
    {
      osg::Group *parent = _group->getParent(0);
      parent->addChild(child);
      parent->removeChild(_group);
    }

    _group->removeChild(child);
  }

  //----------------------------------------------------------------------------
  void ObjectPlacement::setEmission(float emit)
  {
    emit = SGMiscf::clip(emit, 0, 1);

    if( !_material )
    {
      _material = new osg::Material;
      _material->setColorMode(osg::Material::OFF);
      _material->setDataVariance(osg::Object::DYNAMIC);
      _group->getOrCreateStateSet()
            ->setAttribute(_material, ( osg::StateAttribute::ON
                                      | osg::StateAttribute::OVERRIDE ) );
    }

    _material->setEmission(
      osg::Material::FRONT_AND_BACK,
      osg::Vec4(emit, emit, emit, emit)
    );
  }

  //----------------------------------------------------------------------------
  void ObjectPlacement::setCaptureEvents(bool enable)
  {
    if( !enable && _scene_user_data )
      return;

    if( enable && !_pick_cb )
      _pick_cb = new ObjectPickCallback(_canvas);

    _scene_user_data = SGSceneUserData::getOrCreateSceneUserData(_group);
    _scene_user_data->setPickCallback(enable ? _pick_cb.get() : 0);
  }

  //----------------------------------------------------------------------------
  bool ObjectPlacement::childChanged(SGPropertyNode* node)
  {
    if( node->getParent() != _node )
      return false;

    if( node->getNameString() == "emission" )
      setEmission( node->getFloatValue() );
    else if( node->getNameString() == "capture-events" )
      setCaptureEvents( node->getBoolValue() );
    else
      return false;

    return true;
  }

} // namespace canvas
} // namespace simgear
