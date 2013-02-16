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

#ifndef CANVAS_HXX_
#define CANVAS_HXX_

#include "canvas_fwd.hxx"
#include "ODGauge.hxx"

#include <simgear/canvas/elements/CanvasGroup.hxx>
#include <simgear/props/PropertyBasedElement.hxx>
#include <simgear/props/propertyObject.hxx>
#include <osg/NodeCallback>
#include <osg/observer_ptr>

#include <memory>
#include <string>

namespace simgear
{
namespace canvas
{
  class CanvasMgr;
  class MouseEvent;

  class Canvas:
    public PropertyBasedElement
  {
    public:

      enum StatusFlags
      {
        STATUS_OK,
        STATUS_DIRTY     = 1,
        MISSING_SIZE_X = STATUS_DIRTY << 1,
        MISSING_SIZE_Y = MISSING_SIZE_X << 1,
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
          CullCallback(const CanvasWeakPtr& canvas);

        private:
          CanvasWeakPtr _canvas;

          virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
      };
      typedef osg::ref_ptr<CullCallback> CullCallbackPtr;

      Canvas(SGPropertyNode* node);
      virtual ~Canvas();

      void setSystemAdapter(const SystemAdapterPtr& system_adapter);
      SystemAdapterPtr getSystemAdapter() const;

      void setCanvasMgr(CanvasMgr* canvas_mgr);
      CanvasMgr* getCanvasMgr() const;

      /**
       * Add a canvas which should be mared as dirty upon any change to this
       * canvas.
       *
       * This mechanism is used to eg. redraw a canvas if it's displaying
       * another canvas (recursive canvases)
       */
      void addDependentCanvas(const CanvasWeakPtr& canvas);

      /**
       * Stop notifying the given canvas upon changes
       */
      void removeDependentCanvas(const CanvasWeakPtr& canvas);

      GroupPtr createGroup(const std::string& name = "");

      /**
       * Enable rendering for the next frame
       *
       * @param force   Force redraw even if nothing has changed (if dirty flag
       *                is not set)
       */
      void enableRendering(bool force = false);

      void update(double delta_time_sec);

      naRef addEventListener(const nasal::CallContext& ctx);

      void setSizeX(int sx);
      void setSizeY(int sy);

      int getSizeX() const;
      int getSizeY() const;

      void setViewWidth(int w);
      void setViewHeight(int h);

      int getViewWidth() const;
      int getViewHeight() const;

      bool handleMouseEvent(const MouseEventPtr& event);

      virtual void childAdded( SGPropertyNode * parent,
                               SGPropertyNode * child );
      virtual void childRemoved( SGPropertyNode * parent,
                                 SGPropertyNode * child );
      virtual void valueChanged (SGPropertyNode * node);

      osg::Texture2D* getTexture() const;

      CullCallbackPtr getCullCallback() const;

      static void addPlacementFactory( const std::string& type,
                                       PlacementFactory factory );

    protected:

      SystemAdapterPtr  _system_adapter;
      CanvasMgr        *_canvas_mgr;

      std::auto_ptr<EventManager>   _event_manager;

      int _size_x,
          _size_y,
          _view_width,
          _view_height;

      PropertyObject<int>           _status;
      PropertyObject<std::string>   _status_msg;

      bool _sampling_dirty,
           _render_dirty,
           _visible;

      ODGauge _texture;
      GroupPtr _root_group;

      CullCallbackPtr _cull_callback;
      bool _render_always; //<! Used to disable automatic lazy rendering (culling)

      std::vector<SGPropertyNode*> _dirty_placements;
      std::vector<Placements> _placements;
      std::set<CanvasWeakPtr> _dependent_canvases; //<! Canvases which use this
                                                   //   canvas and should be
                                                   //   notified about changes

      typedef std::map<std::string, PlacementFactory> PlacementFactoryMap;
      static PlacementFactoryMap _placement_factories;

      virtual void setSelf(const PropertyBasedElementPtr& self);
      void setStatusFlags(unsigned int flags, bool set = true);

    private:

      Canvas(const Canvas&); // = delete;
      Canvas& operator=(const Canvas&); // = delete;
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_HXX_ */
