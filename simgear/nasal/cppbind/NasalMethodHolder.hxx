///@file
//
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

#ifndef SG_NASAL_METHOD_HOLDER_HXX_
#define SG_NASAL_METHOD_HOLDER_HXX_

#include "NasalContext.hxx"
#include "NasalObjectHolder.hxx"

namespace nasal
{

  /**
   * Hold any callable function in Nasal and call it from C++
   */
  template<class Ret, class... Args>
  class NasalMethodHolder
  {
      using Holder = ObjectHolder<SGReferenced>;

    public:
      NasalMethodHolder(naRef code):
        _code(Holder::makeShared(code))
      {}

      /**
       * @brief Call the function with the given arguments
       *
       * If the first argument is nasal::Me it will be passed as 'me' object and
       * not as argument.
       */
      Ret operator()(Args ... args)
      {
        return call(args...);
      }

    private:
      Holder::Ref _code;

      template<class... CArgs>
      Ret call(Me self, CArgs ... args)
      {
        nasal::Context ctx;
        return ctx.callMethod<Ret, CArgs...>(
          self,
          _code->get_naRef(),
          args...
        );
      }

      template<class... CArgs>
      Ret call(CArgs ... args)
      {
        return call(Me{}, args...);
      }
  };

} // namespace nasal

#endif /* SG_NASAL_METHOD_HOLDER_HXX_ */
