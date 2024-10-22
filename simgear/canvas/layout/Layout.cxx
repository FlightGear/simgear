// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Basic class for canvas layouts
 */

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
  void Layout::contentsRectChanged(const SGRecti& rect)
  {
    doLayout(rect);

    _flags &= ~LAYOUT_DIRTY;
  }


} // namespace canvas
} // namespace simgear
