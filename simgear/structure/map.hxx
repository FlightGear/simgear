///@file
//
// Copyright (C) 2013  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef SG_MAP_HXX_
#define SG_MAP_HXX_

#include <map>
#include <string>

namespace simgear
{

  /**
   * Extended std::map with methods for easier usage.
   */
  template<class Key, class Value>
  class Map:
    public std::map<Key, Value>
  {
    public:
      Map() {}

      /**
       * Initialize a new map with the given key/value pair.
       */
      Map(const Key& key, const Value& value)
      {
        (*this)[key] = value;
      }

      /**
       * Change/add new value.
       */
      Map& operator()(const Key& key, const Value& value)
      {
        (*this)[key] = value;
        return *this;
      }

      /**
       * Retrive a value (or get a default value if it does not exist).
       */
      Value get(const Key& key, const Value& def = Value()) const
      {
        typename Map::const_iterator it = this->find(key);
        if( it != this->end() )
          return it->second;

        return def;
      }
  };

  typedef Map<std::string, std::string> StringMap;

} // namespace simgear

#endif /* SG_MAP_HXX_ */
