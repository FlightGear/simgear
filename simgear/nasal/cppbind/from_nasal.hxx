
///@brief Conversion functions to convert Nasal types to C++ types
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

#ifndef SG_FROM_NASAL_HXX_
#define SG_FROM_NASAL_HXX_

#include <simgear/nasal/cppbind/detail/from_nasal_helper.hxx>
#include <type_traits>

namespace nasal
{

  /**
   * Convert a Nasal type to any supported C++ type.
   *
   * @param c   Active Nasal context
   * @param ref Nasal object to be converted
   * @tparam T  Target type of conversion
   *
   * @throws bad_nasal_cast if conversion is not possible
   *
   * @note  Every type which should be supported needs a function with the
   *        following signature declared:
   *
   *        Type from_nasal_helper(naContext, naRef, Type*)
   */
  template<class T>
  T from_nasal(naContext c, naRef ref)
  {
    return from_nasal_helper(c, ref, static_cast<T*>(0));
  }

  /**
   * Get pointer to specific version of from_nasal, converting to a type
   * compatible to \a Var.
   */
  template<class Var>
  struct from_nasal_ptr
  {
    typedef typename std::remove_const
      < typename std::remove_reference<Var>::type
      >::type return_type;
    typedef return_type(*type)(naContext, naRef);

    static type get()
    {
      return &from_nasal<return_type>;
    }
  };

  /**
   * Get member of hash, ghost (also searching in parent objects).
   */
  template<class T>
  T get_member(naContext c, naRef obj, const std::string& name)
  {
    naRef out;
    if( !naMember_get(c, obj, to_nasal(c, name), &out) )
      return T();

    return from_nasal<T>(c, out);
  }

} // namespace nasal

#endif /* SG_FROM_NASAL_HXX_ */
