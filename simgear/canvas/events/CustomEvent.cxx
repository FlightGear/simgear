// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Canvas user defined event
 */

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
