// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Element providing blank space in a layout.
 */

#include <simgear_config.h>
#include "SpacerItem.hxx"

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  SpacerItem::SpacerItem( const SGVec2i& size,
                          const SGVec2i& max_size )
  {
    _size_hint = size;
    _min_size  = size;
    _max_size  = max_size;
  }

} // namespace canvas
} // namespace simgear
