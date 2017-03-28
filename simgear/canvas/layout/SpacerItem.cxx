// Element providing blank space in a layout.
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
