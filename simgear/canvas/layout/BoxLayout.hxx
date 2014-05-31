// Align items horizontally or vertically in a box
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

#ifndef SG_CANVAS_BOX_LAYOUT_HXX_
#define SG_CANVAS_BOX_LAYOUT_HXX_

#include "Layout.hxx"

namespace simgear
{
namespace canvas
{

  class BoxLayout:
    public Layout
  {
    public:

      enum Direction
      {
        LeftToRight,
        RightToLeft,
        TopToBottom,
        BottomToTop
      };

      BoxLayout(Direction dir);

      virtual void addItem(const LayoutItemRef& item);

      void addItem(const LayoutItemRef& item, int stretch);

      /**
       * Set the stretch factor of the item at position @a index to @a stretch.
       */
      void setStretch(size_t index, int stretch);

      /**
       * Get the stretch factor of the item at position @a index
       */
      int stretch(size_t index) const;

      virtual void setSpacing(int spacing);
      virtual int spacing() const;

      void setDirection(Direction dir);
      Direction direction() const;

      virtual void setCanvas(const CanvasWeakPtr& canvas);

    protected:

      typedef const int& (SGVec2i::*CoordGetter)() const;
      CoordGetter _get_layout_coord,    //<! getter for coordinate in layout
                                        //   direction
                  _get_fixed_coord;     //<! getter for coordinate in secondary
                                        //   (fixed) direction

      int _padding;
      bool _reverse; //<! if true, right-to-left/bottom-to-top layouting

      mutable std::vector<ItemData> _layout_items;
      mutable ItemData _layout_data;

      void updateSizeHints() const;

      virtual SGVec2i sizeHintImpl() const;
      virtual SGVec2i minimumSizeImpl() const;
      virtual SGVec2i maximumSizeImpl() const;

      virtual void doLayout(const SGRecti& geom);
  };

  /**
   * Shortcut for creating a horizontal box layout
   */
  class HBoxLayout:
    public BoxLayout
  {
    public:
      HBoxLayout();
  };

  /**
   * Shortcut for creating a vertical box layout
   */
  class VBoxLayout:
    public BoxLayout
  {
    public:
      VBoxLayout();
  };

} // namespace canvas
} // namespace simgear


#endif /* SG_CANVAS_BOX_LAYOUT_HXX_ */

