///@file
/// Metaprogramming Integer sequence (Is in C++14 but not C++11)
//
// Copyright (C) 2017  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef SIMGEAR_STD_INTEGER_SEQUENCE_HXX_
#define SIMGEAR_STD_INTEGER_SEQUENCE_HXX_

#include <simgear/simgear_config.h>
#include "type_traits.hxx"

#include <utility>

#ifndef HAVE_STD_INDEX_SEQUENCE
# include <cstddef>

namespace std
{
  template<class T, T... Ints>
  struct integer_sequence
  {
    static_assert(
      std::is_integral<T>::value,
      "std::integer_sequence can only be instantiated with an an integral type"
    );

    typedef T value_type;
    static constexpr size_t size() noexcept { return sizeof...(Ints); }
  };
}

namespace simgear { namespace detail
{
  template<class T, class Seq, T El>
  struct append;

  template<class T, T... Ints, T Int>
  struct append<T, std::integer_sequence<T, Ints...>, Int>
  {
    using type = std::integer_sequence<T, Ints..., Int>;
  };

  template<class T, std::size_t N>
  struct sequence_gen
  {
    using type =
      typename append<T, typename sequence_gen<T, N - 1>::type, N - 1>::type;
  };

  template<class T>
  struct sequence_gen<T, 0>
  {
    using type = std::integer_sequence<T>;
  };
}}

namespace std
{
  template<size_t... Ints>
  using index_sequence = integer_sequence<size_t, Ints...>;

  template<class T, size_t N>
  using make_integer_sequence =
    typename simgear::detail::sequence_gen<T, N>::type;

  template<size_t N>
  using make_index_sequence = make_integer_sequence<size_t, N>;

  template<class... T>
  using index_sequence_for = make_index_sequence<sizeof...(T)>;
}
#endif

#endif /* SIMGEAR_STD_INTEGER_SEQUENCE_HXX_ */
