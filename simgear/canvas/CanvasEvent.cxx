///@file
/// Canvas Event for event model similar to DOM Level 3 Event Model
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

#include <simgear_config.h>

#include "CanvasEvent.hxx"

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  Event::Event():
    type(UNKNOWN),
    time(-1),
    propagation_stopped(false),
    default_prevented(false)
  {

  }

  //----------------------------------------------------------------------------
  Event::~Event()
  {

  }

  //----------------------------------------------------------------------------
  bool Event::canBubble() const
  {
    return true;
  }

  //----------------------------------------------------------------------------
  int Event::getType() const
  {
    return type;
  }

  //----------------------------------------------------------------------------
  std::string Event::getTypeString() const
  {
    return typeToStr(type);
  }

  //----------------------------------------------------------------------------
  ElementWeakPtr Event::getTarget() const
  {
    return target;
  }

  //----------------------------------------------------------------------------
  ElementWeakPtr Event::getCurrentTarget() const
  {
    return current_target;
  }

  //----------------------------------------------------------------------------
  double Event::getTime() const
  {
    return time;
  }

  //----------------------------------------------------------------------------
  void Event::stopPropagation()
  {
    propagation_stopped = true;
  }

  //----------------------------------------------------------------------------
  void Event::preventDefault()
  {
    default_prevented = true;
  }

  //----------------------------------------------------------------------------
  bool Event::defaultPrevented() const
  {
    return default_prevented;
  }

  //----------------------------------------------------------------------------
  int Event::getOrRegisterType(const std::string& type_str)
  {
    int type = strToType(type_str);

    if( type == UNKNOWN )
    {
      // Register new type
      TypeMap& type_map = getTypeMap();
      type = type_map.size() + 1; // ids start with 1 (after UNKNOWN)
      type_map.insert(TypeMap::value_type(type_str, type));
    }

    return type;
  }

  //----------------------------------------------------------------------------
  int Event::strToType(const std::string& str)
  {
    TypeMap const& type_map = getTypeMap();

    TypeMap::map_by<name>::const_iterator it = type_map.by<name>().find(str);
    if( it == type_map.by<name>().end() )
      return UNKNOWN;
    return it->second;
  }

  //----------------------------------------------------------------------------
  std::string Event::typeToStr(int type)
  {
    auto const& map_by_id = getTypeMap().by<id>();

    auto it = map_by_id.find(type);
    if( it == map_by_id.end() )
      return "unknown";
    return it->second;
  }

  //----------------------------------------------------------------------------
  Event::TypeMap& Event::getTypeMap()
  {
    static TypeMap type_map;

    if( type_map.empty() )
    {
#   define ENUM_MAPPING(type, str, class_name)\
      type_map.insert(TypeMap::value_type(str, type));
#     include "CanvasEventTypes.hxx"
#   undef ENUM_MAPPING
    }

    return type_map;
  }

} // namespace canvas
} // namespace simgear
