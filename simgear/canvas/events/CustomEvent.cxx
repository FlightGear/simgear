///@file
/// Canvas user defined event
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
#include "CustomEvent.hxx"

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  CustomEvent::CustomEvent( std::string const& type_str,
                            bool bubbles,
                            StringMap const& data ):
    detail(data),
    bubbles(bubbles)
  {
    type = getOrRegisterType(type_str);
  }

  //----------------------------------------------------------------------------
  CustomEvent::CustomEvent( int type_id,
                            bool bubbles,
                            StringMap const& data ):
    detail(data),
    bubbles(bubbles)
  {
    type = type_id;
//    TypeMap::map_by<id>::type const& type_map = getTypeMap().by<id>();
//    assert( type_map.find(type_id) != type_map.end() );
  }

  //----------------------------------------------------------------------------
  CustomEvent* CustomEvent::clone(int type) const
  {
    auto event = new CustomEvent(*this);
    event->type = type;
    return event;
  }

  //----------------------------------------------------------------------------
  void CustomEvent::setDetail(StringMap const& data)
  {
    detail = data;
  }

} // namespace canvas
} // namespace simgear
