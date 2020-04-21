// Basic class for canvas layouts
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
#include "Layout.hxx"

#include <algorithm>

#include <simgear/debug/logstream.hxx>

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  void Layout::removeItem(const LayoutItemRef& item)
  {
    size_t i = 0;
    while( LayoutItemRef child = itemAt(i) )
    {
      if( item == child )
        return (void)takeAt(i);

      ++i;
    }
  }

  //----------------------------------------------------------------------------
  void Layout::clear()
  {
    while( itemAt(0) )
      takeAt(0);
  }

  //----------------------------------------------------------------------------
  SGRecti Layout::alignmentRect(const SGRecti& geom) const
  {
    return alignment() == AlignFill
         // Without explicit alignment (default == AlignFill) use the whole
         // available space and let the layout and its items distribute the
         // excess space.
         ? geom

         // Otherwise align according to flags.
         : LayoutItem::alignmentRect(geom);
  }

  //----------------------------------------------------------------------------
  void Layout::ItemData::reset()
  {
    layout_item = 0;
    size_hint   = 0;
    min_size    = 0;
    max_size    = 0;
    padding_orig= 0;
    padding     = 0;
    size        = 0;
    stretch     = 0;
    visible     = false;
    has_align   = false;
    has_hfw     = false;
    done        = false;
  }

  //----------------------------------------------------------------------------
  int Layout::ItemData::hfw(int w) const
  {
    if( has_hfw )
      return layout_item->heightForWidth(w);
    else
      return layout_item->sizeHint().y();
  }

  //----------------------------------------------------------------------------
  int Layout::ItemData::mhfw(int w) const
  {
    if( has_hfw )
      return layout_item->minimumHeightForWidth(w);
    else
      return layout_item->minimumSize().y();
  }

  //----------------------------------------------------------------------------
  Layout::Layout():
    _num_not_done(0),
    _sum_stretch(0),
    _space_stretch(0),
    _space_left(0)
  {

  }

  //----------------------------------------------------------------------------
  void Layout::contentsRectChanged(const SGRecti& rect)
  {
    doLayout(rect);

    _flags &= ~LAYOUT_DIRTY;
  }

  //----------------------------------------------------------------------------
  void Layout::distribute(std::vector<ItemData>& items, const ItemData& space)
  {
    const int num_children = static_cast<int>(items.size());
    _num_not_done = 0;

    SG_LOG( SG_GUI,
            SG_DEBUG,
            "Layout::distribute(" << space.size << "px for "
                                  << num_children << " items, s.t."
                                  << " min=" << space.min_size
                                  << ", hint=" << space.size_hint
                                  << ", max=" << space.max_size << ")" );

    if( space.size < space.min_size )
    {
      // TODO
      SG_LOG( SG_GUI, SG_WARN, "Layout: not enough size (not implemented)");
    }
    else if( space.size < space.max_size )
    {
      _sum_stretch = 0;
      _space_stretch = 0;

      bool less_then_hint = space.size < space.size_hint;

      // Give min_size/size_hint to all items
      _space_left = space.size
                  - (less_then_hint ? space.min_size : space.size_hint);
      for(int i = 0; i < num_children; ++i)
      {
        ItemData& d = items[i];
        if( !d.visible )
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
            << ", max=" << d.max_size
        );

        if( d.done )
          continue;
        _num_not_done += 1;

        if( d.stretch > 0 )
        {
          _sum_stretch += d.stretch;
          _space_stretch += d.size;
        }
      }

      // Distribute remaining space to increase the size of each item up to its
      // size_hint/max_size
      while( _space_left > 0 )
      {
        if( _num_not_done <= 0 )
        {
          SG_LOG(SG_GUI, SG_WARN, "space left, but no more items?");
          break;
        }

        int space_per_element = std::max(1, _space_left / _num_not_done);

        SG_LOG(SG_GUI, SG_DEBUG, "space/element=" << space_per_element);

        for(int i = 0; i < num_children; ++i)
        {
          ItemData& d = items[i];
          if( !d.visible )
            continue;

          SG_LOG(
            SG_GUI,
            SG_DEBUG,
            i << ") left=" << _space_left
              << ", not_done=" << _num_not_done
              << ", sum=" << _sum_stretch
              << ", stretch=" << _space_stretch
              << ", stretch/unit=" << _space_stretch / std::max(1, _sum_stretch)
          );

          if( d.done )
            continue;

          if( _sum_stretch > 0 && d.stretch <= 0 )
            d.done = true;
          else
          {
            int target_size = 0;
            int max_size = less_then_hint ? d.size_hint : d.max_size;

            if( _sum_stretch > 0 )
            {
              target_size = (d.stretch * (_space_left + _space_stretch))
                          / _sum_stretch;

              // Item would be smaller than minimum size or larger than maximum
              // size, so just keep bounded size and ignore stretch factor
              if( target_size <= d.size || target_size >= max_size )
              {
                d.done = true;
                _sum_stretch -= d.stretch;
                _space_stretch -= d.size;

                if( target_size >= max_size )
                  target_size = max_size;
                else
                  target_size = d.size;
              }
              else
                _space_stretch += target_size - d.size;
            }
            else
            {
              // Give space evenly to all remaining elements in this round
              target_size = d.size + std::min(_space_left, space_per_element);

              if( target_size >= max_size )
              {
                d.done = true;
                target_size = max_size;
              }
            }

            int old_size = d.size;
            d.size = target_size;
            _space_left -= d.size - old_size;
          }

          if( d.done )
          {
            _num_not_done -= 1;

            if( _sum_stretch <= 0 && d.stretch > 0 )
              // Distribute remaining space evenly to all non-stretchable items
              // in a new round
              break;
          }
        }
      }
    }
    else
    {
      _space_left = space.size - space.max_size;
      int num_align = 0;
      for(int i = 0; i < num_children; ++i)
      {
        if( !items[i].visible )
          continue;

        _num_not_done += 1;

        if( items[i].has_align )
          num_align += 1;
      }

      SG_LOG(
        SG_GUI,
        SG_DEBUG,
        "Distributing excess space:"
             " not_done=" << _num_not_done
         << ", num_align=" << num_align
         << ", space_left=" << _space_left
      );

      for(int i = 0; i < num_children; ++i)
      {
        ItemData& d = items[i];
        if( !d.visible )
          continue;

        d.padding = d.padding_orig;
        d.size = d.max_size;

        int space_add = 0;

        if( d.has_align )
        {
          // Equally distribute superfluous space and let each child items
          // alignment handle the exact usage.
          space_add = _space_left / num_align;
          num_align -= 1;

          d.size += space_add;
        }
        else if( num_align <= 0 )
        {
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
    for(int i = 0; i < num_children; ++i)
    {
      ItemData const& d = items[i];
      if( d.visible )
        SG_LOG(SG_GUI, SG_DEBUG, i << ") pad=" << d.padding
                                   << ", size= " << d.size);
      else
        SG_LOG(SG_GUI, SG_DEBUG, i << ") [hidden]");
    }
  }

} // namespace canvas
} // namespace simgear
