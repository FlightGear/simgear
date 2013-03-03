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

#include <simgear/nasal/cppbind/detail/to_nasal_helper.hxx>

namespace nasal
{
  /**
   * Convert any supported C++ type to Nasal.
   *
   * @param c   Active Nasal context
   * @param arg C++ Object to be converted
   * @tparam T  Type of converted object
   *
   * @throws bad_nasal_cast if conversion is not possible
   *
   * @note  Every type which should be supported needs a function with the
   *        following signature declared (Type needs to be a const reference
   *        for non-integral types):
   *
   *        naRef to_nasal_helper(naContext, Type)
   */
  template<class T>
  naRef to_nasal(naContext c, T arg)
  {
    return to_nasal_helper(c, arg);
  }

  template<class T, size_t N>
  naRef to_nasal(naContext c, const T(&array)[N])
  {
    return to_nasal_helper(c, array);
  }

  /**
   * Wrapper to get pointer to specific version of to_nasal applicable to given
   * type.
   */
  template<class Var>
  struct to_nasal_ptr
  {
    typedef typename boost::call_traits<Var>::param_type param_type;
    typedef naRef(*type)(naContext, param_type);

    static type get()
    {
      return &to_nasal<param_type>;
    }
  };
} // namespace nasal

#endif /* SG_TO_NASAL_HXX_ */
