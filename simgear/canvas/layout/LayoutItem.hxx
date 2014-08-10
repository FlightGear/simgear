///@file
/// Basic element for layouting canvas elements.
//
// Copyright (C) 2014  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef SG_CANVAS_LAYOUT_ITEM_HXX_
#define SG_CANVAS_LAYOUT_ITEM_HXX_

#include <simgear/canvas/canvas_fwd.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/math/SGRect.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/structure/SGWeakReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

namespace simgear
{
namespace canvas
{
  class LayoutItem;
  typedef SGSharedPtr<LayoutItem> LayoutItemRef;
  typedef SGWeakPtr<LayoutItem> LayoutItemWeakRef;

  /**
   * Holds the four margins for a rectangle.
   */
  struct Margins
  {
    int l, t, r, b;

    /**
     * Set all margins to the same value @a m.
     */
    explicit Margins(int m = 0);

    /**
     * Set horizontal and vertical margins to the same values @a h and @a v
     * respectively.
     *
     * @param h Horizontal margins
     * @param v Vertical margins
     */
    Margins(int h, int v);

    /**
     * Set the margins to the given values.
     */
    Margins(int left, int top, int right, int bottom);

    /**
     * Get the total horizontal margin (sum of left and right margin).
     */
    int horiz() const;

    /**
     * Get the total vertical margin (sum of top and bottom margin).
     */
    int vert() const;

    /**
     * Get total horizontal and vertical margin as vector.
     */
    SGVec2i size() const;

    /**
     * Returns true if all margins are 0.
     */
    bool isNull() const;
  };

  /**
   * Flags for LayoutItem alignment inside {@link Layout Layouts}.
   *
   * @note You can only use one horizontal and one vertical flag at the same.
   */
  enum AlignmentFlag
  {
#define ALIGN_ENUM_MAPPING(key, val, comment) key = val, /*!< comment */
#  include "AlignFlag_values.hxx"
#undef ALIGN_ENUM_MAPPING
  };

  /**
   * Base class for all layouting elements. Specializations either implement a
   * layouting algorithm or a widget.
   */
  class LayoutItem:
    public virtual SGVirtualWeakReferenced
  {
    public:

      /** Maximum item size (indicating no limit) */
      static const SGVec2i MAX_SIZE;

      LayoutItem();
      virtual ~LayoutItem();

      /**
       * Set the margins to use by the layout system around the item.
       *
       * The margins define the size of the clear area around an item. It
       * increases the size hints and reduces the size of the geometry()
       * available to child layouts and widgets.
       *
       * @see Margins
       */
      void setContentsMargins(const Margins& margins);

      /**
       * Set the individual margins.
       *
       * @see setContentsMargins(const Margins&)
       */
      void setContentsMargins(int left, int top, int right, int bottom);

      /**
       * Set all margins to the same value.
       *
       * @see setContentsMargins(const Margins&)
       */
      void setContentsMargin(int margin);

      /**
       * Get the currently used margins.
       *
       * @see setContentsMargins(const Margins&)
       * @see Margins
       */
      Margins getContentsMargins() const;

      /**
       * Get the area available to the contents.
       *
       * This is equal to the geometry() reduced by the sizes of the margins.
       *
       * @see setContentsMargins(const Margins&)
       * @see geometry()
       */
      SGRecti contentsRect() const;

      /**
       * Get the preferred size of this item.
       */
      SGVec2i sizeHint() const;

      /**
       * Get the minimum amount of the space this item requires.
       */
      SGVec2i minimumSize() const;

      /**
       * Get the maximum amount of space this item can use.
       */
      SGVec2i maximumSize() const;

      /**
       * Returns true if this items preferred and minimum height depend on its
       * width.
       *
       * The default implementation returns false. Reimplement for items
       * providing height for width.
       *
       * @see heightForWidth()
       * @see minimumHeightForWidth()
       */
      virtual bool hasHeightForWidth() const;

      /**
       * Returns the preferred height for the given width @a w.
       *
       * Reimplement heightForWidthImpl() for items providing height for width.
       *
       * @see hasHeightForWidth()
       */
      int heightForWidth(int w) const;

      /**
       * Returns the minimum height for the given width @a w.
       *
       * Reimplement minimumHeightForWidthImpl() for items providing height for
       * width.
       *
       * @see hasHeightForWidth()
       */
      int minimumHeightForWidth(int w) const;

      /**
       * Set alignment of item within {@link Layout Layouts}.
       *
       * @param alignment Bitwise combination of vertical and horizontal
       *                  alignment flags.
       * @see AlignmentFlag
       */
      void setAlignment(uint8_t alignment);

      /**
       * Get all alignment flags.
       *
       * @see AlignmentFlag
       */
      uint8_t alignment() const;

      virtual void setVisible(bool visible);
      virtual bool isVisible() const;

      bool isExplicitlyHidden() const;

      void show() { setVisible(true); }
      void hide() { setVisible(false); }

      /**
       * Mark all cached data as invalid and require it to be recalculated.
       */
      virtual void invalidate();

      /**
       * Mark all cached data of parent item as invalid (if the parent is set).
       */
      void invalidateParent();

      /**
       * Apply any changes not applied yet.
       */
      void update();

      /**
       * Set position and size of this element. For layouts this triggers a
       * recalculation of the layout.
       */
      virtual void setGeometry(const SGRecti& geom);

      /**
       * Get position and size of this element.
       */
      virtual SGRecti geometry() const;

      /**
       * Get the actual geometry of this item given the rectangle \a geom
       * taking into account the alignment flags and size hints.
       *
       * @param geom    Area available to this item.
       * @return The resulting geometry for this item.
       *
       * @see setAlignment()
       * @see minimumSize()
       * @see maximumSize()
       * @see sizeHint()
       */
      virtual SGRecti alignmentRect(const SGRecti& geom) const;

      /**
       * Set the canvas this item is attached to.
       */
      virtual void setCanvas(const CanvasWeakPtr& canvas);

      /**
       * Get the canvas this item is attached to.
       */
      CanvasPtr getCanvas() const;

      /**
       * Set the parent layout item (usually this is a layout).
       */
      void setParent(const LayoutItemWeakRef& parent);

      /**
       * Get the parent layout.
       */
      LayoutItemRef getParent() const;

      /// Called before item is removed from a layout
      virtual void onRemove() {}

    protected:

      friend class Canvas;

      enum Flags
      {
        SIZE_HINT_DIRTY = 1,
        MINIMUM_SIZE_DIRTY = SIZE_HINT_DIRTY << 1,
        MAXIMUM_SIZE_DIRTY = MINIMUM_SIZE_DIRTY << 1,
        SIZE_INFO_DIRTY = SIZE_HINT_DIRTY
                        | MINIMUM_SIZE_DIRTY
                        | MAXIMUM_SIZE_DIRTY,
        EXPLICITLY_HIDDEN = MAXIMUM_SIZE_DIRTY << 1,
        VISIBLE = EXPLICITLY_HIDDEN << 1,
        LAYOUT_DIRTY = VISIBLE << 1,
        LAST_FLAG = LAYOUT_DIRTY
      };

      CanvasWeakPtr     _canvas;
      LayoutItemWeakRef _parent;

      SGRecti           _geometry;
      Margins           _margins;
      uint8_t           _alignment;

      mutable uint32_t  _flags;
      mutable SGVec2i   _size_hint,
                        _min_size,
                        _max_size;

      virtual SGVec2i sizeHintImpl() const;
      virtual SGVec2i minimumSizeImpl() const;
      virtual SGVec2i maximumSizeImpl() const;

      /**
       * Returns the preferred height for the given width @a w.
       *
       * The default implementation returns -1, indicating that the preferred
       * height is independent of the given width.
       *
       * Reimplement this function for items supporting height for width.
       *
       * @note Do not take margins into account, as this is already handled
       *       before calling this function.
       *
       * @see hasHeightForWidth()
       */
      virtual int heightForWidthImpl(int w) const;

      /**
       * Returns the minimum height for the given width @a w.
       *
       * The default implementation returns -1, indicating that the minimum
       * height is independent of the given width.
       *
       * Reimplement this function for items supporting height for width.
       *
       * @note Do not take margins into account, as this is already handled
       *       before calling this function.
       *
       * @see hasHeightForWidth()
       */
      virtual int minimumHeightForWidthImpl(int w) const;

      /**
       * @return whether the visibility has changed.
       */
      void setVisibleInternal(bool visible);

      virtual void contentsRectChanged(const SGRecti& rect) {};

      virtual void visibilityChanged(bool visible) {}

      /**
       * Allow calling the protected setVisibleImpl from derived classes
       */
      static void callSetVisibleInternal(LayoutItem* item, bool visible);

  };

} // namespace canvas
} // namespace simgear

#endif /* SG_CANVAS_LAYOUT_ITEM_HXX_ */
