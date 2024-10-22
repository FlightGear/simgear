// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Type Traits (Provide features of later C++ standards)
 */

#ifndef SIMGEAR_STD_TYPE_TRAITS_HXX_
#define SIMGEAR_STD_TYPE_TRAITS_HXX_

#include <simgear/simgear_config.h>
#include <type_traits>

namespace std
{
#ifndef HAVE_STD_REMOVE_CV_T
  template<class T>
  using remove_cv_t = typename remove_cv<T>::type;

  template<class T>
  using remove_const_t = typename remove_const<T>::type;

  template<class T>
  using remove_volatile_t = typename remove_volatile<T>::type;

  template<class T>
  using remove_reference_t = typename remove_reference<T>::type;

  template< class T >
  using remove_pointer_t = typename remove_pointer<T>::type;
#endif

#ifndef HAVE_STD_REMOVE_CVREF_T
  template<class T>
  struct remove_cvref
  {
    using type = remove_cv_t<remove_reference_t<T>>;
  };

  template<class T>
  using remove_cvref_t = typename remove_cvref<T>::type;
#endif

#ifndef HAVE_STD_ENABLE_IF_T
  template<bool B, class T = void>
  using enable_if_t = typename enable_if<B, T>::type;
#endif

#ifndef HAVE_STD_BOOL_CONSTANT
  template <bool B>
  using bool_constant = integral_constant<bool, B>;
#endif
}

#endif /* SIMGEAR_STD_TYPE_TRAITS_HXX_ */
