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

#ifndef SG_FROM_NASAL_DETAIL_HXX_
#define SG_FROM_NASAL_DETAIL_HXX_

#include "nasal_traits.hxx"

#include <simgear/nasal/nasal.h>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>

#include <string>
#include <typeinfo> // std::bad_cast
#include <vector>

class SGPath;

namespace nasal
{
  class Hash;

  /**
   * Thrown when converting a type from/to Nasal has failed
   */
  class bad_nasal_cast:
    public std::bad_cast
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

      /**
       * Get a description of the cause of the failed cast.
       */
      virtual const char* what() const throw();

    protected:
      std::string _msg;
  };

  /**
   * Simple pass through for unified handling also of naRef.
   */
  inline naRef from_nasal_helper(naContext, naRef ref, naRef*) { return ref; }

  /**
   * Convert Nasal string to std::string
   */
  std::string from_nasal_helper(naContext c, naRef ref, std::string*);

  /**
   * Convert a Nasal string to an SGPath
   */
  SGPath from_nasal_helper(naContext c, naRef ref, SGPath*);

  /**
   * Convert a Nasal hash to a nasal::Hash
   */
  Hash from_nasal_helper(naContext c, naRef ref, Hash*);

  /**
   * Convert a Nasal number to a C++ numeric type
   */
  template<class T>
  typename boost::enable_if< boost::is_arithmetic<T>,
                             T
                           >::type
  from_nasal_helper(naContext c, naRef ref, T*)
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
  std::vector<T> from_nasal_helper(naContext c, naRef ref, std::vector<T>*)
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
  from_nasal_helper(naContext c, naRef ref, Vec2*)
  {
    std::vector<double> vec =
      from_nasal_helper(c, ref, static_cast<std::vector<double>*>(0));
    if( vec.size() != 2 )
      throw bad_nasal_cast("Expected vector with two elements");
    return Vec2(vec[0], vec[1]);
  }

} // namespace nasal

#endif /* SG_FROM_NASAL_DETAIL_HXX_ */
