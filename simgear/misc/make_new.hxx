// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright 2013 (C) Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief  Helper functions which created objects with new.
 */

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
