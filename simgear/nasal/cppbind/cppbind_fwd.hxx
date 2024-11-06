///@brief Nasal C++ Bindings forward declarations
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2018 Thomas Geymayer <tomgey@gmail.com>

#ifndef SG_NASAL_CPPBIND_FWD_HXX_
#define SG_NASAL_CPPBIND_FWD_HXX_

#include <simgear/nasal/nasal.h>
#include <cstddef>
#include <string>

namespace nasal
{

  class bad_nasal_cast;

  class CallContext;
  class Context;
  class ContextWrapper;
  class Hash;
  struct Me;
  class Object;
  class String;

  template<class, class>
  class Ghost;

  template<class T>
  naRef to_nasal(naContext c, T arg);

  template<class T, std::size_t N>
  naRef to_nasal(naContext c, const T(&array)[N]);

  template<class T>
  T from_nasal(naContext c, naRef ref);

  template<class Var>
  struct from_nasal_ptr;

  template<class T>
  T get_member(naContext c, naRef obj, const std::string& name);

} // namespace nasal

#endif /* SG_NASAL_CPPBIND_FWD_HXX_ */
