// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief The Canvas 2D API
 */

#ifndef CANVAS_HXX_
#define CANVAS_HXX_

#include "canvas_fwd.hxx"
#include "ODGauge.hxx"

#include <simgear/canvas/elements/CanvasGroup.hxx>
#include <simgear/canvas/layout/Layout.hxx>
#include <simgear/math/SGRect.hxx>
#include <simgear/nasal/cppbind/NasalObject.hxx>
#include <simgear/props/PropertyBasedElement.hxx>
#include <simgear/props/propertyObject.hxx>

#include <osg/NodeCallback>
#include <osg/observer_ptr>

#include <memory>
#include <string>

namespace simgear
{
/// Canvas 2D drawing API
namespace canvas
{
  class CanvasMgr;
  class MouseEvent;

  /**
   * A listener interested in completed canvas drawing.
   */
  class CanvasImageReadyListener {
  public:
    virtual void imageReady(osg::ref_ptr<osg::Image>) = 0;
    virtual ~CanvasImageReadyListener()
    {
    }
  };

  /**
   * Canvas to draw onto (to an off-screen render target).
   */
  class Canvas:
    public PropertyBasedElement,
    public nasal::Object
  {
    public:

      enum StatusFlags
      {
        STATUS_OK,
        STATUS_DIRTY   = 1,
        MISSING_SIZE_X = STATUS_DIRTY << 1,
        MISSING_SIZE_Y = MISSING_SIZE_X << 1,
        MISSING_SIZE   = MISSING_SIZE_X | MISSING_SIZE_Y,
        CREATE_FAILED  = MISSING_SIZE_Y << 1
      };

      /**
       * This callback is installed on every placement of the canvas in the
       * scene to only render the canvas if at least one placement is visible
       */
      class CullCallback:
        public osg::NodeCallback
      {
        public:
          explicit CullCallback(const CanvasWeakPtr& canvas);

        private:
          CanvasWeakPtr _canvas;

          void operator()(osg::Node* node, osg::NodeVisitor* nv) override;
      };
      typedef osg::ref_ptr<CullCallback> CullCallbackPtr;

      explicit Canvas(SGPropertyNode* node);
      virtual ~Canvas();
      void onDestroy() override;

      void setCanvasMgr(CanvasMgr* canvas_mgr);
      CanvasMgr* getCanvasMgr() const;

      bool isInit() const;

      /**
       * Add a canvas which should be marked as dirty upon any change to this
       * canvas.
       *
       * This mechanism is used to eg. redraw a canvas if it's displaying
       * another canvas (recursive canvases)
       */
      void addParentCanvas(const CanvasWeakPtr& canvas);

      /**
       * Add a canvas which should be marked visible if this canvas is visible.
       */
      void addChildCanvas(const CanvasWeakPtr& canvas);

      /**
       * Stop notifying the given canvas upon changes
       */
      void removeParentCanvas(const CanvasWeakPtr& canvas);
      void removeChildCanvas(const CanvasWeakPtr& canvas);

      /**
       * Create a new group
       */
      GroupPtr createGroup(const std::string& name = "");

      /**
       * Get an existing group with the given name
       */
      GroupPtr getGroup(const std::string& name);

      /**
       * Get an existing group with the given name or otherwise create a new
       * group
       */
      GroupPtr getOrCreateGroup(const std::string& name);

      /**
       * Get the root group of the canvas
       */
      GroupPtr getRootGroup();

      /**
       * Set the layout of the canvas (the layout will automatically update with
       * the viewport size of the canvas)
       */
      void setLayout(const LayoutRef& layout);

      /**
       * Set the focus to the given element.
       *
       * The focus element will receive all keyboard events propagated to this
       * canvas. If there is no valid focus element the root group will receive
       * the events instead.
       */
      void setFocusElement(const ElementPtr& el);

      ElementPtr getFocusedElement() const;

      /**
       * Clear the focus element.
       *
       * @see setFocusElement()
       */
      void clearFocusElement();

      /**
       * Enable rendering for the next frame
       *
       * @param force   Force redraw even if nothing has changed (if dirty flag
       *                is not set)
       */
      void enableRendering(bool force = false);

      void update(double delta_time_sec) override;

      int subscribe(CanvasImageReadyListener * subscriber);
      void unsubscribe(CanvasImageReadyListener * subscriber);

      bool addEventListener(const std::string& type, const EventListener& cb);
      bool dispatchEvent(const EventPtr& event);

      void setSizeX(int sx);
      void setSizeY(int sy);

      int getSizeX() const;
      int getSizeY() const;

      void setViewWidth(int w);
      void setViewHeight(int h);

      int getViewWidth() const;
      int getViewHeight() const;
      SGRect<int> getViewport() const;

      bool handleMouseEvent(const MouseEventPtr& event);
      bool handleKeyboardEvent(const KeyboardEventPtr& event);

      bool propagateEvent( EventPtr const& event,
                           EventPropagationPath const& path );

      void childAdded(SGPropertyNode* parent, SGPropertyNode* child) override;
      void childRemoved(SGPropertyNode* parent, SGPropertyNode* child) override;
      void valueChanged(SGPropertyNode * node) override;

      osg::Camera* getCamera() const;
      osg::Texture2D* getTexture() const;

      CullCallbackPtr getCullCallback() const;

      void reloadPlacements( const std::string& type = std::string() );
      static void addPlacementFactory( const std::string& type,
                                       PlacementFactory factory );
      static void removePlacementFactory(const std::string& type);

      /**
       * Set global SystemAdapter for all Canvas/ODGauge instances.
       *
       * The SystemAdapter is responsible for application specific operations
       * like loading images/fonts and adding/removing cameras to the scene
       * graph.
       */
      static void setSystemAdapter(const SystemAdapterPtr& system_adapter);
      static SystemAdapterPtr getSystemAdapter();

    protected:

      CanvasMgr *_canvas_mgr {nullptr};

      std::unique_ptr<EventManager> _event_manager;

      int _size_x {-1},
          _size_y {-1},
          _view_width {-1},
          _view_height {-1};

      PropertyObject<int>           _status;
      PropertyObject<std::string>   _status_msg;

      bool _sampling_dirty {false},
           _anisotropy_dirty {false},
           _render_dirty {true},
           _visible {true};

      ODGauge _texture;

      GroupPtr  _root_group;
      LayoutRef _layout;

      ElementWeakPtr _focus_element;

      CullCallbackPtr _cull_callback;

      /** Used to disable automatic lazy rendering (culling) */
      bool _render_always {false};

      std::vector<SGPropertyNode*> _dirty_placements;
      std::vector<Placements> _placements;
      std::set<CanvasWeakPtr> _parent_canvases, //!< Canvases showing this canvas
                              _child_canvases;  //!< Canvases displayed within
                                                //   this canvas

      typedef std::map<std::string, PlacementFactory> PlacementFactoryMap;
      static PlacementFactoryMap _placement_factories;

      void setStatusFlags(unsigned int flags, bool set = true);

    private:

      static SystemAdapterPtr _system_adapter;

      Canvas(const Canvas&) = delete;
      Canvas& operator=(const Canvas&) = delete;
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_HXX_ */
