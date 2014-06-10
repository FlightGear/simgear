///@file
/// Conversion helpers used by to_nasal<T>(naContext, T)
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

#ifndef SG_TO_NASAL_HELPER_HXX_
#define SG_TO_NASAL_HELPER_HXX_

#include "nasal_traits.hxx"

#include <simgear/math/SGMath.hxx>
#include <simgear/math/SGRect.hxx>
#include <simgear/nasal/nasal.h>

#include <boost/function/function_fwd.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/call_traits.hpp>
#include <boost/type_traits.hpp>

#include <map>
#include <string>
#include <vector>

class SGGeod;
class SGPath;

namespace nasal
{
  class CallContext;
  class Hash;

  typedef boost::function<naRef (CallContext)> free_function_t;

  /**
   * Convert std::string to Nasal string
   */
  naRef to_nasal_helper(naContext c, const std::string& str);

  /**
   * Convert C-string to Nasal string
   */
  // We need this to prevent the array overload of to_nasal being called
  naRef to_nasal_helper(naContext c, const char* str);

  /**
   * Convert a nasal::Hash to a Nasal hash
   */
  naRef to_nasal_helper(naContext c, const Hash& hash);

  /**
   * Simple pass-through of naRef types to allow generic usage of to_nasal
   */
  naRef to_nasal_helper(naContext c, const naRef& ref);

  naRef to_nasal_helper(naContext c, const SGGeod& pos);

  naRef to_nasal_helper(naContext c, const SGPath& path);

  /**
   * Convert function pointer to Nasal function
   */
  naRef to_nasal_helper(naContext c, naCFunction func);

  naRef to_nasal_helper(naContext c, const free_function_t& func);

  /**
   * Convert an enum value to the according numeric value
   */
  template<class T>
  typename boost::enable_if< boost::is_enum<T>, naRef >::type
  to_nasal_helper(naContext c, T val)
  {
    return naNum(val);
  }

  /**
   * Convert a numeric type to Nasal number
   */
  template<class T>
  typename boost::enable_if< boost::is_arithmetic<T>, naRef >::type
  to_nasal_helper(naContext c, T num)
  {
    return naNum(num);
  }

  /**
   * Convert a 2d vector to Nasal vector with 2 elements
   */
  template<class Vec2>
  typename boost::enable_if<is_vec2<Vec2>, naRef>::type
  to_nasal_helper(naContext c, const Vec2& vec);

  /**
   * Convert a std::map to a Nasal Hash
   */
  template<class Value>
  naRef to_nasal_helper(naContext c, const std::map<std::string, Value>& map);

  /**
   * Convert a fixed size array to a Nasal vector
   */
  template<class T, size_t N>
  naRef to_nasal_helper(naContext c, const T(&array)[N]);

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
  to_nasal_helper(naContext c, const Vector<T, Alloc>& vec)
  {
    naRef ret = naNewVector(c);
    naVec_setsize(c, ret, static_cast<int>(vec.size()));
    for(int i = 0; i < static_cast<int>(vec.size()); ++i)
      naVec_set(ret, i, to_nasal_helper(c, vec[i]));
    return ret;
  }

  //----------------------------------------------------------------------------
  template<class Vec2>
  typename boost::enable_if<is_vec2<Vec2>, naRef>::type
  to_nasal_helper(naContext c, const Vec2& vec)
  {
    // We take just double because in Nasal every number is represented as
    // double
    double nasal_vec[2] = {
      static_cast<double>(vec[0]),
      static_cast<double>(vec[1])
    };
    return to_nasal_helper(c, nasal_vec);
  }

  //----------------------------------------------------------------------------
  template<class T>
  naRef to_nasal_helper(naContext c, const SGRect<T>& rect)
  {
    std::vector<double> vec(4);
    vec[0] = rect.x();
    vec[1] = rect.y();
    vec[2] = rect.width();
    vec[3] = rect.height();

    return to_nasal_helper(c, vec);
  }

  //----------------------------------------------------------------------------
  template<class Value>
  naRef to_nasal_helper(naContext c, const std::map<std::string, Value>& map)
  {
    naRef hash = naNewHash(c);

    typedef typename boost::call_traits<Value>::param_type param_type;
    typedef typename std::map<std::string, Value>::const_iterator map_iterator;

    for( map_iterator it = map.begin(); it != map.end(); ++it )
      naHash_set
      (
        hash,
        to_nasal_helper(c, it->first),
        to_nasal_helper(c, static_cast<param_type>(it->second))
      );

    return hash;
  }

  //----------------------------------------------------------------------------
  template<class T, size_t N>
  naRef to_nasal_helper(naContext c, const T(&array)[N])
  {
    naRef ret = naNewVector(c);
    naVec_setsize(c, ret, static_cast<int>(N));
    for(int i = 0; i < static_cast<int>(N); ++i)
      naVec_set(ret, i, to_nasal_helper(c, array[i]));
    return ret;
  }

} // namespace nasal

#endif /* SG_TO_NASAL_HELPER_HXX_ */
