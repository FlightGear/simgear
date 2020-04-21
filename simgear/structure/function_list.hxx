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

#include <functional>
#include <vector>

namespace simgear
{
  template<typename Sig> class function_list;

  /**
   * Handle a list of callbacks like a single std::function.
   *
   * @tparam Ret    Return type of the callbacks
   * @tparam Args   Parameter types of the callbacks
   */
  template<class Ret, class... Args>
  class function_list<Ret(Args...)>:
    public std::vector<std::function<Ret(Args...)>>
  {
    public:
      Ret operator()(Args ... args) const
      {
        if( this->empty() )
          return Ret();

        auto list_end = --this->end();
        for(auto f = this->begin(); f != list_end; ++f)
          if( *f )
            (*f)(args...);

        return (*list_end) ? (*list_end)(args...) : Ret();
      }
  };

  /**
   * Handle a list of callbacks with the same signature as the given
   * std::function type.
   */
  template<class Ret, class... Args>
  class function_list<std::function<Ret(Args...)>>:
    public function_list<Ret(Args...)>
  {

  };

} // namespace simgear

#endif /* SG_FUNCTION_LIST_HXX_ */
