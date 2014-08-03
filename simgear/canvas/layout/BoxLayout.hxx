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

#ifndef SG_CANVAS_BOX_LAYOUT_HXX_
#define SG_CANVAS_BOX_LAYOUT_HXX_

#include "Layout.hxx"

namespace simgear
{
namespace canvas
{

  /**
   * Align LayoutItems horizontally or vertically in a box.
   *
   * @see http://qt-project.org/doc/qt-4.8/qboxlayout.html#details
   */
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
      ~BoxLayout();

      virtual void addItem(const LayoutItemRef& item);

      void addItem( const LayoutItemRef& item,
                    int stretch,
                    uint8_t alignment = 0 );

      void addStretch(int stretch = 0);

      void addSpacing(int size);

      void insertItem( int index,
                       const LayoutItemRef& item,
                       int stretch = 0,
                       uint8_t alignment = 0 );

      void insertStretch(int index, int stretch = 0);

      void insertSpacing(int index, int size);

      virtual size_t count() const;
      virtual LayoutItemRef itemAt(size_t index);
      virtual LayoutItemRef takeAt(size_t index);
      virtual void clear();

      /**
       * Set the stretch factor of the item at position @a index to @a stretch.
       */
      void setStretch(size_t index, int stretch);

      /**
       * Set the stretch factor of the given @a item to @a stretch, if it exists
       * in this layout.
       *
       * @return true, if the @a item was found in the layout
       */
      bool setStretchFactor(const LayoutItemRef& item, int stretch);

      /**
       * Get the stretch factor of the item at position @a index
       */
      int stretch(size_t index) const;

      virtual void setSpacing(int spacing);
      virtual int spacing() const;

      void setDirection(Direction dir);
      Direction direction() const;

      virtual bool hasHeightForWidth() const;

      virtual void setCanvas(const CanvasWeakPtr& canvas);

      bool horiz() const;

    protected:

      typedef const int& (SGVec2i::*CoordGetter)() const;
      CoordGetter _get_layout_coord,    //!< getter for coordinate in layout
                                        //   direction
                  _get_fixed_coord;     //!< getter for coordinate in secondary
                                        //   (fixed) direction

      int _padding;
      Direction _direction;

      typedef std::vector<ItemData> LayoutItems;

      mutable LayoutItems _layout_items;
      mutable ItemData _layout_data;

      // Cache for last height-for-width query
      mutable int _hfw_width,
                  _hfw_height,
                  _hfw_min_height;

      void updateSizeHints() const;
      void updateWFHCache(int w) const;

      virtual SGVec2i sizeHintImpl() const;
      virtual SGVec2i minimumSizeImpl() const;
      virtual SGVec2i maximumSizeImpl() const;

      virtual int heightForWidthImpl(int w) const;
      virtual int minimumHeightForWidthImpl(int w) const;

      virtual void doLayout(const SGRecti& geom);

      virtual void visibilityChanged(bool visible);
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

  typedef SGSharedPtr<BoxLayout> BoxLayoutRef;

} // namespace canvas
} // namespace simgear


#endif /* SG_CANVAS_BOX_LAYOUT_HXX_ */

