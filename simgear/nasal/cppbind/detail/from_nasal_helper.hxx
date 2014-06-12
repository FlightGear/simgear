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
#include <simgear/nasal/nasal.h>
#include <simgear/nasal/cppbind/NasalObjectHolder.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include <boost/bind.hpp>
#include <boost/call_traits.hpp>
#include <boost/function.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>

#include <string>
#include <vector>

class SGPath;

namespace nasal
{
  class Hash;
  class String;

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

      virtual ~bad_nasal_cast() throw();

    protected:
      std::string _msg;
  };

  /**
   * Wrap a naRef to indicate it references the self/me object in Nasal method
   * calls.
   */
  struct Me
  {
    naRef _ref;

    Me(naRef ref):
      _ref(ref)
    {}

    operator naRef() { return _ref; }
  };

  /**
   * Simple pass through for unified handling also of naRef.
   */
  inline naRef from_nasal_helper(naContext, naRef ref, const naRef*)
  { return ref; }

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

  namespace detail
  {
#define SG_BOOST_FUNCTION_FROM_NASAL_FWD
#define BOOST_PP_ITERATION_LIMITS (0, 9)
#define BOOST_PP_FILENAME_1 <simgear/nasal/cppbind/detail/from_nasal_function_templates.hxx>
#include BOOST_PP_ITERATE()
#undef SG_BOOST_FUNCTION_FROM_NASAL_FWD
  }

  /**
   * Convert a Nasal function to a boost::function with the given signature.
   *
   * @tparam Sig    Signature of returned function (arguments and return value
   *                are automatically converted using from_nasal/to_nasal)
   */
  template<class Sig>
  boost::function<Sig>
  from_nasal_helper(naContext c, naRef ref, boost::function<Sig>*)
  {
    if( naIsNil(ref) )
      return boost::function<Sig>();

    if(    !naIsCode(ref)
        && !naIsCCode(ref)
        && !naIsFunc(ref) )
      throw bad_nasal_cast("not a function");

    return detail::boostFunctionFromNasal(ref, static_cast<Sig*>(0));
  }

  template<class Sig>
  typename boost::enable_if< boost::is_function<Sig>,
                             boost::function<Sig>
                           >::type
  from_nasal_helper(naContext c, naRef ref, Sig*)
  {
    return from_nasal_helper(c, ref, static_cast<boost::function<Sig>*>(0));
  }

  /**
   * Convert a Nasal number to a C++ numeric type
   */
  template<class T>
  typename boost::enable_if< boost::is_arithmetic<T>,
                             T
                           >::type
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
   * Convert a Nasal vector of 2 elements to a 2d vector
   */
  template<class Vec2>
  typename boost::enable_if<is_vec2<Vec2>, Vec2>::type
  from_nasal_helper(naContext c, naRef ref, const Vec2*)
  {
    std::vector<double> vec =
      from_nasal_helper(c, ref, static_cast<std::vector<double>*>(0));
    if( vec.size() != 2 )
      throw bad_nasal_cast("Expected vector with two elements");
    return Vec2(vec[0], vec[1]);
  }

  /**
   * Convert a Nasal vector with 4 elements ([x, y, w, h]) to an SGRect
   */
  template<class T>
  SGRect<T> from_nasal_helper(naContext c, naRef ref, const SGRect<T>*)
  {
    std::vector<double> vec =
      from_nasal_helper(c, ref, static_cast<std::vector<double>*>(0));
    if( vec.size() != 4 )
      throw bad_nasal_cast("Expected vector with four elements");

    return SGRect<T>(vec[0], vec[1], vec[2], vec[3]);
  }

  // Helpers for wrapping calls to Nasal functions into boost::function
  namespace detail
  {
    // Dummy include to add a build dependency on this file for gcc/CMake/etc.
#define SG_DONT_DO_ANYTHING
# include <simgear/nasal/cppbind/detail/from_nasal_function_templates.hxx>
#undef SG_DONT_DO_ANYTHING

    // Now the actual include (we are limited to 8 arguments (+me) here because
    // boost::bind has an upper limit of 9)
#define BOOST_PP_ITERATION_LIMITS (0, 8)
#define BOOST_PP_FILENAME_1 <simgear/nasal/cppbind/detail/from_nasal_function_templates.hxx>
#include BOOST_PP_ITERATE()
  }

} // namespace nasal

#endif /* SG_FROM_NASAL_HELPER_HXX_ */
