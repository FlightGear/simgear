// Canvas Event for event model similar to DOM Level 3 Event Model
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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

#include "CanvasEvent.hxx"

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  Event::Event():
    type(UNKNOWN),
    propagation_stopped(false)
  {

  }

  //----------------------------------------------------------------------------
  Event::~Event()
  {

  }

  //----------------------------------------------------------------------------
  Event::Type Event::getType() const
  {
    return type;
  }

  //----------------------------------------------------------------------------
  std::string Event::getTypeString() const
  {
    switch( type )
    {
#     define ENUM_MAPPING(name, str) case name: return str;
#       include "CanvasEventTypes.hxx"
#     undef ENUM_MAPPING
      default:
        return "unknown";
    }
  }

  //----------------------------------------------------------------------------
  ElementWeakPtr Event::getTarget() const
  {
    return target;
  }

  //----------------------------------------------------------------------------
  void Event::stopPropagation()
  {
    propagation_stopped = true;
  }

  //----------------------------------------------------------------------------
  Event::Type Event::strToType(const std::string& str)
  {
    typedef std::map<std::string, Type> TypeMap;
    static TypeMap type_map;

    if( type_map.empty() )
    {
#     define ENUM_MAPPING(type, str) type_map[ str ] = type;
#       include "CanvasEventTypes.hxx"
#     undef ENUM_MAPPING
    }

    TypeMap::const_iterator it = type_map.find(str);
    if( it == type_map.end() )
      return UNKNOWN;

    return it->second;
  }

} // namespace canvas
} // namespace simgear
