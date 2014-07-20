// Window for placing a Canvas onto it (for dialogs, menus, etc.)
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

#ifndef CANVAS_WINDOW_HXX_
#define CANVAS_WINDOW_HXX_

#include <simgear/canvas/elements/CanvasImage.hxx>
#include <simgear/canvas/layout/Layout.hxx>
#include <simgear/canvas/events/MouseEvent.hxx>
#include <simgear/props/PropertyBasedElement.hxx>
#include <simgear/props/propertyObject.hxx>
#include <simgear/misc/CSSBorder.hxx>

#include <osg/Geode>
#include <osg/Geometry>

namespace simgear
{
namespace canvas
{

  class Window:
    public Image,
    public LayoutItem
  {
    public:
      static const std::string TYPE_NAME;

      enum Resize
      {
        NONE    = 0,
        LEFT    = 1,
        RIGHT   = LEFT << 1,
        TOP     = RIGHT << 1,
        BOTTOM  = TOP << 1,
        INIT    = BOTTOM << 1
      };

      /**
       * @param node    Property node containing settings for this window:
       *                  capture-events    Disable/Enable event capturing
       *                  content-size[0-1] Size of content area (excluding
       *                                    decoration border)
       *                  decoration-border Size of decoration border
       *                  resize            Enable resize cursor and properties
       *                  shadow-inset      Inset of shadow image
       *                  shadow-radius     Radius/outset of shadow image
       */
      Window( const CanvasWeakPtr& canvas,
              const SGPropertyNode_ptr& node,
              const Style& parent_style = Style(),
              Element* parent = 0 );
      virtual ~Window();

      virtual void update(double delta_time_sec);
      virtual void valueChanged(SGPropertyNode* node);

      osg::Group* getGroup();
      const SGVec2<float> getPosition() const;
      const SGRect<float>  getScreenRegion() const;

      void setCanvasContent(CanvasPtr canvas);
      simgear::canvas::CanvasWeakPtr getCanvasContent() const;

      void setLayout(const LayoutRef& layout);

      CanvasPtr getCanvasDecoration() const;

      bool isResizable() const;
      bool isCapturingEvents() const;

      virtual void setVisible(bool visible);
      virtual bool isVisible() const;

      /**
       * Moves window on top of all other windows with the same z-index.
       *
       * @note If no z-index is set it defaults to 0.
       */
      void raise();

      void handleResize( uint8_t mode,
                         const osg::Vec2f& offset = osg::Vec2f() );

    protected:

      enum Attributes
      {
        DECORATION = 1
      };

      uint32_t  _attributes_dirty;

      CanvasPtr        _canvas_decoration;
      CanvasWeakPtr    _canvas_content;
      LayoutRef        _layout;

      ImagePtr _image_content,
               _image_shadow;

      bool _resizable,
           _capture_events;

      PropertyObject<int> _resize_top,
                          _resize_right,
                          _resize_bottom,
                          _resize_left,
                          _resize_status;

      CSSBorder _decoration_border;

      void parseDecorationBorder(const std::string& str);
      void updateDecoration();
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_WINDOW_HXX_ */
