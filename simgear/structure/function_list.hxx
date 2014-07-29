///@file
//
// Copyright (C) 2014  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef SG_FUNCTION_LIST_HXX_
#define SG_FUNCTION_LIST_HXX_

#include <boost/function.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/utility/enable_if.hpp>
#include <vector>

namespace simgear
{
  template<typename Sig> class function_list;

  // Build dependency for CMake, gcc, etc.
# define SG_DONT_DO_ANYTHING
#  include <simgear/structure/detail/function_list_template.hxx>
# undef SG_DONT_DO_ANYTHING

# define BOOST_PP_ITERATION_LIMITS (0, 3)
# define BOOST_PP_FILENAME_1 <simgear/structure/detail/function_list_template.hxx>
# include BOOST_PP_ITERATE()

  /**
   * Handle a list of callbacks like a single boost::function.
   *
   * @tparam Sig    Function signature.
   */
  template<typename Sig>
  class function_list<boost::function<Sig> >:
    public function_list<Sig>
  {

  };

} // namespace simgear

#endif /* SG_FUNCTION_LIST_HXX_ */
