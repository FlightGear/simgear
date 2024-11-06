// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2018  Thomas Geymayer <tomgey@gmail.com>


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
