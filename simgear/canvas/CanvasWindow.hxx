// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Window for placing a Canvas onto it (for dialogs, menus, etc.)
 */

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

class FocusScope;

class Window : public Image,
               public LayoutItem
{
public:
    static const std::string TYPE_NAME;

    enum Resize {
        NONE = 0,
        LEFT = 1,
        RIGHT = LEFT << 1,
        TOP = RIGHT << 1,
        BOTTOM = TOP << 1,
        INIT = BOTTOM << 1
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
    Window(const CanvasWeakPtr& canvas,
           const SGPropertyNode_ptr& node,
           const Style& parent_style = Style(),
           Element* parent = 0);
    virtual ~Window();

    void update(double delta_time_sec) override;
    void valueChanged(SGPropertyNode* node) override;

    const SGVec2<float> getPosition() const;
    const SGRect<float> getScreenRegion() const;

    void setCanvasContent(CanvasPtr canvas);
    simgear::canvas::CanvasWeakPtr getCanvasContent() const;

    void setLayout(const LayoutRef& layout);

    CanvasPtr getCanvasDecoration() const;

    bool isResizable() const;
    bool isCapturingEvents() const;

    void setVisible(bool visible) override;
    bool isVisible() const override;

    /**
       * Moves window on top of all other windows with the same z-index.
       *
       * @note If no z-index is set it defaults to 0.
       */
    void raise();

    void handleResize(uint8_t mode,
                      const osg::Vec2f& offset = osg::Vec2f());

    bool handleEvent(const EventPtr& event) override;

    SGVec2<float> toScreenPosition(const osg::Vec2f& pos = {}) const;

    FocusScope* focusScope();

protected:
    enum Attributes {
        DECORATION = 1
    };

    uint32_t _attributes_dirty{0};

    CanvasPtr _canvas_decoration;
    CanvasWeakPtr _canvas_content;
    LayoutRef _layout;

    ImagePtr _image_content,
        _image_shadow;

    bool _resizable{false},
        _capture_events{true};

    PropertyObject<int> _resize_top,
        _resize_right,
        _resize_bottom,
        _resize_left,
        _resize_status;

    CSSBorder _decoration_border;

    /// @brief offset from the Windows position to the content canvas
    /// This is zero when no decoration is set.
    SGVec2<float> _contentOffset;

    std::unique_ptr<FocusScope> _focus_scope;

    void parseDecorationBorder(const std::string& str);
    void updateDecoration();

    void invalidate() override;
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_WINDOW_HXX_ */
