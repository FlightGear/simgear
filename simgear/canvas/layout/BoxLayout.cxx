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

#include "BoxLayout.hxx"
#include <simgear/canvas/Canvas.hxx>

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  BoxLayout::BoxLayout(Direction dir):
    _padding(5)
  {
    setDirection(dir);
  }

  //----------------------------------------------------------------------------
  void BoxLayout::addItem(const LayoutItemRef& item)
  {
    return addItem(item, 0);
  }

  //----------------------------------------------------------------------------
  void BoxLayout::addItem(const LayoutItemRef& item, int stretch)
  {
    ItemData item_data = {0};
    item_data.layout_item = item;
    item_data.stretch = std::max(0, stretch);

    item->setCanvas(_canvas);
    item->setParent(this);

    _layout_items.push_back(item_data);

    invalidate();
  }

  //----------------------------------------------------------------------------
  void BoxLayout::setStretch(size_t index, int stretch)
  {
    if( index >= _layout_items.size() )
      return;

    _layout_items.at(index).stretch = std::max(0, stretch);
    invalidate();
  }

  //----------------------------------------------------------------------------
  int BoxLayout::stretch(size_t index) const
  {
    if( index >= _layout_items.size() )
      return 0;

    return _layout_items.at(index).stretch;
  }

  //----------------------------------------------------------------------------
  void BoxLayout::setSpacing(int spacing)
  {
    if( spacing == _padding )
      return;

    _padding = spacing;
    invalidate();
  }

  //----------------------------------------------------------------------------
  int BoxLayout::spacing() const
  {
    return _padding;
  }

  //----------------------------------------------------------------------------
  void BoxLayout::setDirection(Direction dir)
  {
    _get_layout_coord = &SGVec2i::x;
    _get_fixed_coord = &SGVec2i::y;

    if( dir == TopToBottom || dir == BottomToTop )
      std::swap(_get_layout_coord, _get_fixed_coord);

    _reverse = (dir == RightToLeft || dir == BottomToTop );

    invalidate();
  }

  //----------------------------------------------------------------------------
  BoxLayout::Direction BoxLayout::direction() const
  {
    if( _get_layout_coord == static_cast<CoordGetter>(&SGVec2i::x) )
      return _reverse ? RightToLeft : LeftToRight;
    else
      return _reverse ? BottomToTop : TopToBottom;
  }

  //----------------------------------------------------------------------------
  void BoxLayout::setCanvas(const CanvasWeakPtr& canvas)
  {
    _canvas = canvas;

    for(size_t i = 0; i < _layout_items.size(); ++i)
      _layout_items[i].layout_item->setCanvas(canvas);
  }

  //----------------------------------------------------------------------------
  void BoxLayout::updateSizeHints() const
  {
    SGVec2i min_size(0, 0),
            max_size(0, 0),
            size_hint(0, 0);

    _layout_data.reset();
    bool is_first = true;

    for(size_t i = 0; i < _layout_items.size(); ++i)
    {
      // TODO check visible

      ItemData& item_data = _layout_items[i];
      LayoutItem const& item = *item_data.layout_item;

      item_data.min_size  = (item.minimumSize().*_get_layout_coord)();
      item_data.max_size  = (item.maximumSize().*_get_layout_coord)();
      item_data.size_hint = (item.sizeHint().*_get_layout_coord)();

      if( is_first )
      {
        item_data.padding_orig = 0;
        is_first = false;
      }
      else
        item_data.padding_orig = _padding;

     _layout_data.padding += item_data.padding_orig;

      // Add sizes of all children in layout direction
      safeAdd(min_size.x(),  item_data.min_size);
      safeAdd(max_size.x(),  item_data.max_size);
      safeAdd(size_hint.x(), item_data.size_hint);

      // Take maximum in fixed (non-layouted) direction
      min_size.y()  = std::max( min_size.y(),
                                (item.minimumSize().*_get_fixed_coord)() );
      max_size.y()  = std::max( max_size.y(),
                                (item.maximumSize().*_get_fixed_coord)() );
      size_hint.y() = std::max( min_size.y(),
                                (item.sizeHint().*_get_fixed_coord)() );
    }

    safeAdd(min_size.x(),  _layout_data.padding);
    safeAdd(max_size.x(),  _layout_data.padding);
    safeAdd(size_hint.x(), _layout_data.padding);

    _layout_data.min_size = min_size.x();
    _layout_data.max_size = max_size.x();
    _layout_data.size_hint = size_hint.x();

    _min_size.x()  = (min_size.*_get_layout_coord)();
    _max_size.x()  = (max_size.*_get_layout_coord)();
    _size_hint.x() = (size_hint.*_get_layout_coord)();

    _min_size.y()  = (min_size.*_get_fixed_coord)();
    _max_size.y()  = (max_size.*_get_fixed_coord)();
    _size_hint.y() = (size_hint.*_get_fixed_coord)();

    _flags &= ~SIZE_INFO_DIRTY;
  }

  //----------------------------------------------------------------------------
  SGVec2i BoxLayout::sizeHintImpl() const
  {
    updateSizeHints();
    return _size_hint;
  }

  //----------------------------------------------------------------------------
  SGVec2i BoxLayout::minimumSizeImpl() const
  {
    updateSizeHints();
    return _min_size;
  }

  //----------------------------------------------------------------------------
  SGVec2i BoxLayout::maximumSizeImpl() const
  {
    updateSizeHints();
    return _max_size;
  }

  //----------------------------------------------------------------------------
  void BoxLayout::doLayout(const SGRecti& geom)
  {
    if( _flags & SIZE_INFO_DIRTY )
      updateSizeHints();

    _layout_data.size = (geom.size().*_get_layout_coord)();
    distribute(_layout_items, _layout_data);

    int fixed_size = (geom.size().*_get_fixed_coord)();
    SGVec2i cur_pos( (geom.pos().*_get_layout_coord)(),
                     (geom.pos().*_get_fixed_coord)() );
    if( _reverse )
      cur_pos.x() += (geom.size().*_get_layout_coord)();

    // TODO handle reverse layouting (rtl/btt)
    for(size_t i = 0; i < _layout_items.size(); ++i)
    {
      ItemData const& data = _layout_items[i];
      cur_pos.x() += _reverse ? -data.padding - data.size : data.padding;

      SGVec2i size(
        data.size,
        std::min( (data.layout_item->maximumSize().*_get_fixed_coord)(),
                  fixed_size )
      );

      // Center in fixed direction (TODO allow specifying alignment)
      int offset_fixed = (fixed_size - size.y()) / 2;
      cur_pos.y() += offset_fixed;

      data.layout_item->setGeometry(SGRecti(
        (cur_pos.*_get_layout_coord)(),
        (cur_pos.*_get_fixed_coord)(),
        (size.*_get_layout_coord)(),
        (size.*_get_fixed_coord)()
      ));

      if( !_reverse )
        cur_pos.x() += data.size;
      cur_pos.y() -= offset_fixed;
    }
  }

  //----------------------------------------------------------------------------
  HBoxLayout::HBoxLayout():
    BoxLayout(LeftToRight)
  {

  }

  //----------------------------------------------------------------------------
  VBoxLayout::VBoxLayout():
    BoxLayout(TopToBottom)
  {

  }

} // namespace canvas
} // namespace simgear
