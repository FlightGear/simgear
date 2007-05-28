/* -*-c++-*-
 *
 * Copyright (C) 2006-2007 Mathias Froehlich 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef SG_VERTEX_ARRAY_BIN_HXX
#define SG_VERTEX_ARRAY_BIN_HXX

#include <vector>
#include <map>

template<typename T>
class SGVertexArrayBin {
public:
  typedef T value_type;
  typedef typename value_type::less less;
  typedef std::vector<value_type> ValueVector;
  typedef typename ValueVector::size_type index_type;
  typedef std::map<value_type, index_type, less> ValueMap;

  index_type insert(const value_type& t)
  {
    typename ValueMap::iterator i = _valueMap.find(t);
    if (i != _valueMap.end())
      return i->second;

    index_type index = _values.size();
    _valueMap[t] = index;
    _values.push_back(t);
    return index;
  }

  const value_type& getVertex(index_type index) const
  { return _values[index]; }

  index_type getNumVertices() const
  { return _values.size(); }

  bool empty() const
  { return _values.empty(); }

private:
  ValueVector _values;
  ValueMap _valueMap;
};

#endif
