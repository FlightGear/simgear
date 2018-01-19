///@file
/// Nasal C++ Bindings forward declarations
///
// Copyright (C) 2018  Thomas Geymayer <tomgey@gmail.com>
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
