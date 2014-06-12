///@file Basic element for layouting canvas elements
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
   * Base class for all layouting elements. Specializations either implement a
   * layouting algorithm or a widget.
   */
  class LayoutItem:
    public virtual SGVirtualWeakReferenced
  {
    public:

      static const SGVec2i MAX_SIZE;

      LayoutItem();
      virtual ~LayoutItem();

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

      virtual bool hasHeightForWidth() const;
      virtual int heightForWidth(int w) const;
      virtual int minimumHeightForWidth(int w) const;

      /**
       * Mark all cached data as invalid and require it to be recalculated.
       */
      virtual void invalidate();

      /**
       * Mark all cached data of parent item as invalid (if it is known)
       */
      void invalidateParent();

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
        LAST_FLAG = MAXIMUM_SIZE_DIRTY
      };

      CanvasWeakPtr     _canvas;
      LayoutItemWeakRef _parent;

      SGRecti           _geometry;

      mutable uint32_t  _flags;
      mutable SGVec2i   _size_hint,
                        _min_size,
                        _max_size;

      virtual SGVec2i sizeHintImpl() const;
      virtual SGVec2i minimumSizeImpl() const;
      virtual SGVec2i maximumSizeImpl() const;

  };

} // namespace canvas
} // namespace simgear

#endif /* SG_CANVAS_LAYOUT_ITEM_HXX_ */
