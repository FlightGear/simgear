/// @file
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

      struct ItemData
      {
        LayoutItemRef layout_item;
        int     size_hint,
                min_size,
                max_size,
                padding_orig, //!< original padding as specified by the user
                padding,      //!< padding before element (layouted)
                size,         //!< layouted size
                stretch;      //!< stretch factor
        bool    visible : 1,
                has_align: 1, //!< Has alignment factor set (!= AlignFill)
                has_hfw : 1,  //!< height for width
                done : 1;     //!< layouting done

        /** Clear values (reset to default/empty state) */
        void reset();

        int hfw(int w) const;
        int mhfw(int w) const;
      };

      Layout();

      virtual void contentsRectChanged(const SGRecti& rect);

      /**
       * Override to implement the actual layouting
       */
      virtual void doLayout(const SGRecti& geom) = 0;

      /**
       * Distribute the available @a space to all @a items
       */
      void distribute(std::vector<ItemData>& items, const ItemData& space);

    private:

      int _num_not_done, //!< number of children not layouted yet
          _sum_stretch,  //!< sum of stretch factors of all not yet layouted
                         //   children
          _space_stretch,//!< space currently assigned to all not yet layouted
                         //   stretchable children
          _space_left;   //!< remaining space not used by any child yet

  };

  typedef SGSharedPtr<Layout> LayoutRef;

} // namespace canvas
} // namespace simgear


#endif /* SG_CANVAS_LAYOUT_HXX_ */
