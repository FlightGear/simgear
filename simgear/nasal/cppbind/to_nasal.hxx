///@file
/// Conversion functions to convert C++ types to Nasal types
///
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

#ifndef SG_TO_NASAL_HXX_
#define SG_TO_NASAL_HXX_

#include "nasal_traits.hxx"

#include <simgear/nasal/nasal.h>

#include <boost/utility/enable_if.hpp>
#include <boost/call_traits.hpp>
#include <boost/type_traits.hpp>

#include <map>
#include <string>
#include <vector>

class SGPath;

namespace nasal
{
  class Hash;

  /**
   * Convert std::string to Nasal string
   */
  naRef to_nasal(naContext c, const std::string& str);

  /**
   * Convert C-string to Nasal string
   */
  // We need this to prevent the array overload of to_nasal being called
  naRef to_nasal(naContext c, const char* str);

  /**
   * Convert function pointer to Nasal function
   */
  naRef to_nasal(naContext c, naCFunction func);

  /**
   * Convert a nasal::Hash to a Nasal hash
   */
  naRef to_nasal(naContext c, const Hash& hash);

  /**
   * Simple pass-through of naRef types to allow generic usage of to_nasal
   */
  naRef to_nasal(naContext c, naRef ref);

  naRef to_nasal(naContext c, const SGPath& path);
    
  /**
   * Convert a numeric type to Nasal number
   */
  template<class T>
  typename boost::enable_if< boost::is_arithmetic<T>, naRef >::type
  to_nasal(naContext c, T num)
  {
    return naNum(num);
  }

  /**
   * Convert a 2d vector to Nasal vector with 2 elements
   */
  template<class Vec2>
  typename boost::enable_if<is_vec2<Vec2>, naRef>::type
  to_nasal(naContext c, const Vec2& vec);

  /**
   * Convert a std::map to a Nasal Hash
   */
  template<class Value>
  naRef to_nasal(naContext c, const std::map<std::string, Value>& map);

  /**
   * Convert a fixed size array to a Nasal vector
   */
  template<class T, size_t N>
  naRef to_nasal(naContext c, const T(&array)[N]);

  /**
   * Convert std::vector to Nasal vector
   */
  template< template<class T, class Alloc> class Vector,
            class T,
            class Alloc
          >
  typename boost::enable_if< boost::is_same< Vector<T,Alloc>,
                                             std::vector<T,Alloc>
                                           >,
                             naRef
                           >::type
  to_nasal(naContext c, const Vector<T, Alloc>& vec)
  {
    naRef ret = naNewVector(c);
    naVec_setsize(c, ret, vec.size());
    for(size_t i = 0; i < vec.size(); ++i)
      naVec_set(ret, i, to_nasal(c, vec[i]));
    return ret;
  }

  //----------------------------------------------------------------------------
  template<class Vec2>
  typename boost::enable_if<is_vec2<Vec2>, naRef>::type
  to_nasal(naContext c, const Vec2& vec)
  {
    // We take just double because in Nasal every number is represented as
    // double
    double nasal_vec[2] = {vec[0], vec[1]};
    return to_nasal(c, nasal_vec);
  }

  //----------------------------------------------------------------------------
  template<class Value>
  naRef to_nasal(naContext c, const std::map<std::string, Value>& map)
  {
    naRef hash = naNewHash(c);

    typedef typename boost::call_traits<Value>::param_type param_type;
    typedef typename std::map<std::string, Value>::const_iterator map_iterator;

    for( map_iterator it = map.begin(); it != map.end(); ++it )
      naHash_set
      (
        hash,
        to_nasal(c, it->first),
        to_nasal(c, static_cast<param_type>(it->second))
      );

    return hash;
  }

  //----------------------------------------------------------------------------
  template<class T, size_t N>
  naRef to_nasal(naContext c, const T(&array)[N])
  {
    naRef ret = naNewVector(c);
    naVec_setsize(c, ret, N);
    for(size_t i = 0; i < N; ++i)
      naVec_set(ret, i, to_nasal(c, array[i]));
    return ret;
  }

} // namespace nasal

#endif /* SG_TO_NASAL_HXX_ */
