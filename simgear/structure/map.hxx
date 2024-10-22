// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 Thomas Geymayer <tomgey@gmail.com>

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
