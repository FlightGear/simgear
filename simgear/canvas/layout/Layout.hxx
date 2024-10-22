// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Basic class for canvas layouts
 */

#ifndef SG_CANVAS_LAYOUT_HXX_
#define SG_CANVAS_LAYOUT_HXX_

#include "LayoutItem.hxx"
#include <vector>

namespace simgear
{
namespace canvas
{

  /**
   * Base class for all Canvas layouts.
   */
  class Layout:
    public LayoutItem
  {
    public:
      virtual void addItem(const LayoutItemRef& item) = 0;
      virtual void setSpacing(int spacing) = 0;
      virtual int spacing() const = 0;

      /**
       * Get the number of items.
       */
      virtual size_t count() const = 0;

      /**
       * Get the item at position @a index.
       *
       * If there is no such item the function must do nothing and return an
       * empty reference.
       */
      virtual LayoutItemRef itemAt(size_t index) = 0;

      /**
       * Remove and get the item at position @a index.
       *
       * If there is no such item the function must do nothing and return an
       * empty reference.
       */
      virtual LayoutItemRef takeAt(size_t index) = 0;

      /**
       * Remove the given @a item from the layout.
       */
      void removeItem(const LayoutItemRef& item);

      /**
       * Remove all items.
       */
      virtual void clear();

      /**
       * Get the actual geometry of this layout given the rectangle \a geom
       * taking into account the alignment flags and size hints. For layouts,
       * if no alignment (different to AlignFill) is set, the whole area is
       * used. Excess space is distributed by the layouting algorithm and
       * handled by the individual children.
       *
       * @param geom    Area available to this layout.
       * @return The resulting geometry for this layout.
       */
      virtual SGRecti alignmentRect(const SGRecti& geom) const;

    protected:
      enum LayoutFlags
      {
        LAST_FLAG = LayoutItem::LAST_FLAG
      };

      Layout() = default;

      virtual void contentsRectChanged(const SGRecti& rect);

      /**
       * Override to implement the actual layouting
       */
      virtual void doLayout(const SGRecti& geom) = 0;


  };

  typedef SGSharedPtr<Layout> LayoutRef;

} // namespace canvas
} // namespace simgear


#endif /* SG_CANVAS_LAYOUT_HXX_ */
