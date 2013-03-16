// Helper functions which created objects with new.
//
// Copyright (C) 2013  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef SG_MAKE_NEW_HXX_
#define SG_MAKE_NEW_HXX_

namespace simgear
{
  template<class T>
  T* make_new()
  { return new T; }

  template<class T, class A1>
  T* make_new(const A1& a1)
  { return new T(a1); }

  template<class T, class A1, class A2>
  T* make_new(const A1& a1, const A2& a2)
  { return new T(a1, a2); }

  template<class Base, class Derived>
  Base* make_new_derived()
  { return new Derived; }

  template<class Base, class Derived, class A1>
  Base* make_new_derived(const A1& a1)
  { return new Derived(a1); }

  template<class Base, class Derived, class A1, class A2>
  Base* make_new_derived(const A1& a1, const A2& a2)
  { return new Derived(a1, a2); }

  // Add more if needed (Variadic templates would be really nice!)

} // namespace simgear

#endif /* SG_MAKE_NEW_HXX_ */
