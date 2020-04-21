///@file
/// Compare lists and get differences
//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//
///
// Changes Copyright (C) 2019    Richard Harrison (rjh@zaretto.com)
//
// As the boost licence is lax and permissive see 
// (https://www.gnu.org/licenses/license-list.en.html#boost)
// any changes to this module are covered under the GPL
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

#ifndef LRU_CACHE_HXX_
#define LRU_CACHE_HXX_

#include <map>
#include <list>
#include <utility>
#include <mutex>

#include <boost/optional.hpp>
#include <simgear/threads/SGThread.hxx>

namespace simgear
{
    // a cache which evicts the least recently used item when it is full
    template<class Key, class Value>
    class lru_cache
    {
    public:
        std::mutex _mutex;

        typedef Key key_type;
        typedef Value value_type;
        typedef std::list<key_type> list_type;
        typedef std::map<
            key_type,
            std::pair<value_type, typename list_type::iterator>
        > map_type;

        lru_cache(size_t capacity)
            : m_capacity(capacity)
        {
        }

        ~lru_cache()
        {
        }


        size_t size() const
        {
            return m_map.size();
        }

        size_t capacity() const
        {
            return m_capacity;
        }

        bool empty() const
        {
            return m_map.empty();
        }

        bool contains(const key_type &key)
        {
            std::lock_guard<std::mutex> scopeLock(_mutex);
            return m_map.find(key) != m_map.end();
        }

        void insert(const key_type &key, const value_type &value)
        {
            std::lock_guard<std::mutex> scopeLock(_mutex);
            typename map_type::iterator i = m_map.find(key);
            if (i == m_map.end()) {
                // insert item into the cache, but first check if it is full
                if (size() >= m_capacity) {
                    // cache is full, evict the least recently used item
                    evict();
                }

                // insert the new item
                m_list.push_front(key);
                m_map[key] = std::make_pair(value, m_list.begin());
            }
        }
        boost::optional<key_type> findValue(const std::string &requiredValue)
        {
            std::lock_guard<std::mutex> scopeLock(_mutex);
            for (typename map_type::iterator it = m_map.begin(); it != m_map.end(); ++it)
                if (it->second.first == requiredValue)
                    return it->first;
            return boost::none;
        }
        boost::optional<value_type> get(const key_type &key)
        {
            std::lock_guard<std::mutex> scopeLock(_mutex);
            // lookup value in the cache
            typename map_type::iterator i = m_map.find(key);
            if (i == m_map.end()) {
                // value not in cache
                return boost::none;
            }

            // return the value, but first update its place in the most
            // recently used list
            typename list_type::iterator j = i->second.second;
            if (j != m_list.begin()) {
                // move item to the front of the most recently used list
                m_list.erase(j);
                m_list.push_front(key);

                // update iterator in map
                j = m_list.begin();
                const value_type &value = i->second.first;
                m_map[key] = std::make_pair(value, j);

                // return the value
                return value;
            }
            else {
                // the item is already at the front of the most recently
                // used list so just return it
                return i->second.first;
            }
        }

        void clear()
        {
            std::lock_guard<std::mutex> scopeLock(_mutex);
            m_map.clear();
            m_list.clear();
        }

    private:
        void evict()
        {
            std::lock_guard<std::mutex> scopeLock(_mutex);
            // evict item from the end of most recently used list
            typename list_type::iterator i = --m_list.end();
            m_map.erase(*i);
            m_list.erase(i);
        }

    private:
        map_type m_map;
        list_type m_list;
        size_t m_capacity;
    };
} // namespace simgear

#endif /* SG_LISTDIFF_HXX_ */
