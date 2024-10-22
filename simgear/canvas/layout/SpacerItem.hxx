// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Element providing blank space in a layout.
 */

#ifndef SG_CANVAS_SPACER_ITEM_HXX_
#define SG_CANVAS_SPACER_ITEM_HXX_

#include "LayoutItem.hxx"

namespace simgear
{
namespace canvas
{
  /**
   * Element for providing blank space in a layout.
   */
  class SpacerItem:
    public LayoutItem
  {
    public:
      SpacerItem( const SGVec2i& size = SGVec2i(0, 0),
                  const SGVec2i& max_size = MAX_SIZE );

  };

using SpacerItemRef = SGSharedPtr<SpacerItem> ;

} // namespace canvas
} // namespace simgear

#endif /* SG_CANVAS_SPACER_ITEM_HXX_ */
