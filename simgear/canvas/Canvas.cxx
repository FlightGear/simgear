// The canvas for rendering with the 2d API
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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

#include "Canvas.hxx"
#include "CanvasEventManager.hxx"
#include "CanvasEventVisitor.hxx"
#include <simgear/canvas/MouseEvent.hxx>
#include <simgear/canvas/CanvasPlacement.hxx>
#include <simgear/scene/util/parse_color.hxx>
#include <simgear/scene/util/RenderConstants.hxx>

#include <osg/Camera>
#include <osg/Geode>
#include <osgText/Text>
#include <osgViewer/Viewer>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  Canvas::CullCallback::CullCallback(const CanvasWeakPtr& canvas):
    _canvas( canvas )
  {

  }

  //----------------------------------------------------------------------------
  void Canvas::CullCallback::operator()( osg::Node* node,
                                         osg::NodeVisitor* nv )
  {
    if( (nv->getTraversalMask() & simgear::MODEL_BIT) && !_canvas.expired() )
      _canvas.lock()->enableRendering();

    traverse(node, nv);
  }

  //----------------------------------------------------------------------------
  Canvas::Canvas(SGPropertyNode* node):
    PropertyBasedElement(node),
    _canvas_mgr(0),
    _event_manager(new EventManager),
    _size_x(-1),
    _size_y(-1),
    _view_width(-1),
    _view_height(-1),
    _status(node, "status"),
    _status_msg(node, "status-msg"),
    _sampling_dirty(false),
    _render_dirty(true),
    _visible(true),
    _render_always(false)
  {
    _status = 0;
    setStatusFlags(MISSING_SIZE_X | MISSING_SIZE_Y);
  }

  //----------------------------------------------------------------------------
  void Canvas::onDestroy()
  {
    if( _root_group )
    {
      _root_group->clearEventListener();
      _root_group->onDestroy();
    }
  }

  //----------------------------------------------------------------------------
  void Canvas::setSystemAdapter(const SystemAdapterPtr& system_adapter)
  {
    _system_adapter = system_adapter;
    _texture.setSystemAdapter(system_adapter);
  }

  //----------------------------------------------------------------------------
  SystemAdapterPtr Canvas::getSystemAdapter() const
  {
    return _system_adapter;
  }

  //----------------------------------------------------------------------------
  void Canvas::setCanvasMgr(CanvasMgr* canvas_mgr)
  {
    _canvas_mgr = canvas_mgr;
  }

  //----------------------------------------------------------------------------
  CanvasMgr* Canvas::getCanvasMgr() const
  {
    return _canvas_mgr;
  }

  //----------------------------------------------------------------------------
  bool Canvas::isInit() const
  {
    return _texture.serviceable();
  }

  //----------------------------------------------------------------------------
  void Canvas::addParentCanvas(const CanvasWeakPtr& canvas)
  {
    if( canvas.expired() )
    {
      SG_LOG
      (
        SG_GENERAL,
        SG_WARN,
        "Canvas::addParentCanvas(" << _node->getPath(true) << "): "
        "got an expired parent!"
      );
      return;
    }

    _parent_canvases.insert(canvas);
  }

  //----------------------------------------------------------------------------
  void Canvas::addChildCanvas(const CanvasWeakPtr& canvas)
  {
    if( canvas.expired() )
    {
      SG_LOG
      (
        SG_GENERAL,
        SG_WARN,
        "Canvas::addChildCanvas(" << _node->getPath(true) << "): "
        " got an expired child!"
      );
      return;
    }

    _child_canvases.insert(canvas);
  }

  //----------------------------------------------------------------------------
  void Canvas::removeParentCanvas(const CanvasWeakPtr& canvas)
  {
    _parent_canvases.erase(canvas);
  }

  //----------------------------------------------------------------------------
  void Canvas::removeChildCanvas(const CanvasWeakPtr& canvas)
  {
    _child_canvases.erase(canvas);
  }

  //----------------------------------------------------------------------------
  GroupPtr Canvas::createGroup(const std::string& name)
  {
    return _root_group->createChild<Group>(name);
  }

  //----------------------------------------------------------------------------
  GroupPtr Canvas::getGroup(const std::string& name)
  {
    return _root_group->getChild<Group>(name);
  }

  //----------------------------------------------------------------------------
  GroupPtr Canvas::getOrCreateGroup(const std::string& name)
  {
    return _root_group->getOrCreateChild<Group>(name);
  }

  //----------------------------------------------------------------------------
  GroupPtr Canvas::getRootGroup()
  {
    return _root_group;
  }

  //----------------------------------------------------------------------------
  void Canvas::enableRendering(bool force)
  {
    _visible = true;
    if( force )
      _render_dirty = true;
  }

  //----------------------------------------------------------------------------
  void Canvas::update(double delta_time_sec)
  {
    if(    (!_texture.serviceable() && _status != STATUS_DIRTY)
        || (_status & CREATE_FAILED) )
      return;

    if( _status == STATUS_DIRTY )
    {
      _texture.setSize(_size_x, _size_y);

      if( !_texture.serviceable() )
      {
        _texture.useImageCoords(true);
        _texture.useStencil(true);
        _texture.allocRT(/*_camera_callback*/);
      }
      else
      {
        // Resizing causes a new texture to be created so we need to reapply all
        // existing placements
        reloadPlacements();
      }

      osg::Camera* camera = _texture.getCamera();

      osg::Vec4 clear_color(0.0f, 0.0f, 0.0f , 1.0f);
      parseColor(_node->getStringValue("background"), clear_color);
      camera->setClearColor(clear_color);

      camera->addChild(_root_group->getMatrixTransform());

      // Ensure objects are drawn in order of traversal
      camera->getOrCreateStateSet()->setBinName("TraversalOrderBin");

      if( _texture.serviceable() )
      {
        setStatusFlags(STATUS_OK);
        setStatusFlags(STATUS_DIRTY, false);
        _render_dirty = true;
      }
      else
      {
        setStatusFlags(CREATE_FAILED);
        return;
      }
    }

    if( _visible || _render_always )
    {
      BOOST_FOREACH(CanvasWeakPtr canvas, _child_canvases)
      {
        // TODO should we check if the image the child canvas is displayed
        //      within is really visible?
        if( !canvas.expired() )
          canvas.lock()->_visible = true;
      }

      if( _render_dirty )
      {
        // Also mark all canvases this canvas is displayed within as dirty
        BOOST_FOREACH(CanvasWeakPtr canvas, _parent_canvases)
        {
          if( !canvas.expired() )
            canvas.lock()->_render_dirty = true;
        }
      }

      _texture.setRender(_render_dirty);

      _render_dirty = false;
      _visible = false;
    }
    else
      _texture.setRender(false);

    _root_group->update(delta_time_sec);

    if( _sampling_dirty )
    {
      _texture.setSampling(
        _node->getBoolValue("mipmapping"),
        _node->getIntValue("coverage-samples"),
        _node->getIntValue("color-samples")
      );
      _sampling_dirty = false;
      _render_dirty = true;
    }

    while( !_dirty_placements.empty() )
    {
      SGPropertyNode *node = _dirty_placements.back();
      _dirty_placements.pop_back();

      if( node->getIndex() >= static_cast<int>(_placements.size()) )
        // New placement
        _placements.resize(node->getIndex() + 1);
      else
        // Remove possibly existing placements
        _placements[ node->getIndex() ].clear();

      // Get new placements
      PlacementFactoryMap::const_iterator placement_factory =
        _placement_factories.find( node->getStringValue("type", "object") );
      if( placement_factory != _placement_factories.end() )
      {
        Placements& placements = _placements[ node->getIndex() ] =
          placement_factory->second
          (
            node,
            boost::static_pointer_cast<Canvas>(_self.lock())
          );
        node->setStringValue
        (
          "status-msg",
          placements.empty() ? "No match" : "Ok"
        );
      }
      else
        node->setStringValue("status-msg", "Unknown placement type");
    }
  }

  //----------------------------------------------------------------------------
  naRef Canvas::addEventListener(const nasal::CallContext& ctx)
  {
    if( !_root_group.get() )
      naRuntimeError(ctx.c, "Canvas: No root group!");

    return _root_group->addEventListener(ctx);
  }

  //----------------------------------------------------------------------------
  void Canvas::setSizeX(int sx)
  {
    if( _size_x == sx )
      return;
    _size_x = sx;
    setStatusFlags(STATUS_DIRTY);

    if( _size_x <= 0 )
      setStatusFlags(MISSING_SIZE_X);
    else
      setStatusFlags(MISSING_SIZE_X, false);

    // reset flag to allow creation with new size
    setStatusFlags(CREATE_FAILED, false);
  }

  //----------------------------------------------------------------------------
  void Canvas::setSizeY(int sy)
  {
    if( _size_y == sy )
      return;
    _size_y = sy;
    setStatusFlags(STATUS_DIRTY);

    if( _size_y <= 0 )
      setStatusFlags(MISSING_SIZE_Y);
    else
      setStatusFlags(MISSING_SIZE_Y, false);

    // reset flag to allow creation with new size
    setStatusFlags(CREATE_FAILED, false);
  }

  //----------------------------------------------------------------------------
  int Canvas::getSizeX() const
  {
    return _size_x;
  }

  //----------------------------------------------------------------------------
  int Canvas::getSizeY() const
  {
    return _size_y;
  }

  //----------------------------------------------------------------------------
  void Canvas::setViewWidth(int w)
  {
    if( _view_width == w )
      return;
    _view_width = w;

    _texture.setViewSize(_view_width, _view_height);
  }

  //----------------------------------------------------------------------------
  void Canvas::setViewHeight(int h)
  {
    if( _view_height == h )
      return;
    _view_height = h;

    _texture.setViewSize(_view_width, _view_height);
  }

  //----------------------------------------------------------------------------
  int Canvas::getViewWidth() const
  {
    return _texture.getViewSize().x();
  }

  //----------------------------------------------------------------------------
  int Canvas::getViewHeight() const
  {
    return _texture.getViewSize().y();
  }

  //----------------------------------------------------------------------------
  SGRect<int> Canvas::getViewport() const
  {
    return SGRect<int>(0, 0, getViewWidth(), getViewHeight());
  }

  //----------------------------------------------------------------------------
  bool Canvas::handleMouseEvent(const MouseEventPtr& event)
  {
    if( !_root_group.get() )
      return false;

    EventVisitor visitor( EventVisitor::TRAVERSE_DOWN,
                          event->getClientPos(),
                          event->getDelta(),
                          _root_group );
    if( !_root_group->accept(visitor) )
      return false;

    return _event_manager->handleEvent(event, visitor.getPropagationPath());
  }

  //----------------------------------------------------------------------------
  void Canvas::childAdded( SGPropertyNode * parent,
                           SGPropertyNode * child )
  {
    if( parent != _node )
      return;

    if( child->getNameString() == "placement" )
      _dirty_placements.push_back(child);
    else if( _root_group.get() )
      static_cast<Element*>(_root_group.get())->childAdded(parent, child);
  }

  //----------------------------------------------------------------------------
  void Canvas::childRemoved( SGPropertyNode * parent,
                             SGPropertyNode * child )
  {
    _render_dirty = true;

    if( parent != _node )
      return;

    if( child->getNameString() == "placement" )
      _placements[ child->getIndex() ].clear();
    else if( _root_group.get() )
      static_cast<Element*>(_root_group.get())->childRemoved(parent, child);
  }

  //----------------------------------------------------------------------------
  void Canvas::valueChanged(SGPropertyNode* node)
  {
    if(    boost::starts_with(node->getNameString(), "status")
        || node->getParent()->getNameString() == "bounding-box" )
      return;
    _render_dirty = true;

    bool handled = true;
    if(    node->getParent()->getParent() == _node
        && node->getParent()->getNameString() == "placement" )
    {
      size_t index = node->getIndex();
      if( index < _placements.size() )
      {
        Placements& placements = _placements[index];
        if( !placements.empty() )
        {
          bool placement_dirty = false;
          BOOST_FOREACH(PlacementPtr& placement, placements)
          {
            // check if change can be directly handled by placement
            if(    placement->getProps() == node->getParent()
                && !placement->childChanged(node) )
              placement_dirty = true;
          }

          if( !placement_dirty )
            return;
        }
      }

      // prevent double updates...
      for( size_t i = 0; i < _dirty_placements.size(); ++i )
      {
        if( node->getParent() == _dirty_placements[i] )
          return;
      }

      _dirty_placements.push_back(node->getParent());
    }
    else if( node->getParent() == _node )
    {
      if( node->getNameString() == "background" )
      {
        osg::Vec4 color;
        if( _texture.getCamera() && parseColor(node->getStringValue(), color) )
        {
          _texture.getCamera()->setClearColor(color);
          _render_dirty = true;
        }
      }
      else if(    node->getNameString() == "mipmapping"
              || node->getNameString() == "coverage-samples"
              || node->getNameString() == "color-samples" )
      {
        _sampling_dirty = true;
      }
      else if( node->getNameString() == "additive-blend" )
      {
        _texture.useAdditiveBlend( node->getBoolValue() );
      }
      else if( node->getNameString() == "render-always" )
      {
        _render_always = node->getBoolValue();
      }
      else if( node->getNameString() == "size" )
      {
        if( node->getIndex() == 0 )
          setSizeX( node->getIntValue() );
        else if( node->getIndex() == 1 )
          setSizeY( node->getIntValue() );
      }
      else if( node->getNameString() == "view" )
      {
        if( node->getIndex() == 0 )
          setViewWidth( node->getIntValue() );
        else if( node->getIndex() == 1 )
          setViewHeight( node->getIntValue() );
      }
      else if( node->getNameString() == "freeze" )
        _texture.setRender( node->getBoolValue() );
      else
        handled = false;
    }
    else
      handled = false;

    if( !handled && _root_group.get() )
      _root_group->valueChanged(node);
  }

  //----------------------------------------------------------------------------
  osg::Texture2D* Canvas::getTexture() const
  {
    return _texture.getTexture();
  }

  //----------------------------------------------------------------------------
  Canvas::CullCallbackPtr Canvas::getCullCallback() const
  {
    return _cull_callback;
  }

  //----------------------------------------------------------------------------
  void Canvas::reloadPlacements(const std::string& type)
  {
    for(size_t i = 0; i < _placements.size(); ++i)
    {
      if( _placements[i].empty() )
        continue;

      SGPropertyNode* child = _placements[i].front()->getProps();
      if(    type.empty()
             // reload if type matches or no type specified
          || child->getStringValue("type", type.c_str()) == type )
      {
        _dirty_placements.push_back(child);
      }
    }
  }

  //----------------------------------------------------------------------------
  void Canvas::addPlacementFactory( const std::string& type,
                                    PlacementFactory factory )
  {
    if( _placement_factories.find(type) != _placement_factories.end() )
      SG_LOG
      (
        SG_GENERAL,
        SG_WARN,
        "Canvas::addPlacementFactory: replace existing factor for type " << type
      );

    _placement_factories[type] = factory;
  }

  //----------------------------------------------------------------------------
  void Canvas::setSelf(const PropertyBasedElementPtr& self)
  {
    PropertyBasedElement::setSelf(self);

    CanvasPtr canvas = boost::static_pointer_cast<Canvas>(self);

    _root_group.reset( new Group(canvas, _node) );
    _root_group->setSelf(_root_group);

    // Remove automatically created property listener as we forward them on our
    // own
    _root_group->removeListener();

    _cull_callback = new CullCallback(canvas);
  }

  //----------------------------------------------------------------------------
  void Canvas::setStatusFlags(unsigned int flags, bool set)
  {
    if( set )
      _status |= flags;
    else
      _status &= ~flags;

    if( (_status & MISSING_SIZE_X) && (_status & MISSING_SIZE_Y) )
      _status_msg = "Missing size";
    else if( _status & MISSING_SIZE_X )
      _status_msg = "Missing size-x";
    else if( _status & MISSING_SIZE_Y )
      _status_msg = "Missing size-y";
    else if( _status & CREATE_FAILED )
      _status_msg = "Creating render target failed";
    else if( _status == STATUS_DIRTY )
      _status_msg = "Creation pending...";
    else
      _status_msg = "Ok";
  }

  //----------------------------------------------------------------------------
  Canvas::PlacementFactoryMap Canvas::_placement_factories;

} // namespace canvas
} // namespace simgear
