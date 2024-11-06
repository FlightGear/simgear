// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014  Thomas Geymayer <tomgey@gmail.com>


#ifndef SG_NASAL_CONTEXT_HXX_
#define SG_NASAL_CONTEXT_HXX_

#include "cppbind_fwd.hxx"
#include "NasalMe.hxx"

#include <boost/call_traits.hpp>
#include <initializer_list>

namespace nasal
{

  /**
   * Wraps a nasal ::naContext without taking ownership/managing its lifetime
   */
  class ContextWrapper
  {
    public:
      explicit ContextWrapper(naContext ctx);

      operator naContext();

      /** Convert to non-const ::naContext for usage with C-APIs */
      naContext c_ctx() const;

      Hash newHash();
      String newString(const char* str);

      /** Raise a nasal runtime error */
      template<class... Args>
      void runtimeError(const char* fmt, Args ... args) const
      {
        naRuntimeError(c_ctx(), fmt, args...);
      }

      template<class T>
      naRef to_nasal(T arg) const
      {
        return nasal::to_nasal(_ctx, arg);
      }

      template<class T, std::size_t N>
      naRef to_nasal(const T(&array)[N]) const
      {
        return nasal::to_nasal(_ctx, array);
      }

      /** Create a nasal vector filled with the given values */
      template<class... Vals>
      naRef to_nasal_vec(Vals ... vals)
      {
        return newVector({to_nasal(vals)...});
      }

      template<class T>
      Me to_me(T arg) const
      {
        return Me{ to_nasal(arg) };
      }

      template<class T>
      typename from_nasal_ptr<T>::return_type
      from_nasal(naRef ref) const
      {
        return (*from_nasal_ptr<T>::get())(_ctx, ref);
      }

      naRef callMethod(Me me, naRef code, std::initializer_list<naRef> args);

      template<class Ret, class... Args>
      Ret callMethod( Me me,
                      naRef code,
                      typename boost::call_traits<Args>::param_type ... args )
      {
        // TODO warn if with Ret == void something different to nil is returned?
        return from_nasal<Ret>(callMethod(
          me,
          code,
          { to_nasal<typename boost::call_traits<Args>::param_type>(args)... }
        ));
      }

    protected:
      naContext _ctx;

      // Not exposed to avoid confusion of intializer_list<naRef> to variadic
      // arguments
      naRef newVector(std::initializer_list<naRef> vals);
  };

  /**
   * Creates and manages the lifetime of a ::naContext
   */
  class Context:
    public ContextWrapper
  {
    public:
      Context();
      virtual ~Context();

      Context(const Context&) = delete;
      Context& operator=(const Context&) = delete;
  };

} // namespace nasal

#include "from_nasal.hxx"
#include "to_nasal.hxx"

#endif /* SG_NASAL_CONTEXT_HXX_ */
