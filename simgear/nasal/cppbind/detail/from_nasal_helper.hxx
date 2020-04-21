///@file
/// Conversion helpers used by from_nasal<T>(naContext, naRef)
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

#ifndef SG_FROM_NASAL_HELPER_HXX_
#define SG_FROM_NASAL_HELPER_HXX_

#include "nasal_traits.hxx"

#include <simgear/math/SGMath.hxx>
#include <simgear/math/SGRect.hxx>
#include <simgear/nasal/cppbind/NasalContext.hxx>
#include <simgear/nasal/cppbind/NasalMe.hxx>
#include <simgear/nasal/cppbind/NasalMethodHolder.hxx>
#include <simgear/nasal/cppbind/NasalObjectHolder.hxx>
#include <simgear/std/type_traits.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include <array>
#include <functional>
#include <string>
#include <vector>

class SGPath;

namespace nasal
{

  /**
   * Thrown when converting a type from/to Nasal has failed
   */
  class bad_nasal_cast:
    public sg_exception
  {
    public:
      /**
       * Construct with generic error message
       */
      bad_nasal_cast();

      /**
       * Construct from an error message
       *
       * @param msg Error message/description
       */
      explicit bad_nasal_cast(const std::string& msg);

      virtual ~bad_nasal_cast();

    protected:
      std::string _msg;
  };

  /**
   * Simple pass through for unified handling also of naRef.
   */
  inline naRef from_nasal_helper(naContext, naRef ref, const naRef*)
  { return ref; }

  /**
   * Ignore return value
   */
  // TODO show some warning when something is returned but ignored?
  inline void from_nasal_helper(naContext, naRef, const void*)
  {}

  /**
   * Convert Nasal string to std::string
   */
  std::string from_nasal_helper(naContext c, naRef ref, const std::string*);

  /**
   * Convert a Nasal string to an SGPath
   */
  SGPath from_nasal_helper(naContext c, naRef ref, const SGPath*);

  /**
   * Convert a Nasal hash to a nasal::Hash
   */
  Hash from_nasal_helper(naContext c, naRef ref, const Hash*);

  /**
   * Convert a Nasal string to a nasal::String
   */
  String from_nasal_helper(naContext c, naRef ref, const String*);

  /**
   * Convert a Nasal object to bool.
   *
   * @return true, if ref is string or ref is number != 0
   *         false, else
   */
  bool from_nasal_helper(naContext c, naRef ref, const bool*);

  /**
   * Convert a Nasal function to a std::function with the given signature.
   *
   * @tparam Sig    Signature of returned function (arguments and return value
   *                are automatically converted using from_nasal/to_nasal)
   */
  template<class Ret, class... Args>
  std::function<Ret (Args...)>
  from_nasal_helper(naContext c, naRef ref, const std::function<Ret (Args...)>*)
  {
    if( naIsNil(ref) )
      return {};

    if(    !naIsCode(ref)
        && !naIsCCode(ref)
        && !naIsFunc(ref) )
      throw bad_nasal_cast("not a function");

    return NasalMethodHolder<Ret, Args...>(ref);
  }

  template<class Ret, class... Args>
  std::function<Ret (Args...)>
  from_nasal_helper(naContext c, naRef ref, Ret (*const)(Args...))
  {
    return
    from_nasal_helper(c, ref, static_cast<std::function<Ret (Args...)>*>(0));
  }

  /**
   * Convert a Nasal number to a C++ numeric type
   */
  template<class T>
  std::enable_if_t<std::is_arithmetic<T>::value, T>
  from_nasal_helper(naContext c, naRef ref, const T*)
  {
    naRef num = naNumValue(ref);
    if( !naIsNum(num) )
      throw bad_nasal_cast("Not a number");

    return static_cast<T>(num.num);
  }

  /**
   * Convert a Nasal vector to a std::vector
   */
  template<class T>
  std::vector<T>
  from_nasal_helper(naContext c, naRef ref, const std::vector<T>*)
  {
    if( !naIsVector(ref) )
      throw bad_nasal_cast("Not a vector");

    int size = naVec_size(ref);
    std::vector<T> vec(size);

    for(int i = 0; i < size; ++i)
      vec[i] = from_nasal_helper(c, naVec_get(ref, i), static_cast<T*>(0));

    return vec;
  }

  /**
   * Convert a Nasal vector to a std::array
   */
  template<class T, std::size_t N>
  std::array<T, N>
  from_nasal_helper(naContext c, naRef ref, const std::array<T, N>*)
  {
    if( !naIsVector(ref) )
      throw bad_nasal_cast("Not a vector");

    if( naVec_size(ref) != N )
      throw bad_nasal_cast(
        "Expected vector with " + std::to_string(N) + " elements"
      );

    std::array<T, N> arr;

    for(std::size_t i = 0; i < N; ++i)
      arr[i] = from_nasal_helper(c, naVec_get(ref, i), static_cast<T*>(0));

    return arr;
  }

  /**
   * Convert a Nasal vector of 2 elements to a 2d vector
   */
  template<class Vec2>
  std::enable_if_t<is_vec2<Vec2>::value, Vec2>
  from_nasal_helper(naContext c, naRef ref, const Vec2*)
  {
    auto vec =
      from_nasal_helper(c, ref, static_cast<std::array<double, 2>*>(0));

    return Vec2(vec[0], vec[1]);
  }

  template<class Vec4>
  std::enable_if_t<is_vec4<Vec4>::value, Vec4>
  from_nasal_helper(naContext c, naRef ref, const Vec4*)
  {
    auto vec = from_nasal_helper(c, ref, static_cast<std::array<double, 4>*>(0));
    return Vec4(vec[0], vec[1], vec[2], vec[3]);
  }


  /**
   * Convert a Nasal vector with 4 elements ([x, y, w, h]) to an SGRect
   */
  template<class T>
  SGRect<T> from_nasal_helper(naContext c, naRef ref, const SGRect<T>*)
  {
    auto vec =
      from_nasal_helper(c, ref, static_cast<std::array<double, 4>*>(0));

    return SGRect<T>(vec[0], vec[1], vec[2], vec[3]);
  }

} // namespace nasal

#endif /* SG_FROM_NASAL_HELPER_HXX_ */
