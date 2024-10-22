// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

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
