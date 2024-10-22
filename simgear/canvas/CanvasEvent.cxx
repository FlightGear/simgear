// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Canvas Event event model similar to DOM Level 3 Event Model
 */

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
  bool Event::isPropagationStopped() const
  {
    return propagation_stopped;
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
