///@file
/// The canvas for rendering with the 2d API
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

#include <simgear_config.h>

#include "vg/openvg.h"
#include "Canvas.hxx"
#include "CanvasEventManager.hxx"
#include "CanvasEventVisitor.hxx"
#include "CanvasPlacement.hxx"

#include <simgear/canvas/events/KeyboardEvent.hxx>
#include <simgear/canvas/events/MouseEvent.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/scene/util/parse_color.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGImageUtils.hxx>

#include <osg/Camera>
#include <osg/Geode>
#include <osgText/Text>
#include <osgViewer/Viewer>

namespace simgear
{
namespace canvas
{
  /**
   * Camera Draw Callback for moving completed canvas images to subscribed listener.
   * This is attached to the underlying ODGauge camera.
   */
  class CanvasImageCallback : public osg::Camera::DrawCallback {
  public:
    CanvasImageCallback(osg::Texture2D *texture) : _texture(texture) {
      _previousFrameTick = osg::Timer::instance()->tick();
    }

    virtual void operator()(osg::RenderInfo& renderInfo) const {
      if (!_texture) {
        return;
      }
      osg::Timer_t n = osg::Timer::instance()->tick();
      double dt = osg::Timer::instance()->delta_s(_previousFrameTick, n);
      if (dt < _min_delta_tick) {
        // Do nothing
        return;
      }
      _previousFrameTick = n;

      if (hasSubscribers()) {
        // Transfer the FBO texture from the GPU to the CPU
        osg::State *state = renderInfo.getState();
        state->applyAttribute(_texture);
        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->readImageFromCurrentTexture(renderInfo.getContextID(), false, GL_UNSIGNED_BYTE);
        // Convert from RGBA to RGB because the subscriber might use JPEG
        osg::ref_ptr<osg::Image> image_rgb = ImageUtils::convertToRGB8(image);
        {
          OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_lock);
          while (!_subscribers.empty()) {
            try {
              CanvasImageReadyListener *subs = _subscribers.back();
              if (subs) {
                subs->imageReady(image_rgb);
              } else {
                SG_LOG(SG_GENERAL, SG_WARN, "CanvasImageCallback: Subscriber is null");
              }
            } catch (...) { }
            _subscribers.pop_back();
          }
        }
      }
    }

    void subscribe(CanvasImageReadyListener * subscriber) {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_lock);
      _subscribers.push_back(subscriber);
    }

    void unsubscribe(CanvasImageReadyListener * subscriber) {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_lock);
      _subscribers.remove(subscriber);
    }

    bool hasSubscribers() const {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_lock);
      return !_subscribers.empty();
    }
  private:
    osg::Texture2D *_texture;
    mutable std::list<CanvasImageReadyListener*> _subscribers;
    mutable OpenThreads::Mutex _lock;
    mutable double _previousFrameTick;
    double _min_delta_tick = 1.0 / 8.0;
  };

  //----------------------------------------------------------------------------
  Canvas::CullCallback::CullCallback(const CanvasWeakPtr& canvas):
    _canvas( canvas )
  {

  }

  //----------------------------------------------------------------------------
  void Canvas::CullCallback::operator()( osg::Node* node,
                                         osg::NodeVisitor* nv )
  {
    if( (nv->getTraversalMask() & simgear::MODEL_BIT) )
    {
      CanvasPtr canvas = _canvas.lock();
      if( canvas )
        canvas->enableRendering();
    }

    traverse(node, nv);
  }

  //----------------------------------------------------------------------------
  Canvas::Canvas(SGPropertyNode* node):
    PropertyBasedElement(node),
    _event_manager(new EventManager),
    _status(node, "status"),
    _status_msg(node, "status-msg")
  {
    // Looks like we need to propogate value changes upwards.
    node->setAttribute(SGPropertyNode::VALUE_CHANGED_DOWN, true);
    _status = 0;
    setStatusFlags(MISSING_SIZE_X | MISSING_SIZE_Y);

    _root_group.reset( new Group(this, _node) );

    // Remove automatically created property listener as we forward them on our
    // own
    _root_group->removeListener();
    _cull_callback = new CullCallback(this);
  }

  //----------------------------------------------------------------------------
  Canvas::~Canvas()
  {

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
  void Canvas::setLayout(const LayoutRef& layout)
  {
    _layout = layout;
    _layout->setCanvas(this);
  }

  //----------------------------------------------------------------------------
  void Canvas::setFocusElement(const ElementPtr& el)
  {
    if( el && el->getCanvas().lock() != this )
    {
      SG_LOG(SG_GUI, SG_WARN, "setFocusElement: element not from this canvas");
      return;
    }

    // TODO focus out/in events
    _focus_element = el;
  }

  //----------------------------------------------------------------------------
  void Canvas::clearFocusElement()
  {
    _focus_element.reset();
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
    if( _status & (CREATE_FAILED | MISSING_SIZE) )
      return;

    if( _status & STATUS_DIRTY )
    {
      // STATUS_DIRTY can only happen when size has changed
      _texture.setSize(_size_x, _size_y);

      // Create the ODGauge if it wasn't initialized yet
      if (!_texture.serviceable()) {
        _texture.useImageCoords(true);
        _texture.useStencil(true);
        _texture.allocRT(/*_camera_callback*/);

        osg::Camera* camera = _texture.getCamera();

        // XXX: Render order is determined by the Canvas index
        int order = _node->getIndex();
        camera->setRenderOrder(osg::Camera::PRE_RENDER, order);

        // Set the clear color to the background color
        osg::Vec4 clear_color(0.0f, 0.0f, 0.0f, 1.0f);
        parseColor(_node->getStringValue("background"), clear_color);
        camera->setClearColor(clear_color);

        // Add the scene group as children to the RTT camera.
        // NOTE: The scene group we are retrieving is a weak pointer. The only
        // ref_ptr holding the scene group is the child we've just added. If the
        // camera is destroyed, the scene group will also be destroyed.
        camera->addChild(_root_group->getSceneGroup());

        // Add the screenshot draw callback
        camera->setFinalDrawCallback(new CanvasImageCallback(_texture.getTexture()));
      }

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

    if( _layout )
      _layout->setGeometry(SGRecti(0, 0, _view_width, _view_height));

    if( _visible || _render_always )
    {
      for(auto& canvas_weak: _child_canvases)
      {
        // TODO should we check if the image the child canvas is displayed
        //      within is really visible?
        CanvasPtr canvas = canvas_weak.lock();
        if( canvas )
          canvas->_visible = true;
      }

      if( _render_dirty )
      {
        // Also mark all canvases this canvas is displayed within as dirty
        for(auto& canvas_weak: _parent_canvases)
        {
          CanvasPtr canvas = canvas_weak.lock();
          if( canvas )
            canvas->_render_dirty = true;
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
	
    if( _anisotropy_dirty )
    {
      _texture.setMaxAnisotropy( _node->getFloatValue("anisotropy") );
      _anisotropy_dirty = false;
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
          placement_factory->second(node, this);
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

  int Canvas::subscribe(CanvasImageReadyListener * subscriber)
  {
    const std::string &canvasname = _node->getStringValue("name");
    SG_LOG(SG_GENERAL, SG_DEBUG, "Canvas: subscribe to canvas '" << canvasname << "'");
    if (_texture.serviceable()) {
      osg::Camera* camera = _texture.getCamera();
      auto cb = dynamic_cast<CanvasImageCallback*>(camera->getFinalDrawCallback());
      if (cb) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "Canvas: adding subscriber to camera draw callback");
        cb->subscribe(subscriber);
        enableRendering(true);
        return 1;
      }
    }
    return 0;
  }

  void Canvas::unsubscribe(CanvasImageReadyListener * subscriber)
  {
    const std::string &canvasname = _node->getStringValue("name");
    SG_LOG(SG_GENERAL, SG_DEBUG, "Canvas: unsubscribe from canvas '" << canvasname << "'");
    if (_texture.serviceable()) {
      osg::Camera* camera = _texture.getCamera();
      auto cb = dynamic_cast<CanvasImageCallback*>(camera->getFinalDrawCallback());
      if (cb) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "Canvas: removing subscriber from camera draw callback");
        cb->unsubscribe(subscriber);
      }
    }
  }

  //----------------------------------------------------------------------------
  bool Canvas::addEventListener( const std::string& type,
                                 const EventListener& cb )
  {
    if( !_root_group.get() )
      throw std::runtime_error("Canvas::addEventListener: no root group!");

    return _root_group->addEventListener(type, cb);
  }

  //----------------------------------------------------------------------------
  bool Canvas::dispatchEvent(const EventPtr& event)
  {
    if( !_root_group.get() )
      throw std::runtime_error("Canvas::dispatchEvent: no root group!");

    return _root_group->dispatchEvent(event);
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
    if( !_root_group )
      return false;

    EventVisitor visitor( EventVisitor::TRAVERSE_DOWN,
                          event->getClientPos(),
                          _root_group );
    if( !_root_group->accept(visitor) )
      return false;

    return _event_manager->handleEvent(event, visitor.getPropagationPath());
  }

  //----------------------------------------------------------------------------
  bool Canvas::handleKeyboardEvent(const KeyboardEventPtr& event)
  {
    ElementPtr target = _focus_element.lock();
    if( !target )
      target = _root_group;
    if( !target )
      return false;

    return target->dispatchEvent(event);
  }

  //----------------------------------------------------------------------------
  bool Canvas::propagateEvent( EventPtr const& event,
                               EventPropagationPath const& path )
  {
    return _event_manager->propagateEvent(event, path);
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

    if (child->getNameString() == "placement") {
        const size_t index = child->getIndex();
        if (index < _placements.size()) {
            _placements[child->getIndex()].clear();
        } else {
            SG_LOG(SG_GENERAL, SG_DEV_ALERT, "Cavas::childRemoved: no placement at index:" << child->getPath());
        }
    } else if (_root_group.get()) {
        static_cast<Element*>(_root_group.get())->childRemoved(parent, child);
    }
  }

  //----------------------------------------------------------------------------
  void Canvas::valueChanged(SGPropertyNode* node)
  {
    const std::string& name = node->getNameString();

    if(    strutils::starts_with(name, "status")
        || strutils::starts_with(name, "data-") )
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
          for(auto& placement: placements)
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
      if( name == "background" )
      {
        osg::Vec4 color;
        if( _texture.getCamera() && parseColor(node->getStringValue(), color) )
        {
          _texture.getCamera()->setClearColor(color);
          _render_dirty = true;
        }
      }
      else if(   name == "mipmapping"
              || name == "coverage-samples"
              || name == "color-samples" )
      {
        _sampling_dirty = true;
      }
      else if( name == "anisotropy" )
      {
        _anisotropy_dirty = true;
      }
      else if( name == "additive-blend" )
      {
        _texture.useAdditiveBlend( node->getBoolValue() );
      }
      else if( name == "render-always" )
      {
        _render_always = node->getBoolValue();
      }
      else if( name == "size" )
      {
        if( node->getIndex() == 0 )
          setSizeX( node->getIntValue() );
        else if( node->getIndex() == 1 )
          setSizeY( node->getIntValue() );
      }
      else if( name == "update" )
      {
        if( _root_group )
          _root_group->update(0);
        return update(0);
      }
      else if( name == "view" )
      {
        if( node->getIndex() == 0 )
          setViewWidth( node->getIntValue() );
        else if( node->getIndex() == 1 )
          setViewHeight( node->getIntValue() );
      }
      else if( name == "freeze" )
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
  osg::Camera* Canvas::getCamera() const
  {
    return _texture.getCamera();
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
        "Canvas::addPlacementFactory: replace existing factory '" << type << "'"
      );

    _placement_factories[type] = factory;
  }

  //----------------------------------------------------------------------------
  void Canvas::removePlacementFactory(const std::string& type)
  {
    PlacementFactoryMap::iterator it = _placement_factories.find(type);
    if( it == _placement_factories.end() )
      SG_LOG
      (
        SG_GENERAL,
        SG_WARN,
        "Canvas::removePlacementFactory: no such factory '" << type << "'"
      );
    else
      _placement_factories.erase(it);
  }


  //----------------------------------------------------------------------------
  void Canvas::setSystemAdapter(const SystemAdapterPtr& system_adapter)
  {
    _system_adapter = system_adapter;
  }

  //----------------------------------------------------------------------------
  SystemAdapterPtr Canvas::getSystemAdapter()
  {
    return _system_adapter;
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
    else if( _status & STATUS_DIRTY )
      _status_msg = "Creation pending...";
    else
      _status_msg = "Ok";
  }

  //----------------------------------------------------------------------------
  ElementPtr Canvas::getFocusedElement() const
  {
    return _focus_element.lock();
  }


  //----------------------------------------------------------------------------
  Canvas::PlacementFactoryMap Canvas::_placement_factories;
  SystemAdapterPtr Canvas::_system_adapter;

} // namespace canvas
} // namespace simgear
