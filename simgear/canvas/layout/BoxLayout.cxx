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

#include <simgear_config.h>

#include "BoxLayout.hxx"
#include "SpacerItem.hxx"
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
  BoxLayout::~BoxLayout()
  {
    _parent.reset(); // No need to invalidate parent again...
    clear();
  }

  //----------------------------------------------------------------------------
  void BoxLayout::addItem(const LayoutItemRef& item)
  {
    return addItem(item, 0);
  }

  //----------------------------------------------------------------------------
  void BoxLayout::addItem( const LayoutItemRef& item,
                           int stretch,
                           uint8_t alignment )
  {
    insertItem(-1, item, stretch, alignment);
  }

  //----------------------------------------------------------------------------
  void BoxLayout::addStretch(int stretch)
  {
    insertStretch(-1, stretch);
  }

  //----------------------------------------------------------------------------
  void BoxLayout::addSpacing(int size)
  {
    insertSpacing(-1, size);
  }

  //----------------------------------------------------------------------------
  void BoxLayout::insertItem( int index,
                              const LayoutItemRef& item,
                              int stretch,
                              uint8_t alignment )
  {
    ItemData item_data = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    item_data.layout_item = item;
    item_data.stretch = std::max(0, stretch);

    if( alignment != AlignFill )
      item->setAlignment(alignment);

    if( SGWeakReferenced::count(this) )
      item->setParent(this);
    else
      SG_LOG( SG_GUI,
              SG_WARN,
              "Adding item to expired or non-refcounted layout" );

    if( index < 0 )
      _layout_items.push_back(item_data);
    else
      _layout_items.insert(_layout_items.begin() + index, item_data);

    invalidate();
  }

  //----------------------------------------------------------------------------
  void BoxLayout::insertStretch(int index, int stretch)
  {
    insertItem(index, LayoutItemRef(new SpacerItem()), stretch);
  }

  //----------------------------------------------------------------------------
  void BoxLayout::insertSpacing(int index, int size)
  {
    SGVec2i size_hint = horiz()
                          ? SGVec2i(size, 0)
                          : SGVec2i(0, size),
                max_size  = size_hint;

    insertItem(index, LayoutItemRef(new SpacerItem(size_hint, max_size)));
  }

  //----------------------------------------------------------------------------
  size_t BoxLayout::count() const
  {
    return _layout_items.size();
  }

  //----------------------------------------------------------------------------
  LayoutItemRef BoxLayout::itemAt(size_t index)
  {
    if( index >= _layout_items.size() )
      return LayoutItemRef();

    return _layout_items[index].layout_item;
  }

  //----------------------------------------------------------------------------
  LayoutItemRef BoxLayout::takeAt(size_t index)
  {
    if( index >= _layout_items.size() )
      return LayoutItemRef();

    LayoutItems::iterator it = _layout_items.begin() + index;
    LayoutItemRef item = it->layout_item;
    item->onRemove();
    item->setParent(LayoutItemWeakRef());
    _layout_items.erase(it);

    invalidate();

    return item;
  }

  //----------------------------------------------------------------------------
  void BoxLayout::clear()
  {
    for( LayoutItems::iterator it = _layout_items.begin();
                               it != _layout_items.end();
                             ++it )
    {
      it->layout_item->onRemove();
      it->layout_item->setParent(LayoutItemWeakRef());
    }
    _layout_items.clear();
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
  bool BoxLayout::setStretchFactor(const LayoutItemRef& item, int stretch)
  {
    for( LayoutItems::iterator it = _layout_items.begin();
                               it != _layout_items.end();
                             ++it )
    {
      if( item == it->layout_item )
      {
        it->stretch = std::max(0, stretch);
        invalidate();
        return true;
      }
    }

    return false;
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
    _direction = dir;
    _get_layout_coord = &SGVec2i::x;
    _get_fixed_coord = &SGVec2i::y;

    if( !horiz() )
      std::swap(_get_layout_coord, _get_fixed_coord);

    invalidate();
  }

  //----------------------------------------------------------------------------
  BoxLayout::Direction BoxLayout::direction() const
  {
    return _direction;
  }

  //----------------------------------------------------------------------------
  bool BoxLayout::hasHeightForWidth() const
  {
    if( _flags & SIZE_INFO_DIRTY )
      updateSizeHints();

    return _layout_data.has_hfw;
  }

  //----------------------------------------------------------------------------
  void BoxLayout::setCanvas(const CanvasWeakPtr& canvas)
  {
    _canvas = canvas;

    for(size_t i = 0; i < _layout_items.size(); ++i)
      _layout_items[i].layout_item->setCanvas(canvas);
  }

  //----------------------------------------------------------------------------
  bool BoxLayout::horiz() const
  {
    return (_direction == LeftToRight) || (_direction == RightToLeft);
  }

  //----------------------------------------------------------------------------
  void BoxLayout::updateSizeHints() const
  {
    SGVec2i min_size(0, 0),
            max_size(0, 0),
            size_hint(0, 0);

    _layout_data.reset();
    _hfw_width = _hfw_height = _hfw_min_height = -1;

    bool is_first = true;

    for(size_t i = 0; i < _layout_items.size(); ++i)
    {
      ItemData& item_data = _layout_items[i];
      LayoutItem const& item = *item_data.layout_item;

      item_data.visible = item.isVisible();
      if( !item_data.visible )
        continue;

      item_data.min_size  = (item.minimumSize().*_get_layout_coord)();
      item_data.max_size  = (item.maximumSize().*_get_layout_coord)();
      item_data.size_hint = (item.sizeHint().*_get_layout_coord)();
      item_data.has_hfw = item.hasHeightForWidth();

      uint8_t alignment_mask = horiz()
                             ? AlignHorizontal_Mask
                             : AlignVertical_Mask;
      item_data.has_align = (item.alignment() & alignment_mask) != 0;

      if( !dynamic_cast<SpacerItem*>(item_data.layout_item.get()) )
      {
        if( is_first )
        {
          item_data.padding_orig = 0;
          is_first = false;
        }
        else
        {
          item_data.padding_orig = _padding;
          _layout_data.padding += item_data.padding_orig;
        }
      }

      // Add sizes of all children in layout direction
      SGMisc<int>::addClipOverflowInplace(min_size.x(),  item_data.min_size);
      SGMisc<int>::addClipOverflowInplace(max_size.x(),  item_data.max_size);
      SGMisc<int>::addClipOverflowInplace(size_hint.x(), item_data.size_hint);

      // Take maximum in fixed (non-layouted) direction
      min_size.y()  = std::max( min_size.y(),
                                (item.minimumSize().*_get_fixed_coord)() );
      max_size.y()  = std::max( max_size.y(),
                                (item.maximumSize().*_get_fixed_coord)() );
      size_hint.y() = std::max( size_hint.y(),
                                (item.sizeHint().*_get_fixed_coord)() );

      _layout_data.has_hfw = _layout_data.has_hfw || item.hasHeightForWidth();
    }

    SGMisc<int>::addClipOverflowInplace(min_size.x(),  _layout_data.padding);
    SGMisc<int>::addClipOverflowInplace(max_size.x(),  _layout_data.padding);
    SGMisc<int>::addClipOverflowInplace(size_hint.x(), _layout_data.padding);

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
  void BoxLayout::updateWFHCache(int w) const
  {
    if( w == _hfw_width )
      return;

    _hfw_height = 0;
    _hfw_min_height = 0;

    if( horiz() )
    {
      _layout_data.size = w;
      const_cast<BoxLayout*>(this)->distribute(_layout_items, _layout_data);

      for(size_t i = 0; i < _layout_items.size(); ++i)
      {
        ItemData const& data = _layout_items[i];
        if( !data.visible )
          continue;

        _hfw_height = std::max(_hfw_height, data.hfw(data.size));
        _hfw_min_height = std::max(_hfw_min_height, data.mhfw(data.size));
      }
    }
    else
    {
      for(size_t i = 0; i < _layout_items.size(); ++i)
      {
        ItemData const& data = _layout_items[i];
        if( !data.visible )
          continue;

        _hfw_height += data.hfw(w) + data.padding_orig;
        _hfw_min_height += data.mhfw(w) + data.padding_orig;
      }
    }

    _hfw_width = w;
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
  int BoxLayout::heightForWidthImpl(int w) const
  {
    if( !hasHeightForWidth() )
      return -1;

    updateWFHCache(w);
    return _hfw_height;
  }

  //----------------------------------------------------------------------------
  int BoxLayout::minimumHeightForWidthImpl(int w) const
  {
    if( !hasHeightForWidth() )
      return -1;

    updateWFHCache(w);
    return _hfw_min_height;
  }

  //----------------------------------------------------------------------------
  void BoxLayout::doLayout(const SGRecti& geom)
  {
    if( _flags & SIZE_INFO_DIRTY )
      updateSizeHints();

    // Store current size hints because vertical layouts containing
    // height-for-width items the size hints are update for the actual width of
    // the layout
    int min_size_save = _layout_data.min_size,
        size_hint_save = _layout_data.size_hint;

    _layout_data.size = (geom.size().*_get_layout_coord)();

    // update width dependent data for layouting of vertical layouts
    if( _layout_data.has_hfw && !horiz() )
    {
      for(size_t i = 0; i < _layout_items.size(); ++i)
      {
        ItemData& data = _layout_items[i];
        if( !data.visible )
          continue;

        if( data.has_hfw )
        {
          int w = SGMisc<int>::clip( geom.width(),
                                     data.layout_item->minimumSize().x(),
                                     data.layout_item->maximumSize().x() );

          data.min_size = data.mhfw(w);
          data.size_hint = data.hfw(w);

          // Update size hints for layouting with difference to size hints
          // calculated by using the size hints provided (without trading
          // height for width)
          _layout_data.min_size  += data.min_size
                                  - data.layout_item->minimumSize().y();
          _layout_data.size_hint += data.size_hint
                                  - data.layout_item->sizeHint().y();
        }
      }
    }

    // now do the actual layouting
    distribute(_layout_items, _layout_data);

    // Restore size hints possibly changed by vertical layouting
    _layout_data.min_size = min_size_save;
    _layout_data.size_hint = size_hint_save;

    // and finally set the layouted geometry for each item
    SGVec2i size( 0,
                 // Always assign all available space. Alignment handles final
                 // size.
                 (geom.size().*_get_fixed_coord)() );
    SGVec2i cur_pos( (geom.pos().*_get_layout_coord)(),
                     (geom.pos().*_get_fixed_coord)() );

    bool reverse = (_direction == RightToLeft) || (_direction == BottomToTop);
    if( reverse )
      cur_pos.x() += (geom.size().*_get_layout_coord)();

    for(size_t i = 0; i < _layout_items.size(); ++i)
    {
      ItemData const& data = _layout_items[i];
      if( !data.visible )
        continue;

      cur_pos.x() += reverse ? -data.padding - data.size : data.padding;
      size.x() = data.size;

      data.layout_item->setGeometry(SGRecti(
        (cur_pos.*_get_layout_coord)(),
        (cur_pos.*_get_fixed_coord)(),
        (size.*_get_layout_coord)(),
        (size.*_get_fixed_coord)()
      ));

      if( !reverse )
        cur_pos.x() += data.size;
    }
  }

  //----------------------------------------------------------------------------
  void BoxLayout::visibilityChanged(bool visible)
  {
    for(size_t i = 0; i < _layout_items.size(); ++i)
      callSetVisibleInternal(_layout_items[i].layout_item.get(), visible);
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
