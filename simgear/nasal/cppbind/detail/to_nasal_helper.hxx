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
#include <simgear/nasal/cppbind/cppbind_fwd.hxx>
#include <simgear/std/type_traits.hxx>

#include <boost/call_traits.hpp>

#include <array>
#include <functional>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>

class SGGeod;
class SGPath;

namespace nasal
{

  typedef std::function<naRef (CallContext)> free_function_t;

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

  namespace detail
  {
    template<class T>
    naRef array_to_nasal(naContext c, const T* arr, size_t size);
  }

  /**
   * Convert a fixed size array to a Nasal vector
   */
  template<class T, size_t N>
  naRef to_nasal_helper(naContext c, const T(&array)[N])
  {
    return detail::array_to_nasal(c, array, N);
  }

  /**
   * Convert a fixed size C++ array to a Nasal vector
   */
  template<class T, size_t N>
  naRef to_nasal_helper(naContext c, const std::array<T, N>& array)
  {
    return detail::array_to_nasal(c, array.data(), N);
  }

  /**
   * Convert a std::initializer_list to a Nasal vector
   */
  template<class T>
  naRef to_nasal_helper(naContext c, std::initializer_list<T> list)
  {
    return detail::array_to_nasal(c, list.begin(), list.size());
  }

  /**
   * Convert std::vector to a Nasal vector
   */
  template< template<class T, class Alloc> class Vector,
            class T,
            class Alloc
          >
  std::enable_if_t<
    std::is_same<Vector<T,Alloc>, std::vector<T,Alloc>>::value,
    naRef
  >
  to_nasal_helper(naContext c, const Vector<T, Alloc>& vec)
  {
    return detail::array_to_nasal(c, vec.data(), vec.size());
  }

  /**
   * Convert a std::map to a Nasal Hash
   */
  template<class Value>
  naRef to_nasal_helper(naContext c, const std::map<std::string, Value>& map);

  /**
   * Convert an enum value to the according numeric value
   */
  template<class T>
  std::enable_if_t<std::is_enum<T>::value, naRef>
  to_nasal_helper(naContext c, T val)
  {
    return naNum(val);
  }

  /**
   * Convert a numeric type to Nasal number
   */
  template<class T>
  std::enable_if_t<std::is_arithmetic<T>::value, naRef>
  to_nasal_helper(naContext c, T num)
  {
    return naNum(num);
  }

  /**
   * Convert a 2d vector to Nasal vector with 2 elements
   */
  template<class Vec2>
  std::enable_if_t<is_vec2<Vec2>::value, naRef>
  to_nasal_helper(naContext c, const Vec2& vec)
  {
    return to_nasal_helper(c, {
      // We take just double because in Nasal every number is represented as
      // double
      static_cast<double>(vec[0]),
      static_cast<double>(vec[1])
    });
  }

  /**
   * Convert a SGRect to a Nasal vector with position and size of the rect
   */
  template<class T>
  naRef to_nasal_helper(naContext c, const SGRect<T>& rect)
  {
    return to_nasal_helper(c, {
      static_cast<double>(rect.x()),
      static_cast<double>(rect.y()),
      static_cast<double>(rect.width()),
      static_cast<double>(rect.height())
    });
  }

  //----------------------------------------------------------------------------
  template<class T>
  naRef detail::array_to_nasal(naContext c, const T* arr, size_t size)
  {
    naRef ret = naNewVector(c);
    naVec_setsize(c, ret, static_cast<int>(size));
    for(int i = 0; i < static_cast<int>(size); ++i)
      naVec_set(ret, i, to_nasal_helper(c, arr[i]));
    return ret;
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

} // namespace nasal

#endif /* SG_TO_NASAL_HELPER_HXX_ */
