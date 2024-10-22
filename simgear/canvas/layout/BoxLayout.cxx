// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Align items horizontally or vertically in a box
 */

#include <simgear_config.h>

#include "BoxLayout.hxx"
#include "SpacerItem.hxx"
#include <simgear/canvas/Canvas.hxx>

namespace simgear
{
namespace canvas
{

//----------------------------------------------------------------------------
void BoxLayout::ItemData::reset()
{
    layout_item = 0;
    size_hint = 0;
    min_size = 0;
    max_size = 0;
    padding_orig = 0;
    padding = 0;
    size = 0;
    stretch = 0;
    visible = false;
    has_align = false;
    has_hfw = false;
    done = false;
}

//----------------------------------------------------------------------------
int BoxLayout::ItemData::hfw(int w) const
{
    if (has_hfw)
        return layout_item->heightForWidth(w);
    else
        return layout_item->sizeHint().y();
}

//----------------------------------------------------------------------------
int BoxLayout::ItemData::mhfw(int w) const
{
    if (has_hfw)
        return layout_item->minimumHeightForWidth(w);
    else
        return layout_item->minimumSize().y();
}

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
  void BoxLayout::distribute(LayoutItems& items, const ItemData& space)
  {
      const int num_children = static_cast<int>(items.size());
      _num_not_done = 0;

      SG_LOG(SG_GUI,
             SG_DEBUG,
             "BoxLayout::distribute(" << space.size << "px for "
                                      << num_children << " items, s.t."
                                      << " min=" << space.min_size
                                      << ", hint=" << space.size_hint
                                      << ", max=" << space.max_size << ")");

      if (space.size < space.min_size) {
          // TODO
          SG_LOG(SG_GUI, SG_WARN, "BoxLayout: not enough size (not implemented)");
      } else if (space.size < space.max_size) {
          _sum_stretch = 0;
          _space_stretch = 0;

          bool less_then_hint = space.size < space.size_hint;

          // Give min_size/size_hint to all items
          _space_left = space.size - (less_then_hint ? space.min_size : space.size_hint);
          for (int i = 0; i < num_children; ++i) {
              ItemData& d = items[i];
              if (!d.visible)
                  continue;

              d.size = less_then_hint ? d.min_size : d.size_hint;
              d.padding = d.padding_orig;
              d.done = d.size >= (less_then_hint ? d.size_hint : d.max_size);

              SG_LOG(
                  SG_GUI,
                  SG_DEBUG,
                  i << ") initial=" << d.size
                    << ", min=" << d.min_size
                    << ", hint=" << d.size_hint
                    << ", max=" << d.max_size);

              if (d.done)
                  continue;
              _num_not_done += 1;

              if (d.stretch > 0) {
                  _sum_stretch += d.stretch;
                  _space_stretch += d.size;
              }
          }

          // Distribute remaining space to increase the size of each item up to its
          // size_hint/max_size
          while (_space_left > 0) {
              if (_num_not_done <= 0) {
                  SG_LOG(SG_GUI, SG_WARN, "space left, but no more items?");
                  break;
              }

              int space_per_element = std::max(1, _space_left / _num_not_done);

              SG_LOG(SG_GUI, SG_DEBUG, "space/element=" << space_per_element);

              for (int i = 0; i < num_children; ++i) {
                  ItemData& d = items[i];
                  if (!d.visible)
                      continue;

                  SG_LOG(
                      SG_GUI,
                      SG_DEBUG,
                      i << ") left=" << _space_left
                        << ", not_done=" << _num_not_done
                        << ", sum=" << _sum_stretch
                        << ", stretch=" << _space_stretch
                        << ", stretch/unit=" << _space_stretch / std::max(1, _sum_stretch));

                  if (d.done)
                      continue;

                  if (_sum_stretch > 0 && d.stretch <= 0)
                      d.done = true;
                  else {
                      int target_size = 0;
                      int max_size = less_then_hint ? d.size_hint : d.max_size;

                      if (_sum_stretch > 0) {
                          target_size = (d.stretch * (_space_left + _space_stretch)) / _sum_stretch;

                          // Item would be smaller than minimum size or larger than maximum
                          // size, so just keep bounded size and ignore stretch factor
                          if (target_size <= d.size || target_size >= max_size) {
                              d.done = true;
                              _sum_stretch -= d.stretch;
                              _space_stretch -= d.size;

                              if (target_size >= max_size)
                                  target_size = max_size;
                              else
                                  target_size = d.size;
                          } else
                              _space_stretch += target_size - d.size;
                      } else {
                          // Give space evenly to all remaining elements in this round
                          target_size = d.size + std::min(_space_left, space_per_element);

                          if (target_size >= max_size) {
                              d.done = true;
                              target_size = max_size;
                          }
                      }

                      int old_size = d.size;
                      d.size = target_size;
                      _space_left -= d.size - old_size;
                  }

                  if (d.done) {
                      _num_not_done -= 1;

                      if (_sum_stretch <= 0 && d.stretch > 0)
                          // Distribute remaining space evenly to all non-stretchable items
                          // in a new round
                          break;
                  }
              }
          }
      } else {
          _space_left = space.size - space.max_size;
          int num_align = 0;
          for (int i = 0; i < num_children; ++i) {
              if (!items[i].visible)
                  continue;

              _num_not_done += 1;

              if (items[i].has_align)
                  num_align += 1;
          }

          SG_LOG(
              SG_GUI,
              SG_DEBUG,
              "Distributing excess space:"
              " not_done="
                  << _num_not_done
                  << ", num_align=" << num_align
                  << ", space_left=" << _space_left);

          for (int i = 0; i < num_children; ++i) {
              ItemData& d = items[i];
              if (!d.visible)
                  continue;

              d.padding = d.padding_orig;
              d.size = d.max_size;

              int space_add = 0;

              if (d.has_align && num_align > 0) {
                  // Equally distribute superfluous space and let each child items
                  // alignment handle the exact usage.
                  space_add = _space_left / num_align;
                  num_align -= 1;

                  d.size += space_add;
              } else if (num_align <= 0) {
                  // Add superfluous space as padding
                  space_add = _space_left
                              // Padding after last child...
                              / (_num_not_done + 1);
                  _num_not_done -= 1;

                  d.padding += space_add;
              }

              _space_left -= space_add;
          }
      }

      SG_LOG(SG_GUI, SG_DEBUG, "distribute:");
      for (int i = 0; i < num_children; ++i) {
          ItemData const& d = items[i];
          if (d.visible)
              SG_LOG(SG_GUI, SG_DEBUG, i << ") pad=" << d.padding << ", size= " << d.size);
          else
              SG_LOG(SG_GUI, SG_DEBUG, i << ") [hidden]");
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
