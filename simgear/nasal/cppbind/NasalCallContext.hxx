///@file
/// Call context for Nasal extension functions
///
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef SG_NASAL_CALL_CONTEXT_HXX_
#define SG_NASAL_CALL_CONTEXT_HXX_

#include "from_nasal.hxx"
#include "to_nasal.hxx"

namespace nasal
{

  /**
   * Context passed to a function/method being called from Nasal
   */
  class CallContext
  {
    public:
      CallContext(naContext c, naRef me, size_t argc, naRef* args):
        c(c),
        me(me),
        argc(argc),
        args(args)
      {}

#define SG_CTX_CHECK_ARG(name, check)\
      bool is##name(size_t index) const\
      {\
        return (index < argc && naIs##check(args[index]));\
      }

      SG_CTX_CHECK_ARG(Code,    Code)
      SG_CTX_CHECK_ARG(CCode,   CCode)
      SG_CTX_CHECK_ARG(Func,    Func)
      SG_CTX_CHECK_ARG(Ghost,   Ghost)
      SG_CTX_CHECK_ARG(Hash,    Hash)
      SG_CTX_CHECK_ARG(Nil,     Nil)
      SG_CTX_CHECK_ARG(Numeric, Num)
      SG_CTX_CHECK_ARG(Scalar,  Scalar)
      SG_CTX_CHECK_ARG(String,  String)
      SG_CTX_CHECK_ARG(Vector,  Vector)

#undef SG_CTX_CHECK_ARG

      void popFront(size_t num = 1)
      {
        if( argc < num )
          return;

        args += num;
        argc -= num;
      }

      void popBack(size_t num = 1)
      {
        if( argc < num )
          return;

        argc -= num;
      }

      /**
       * Get the argument with given index if it exists. Otherwise returns the
       * passed default value.
       *
       * @tparam T    Type of argument (converted using ::from_nasal)
       * @param index Index of requested argument
       * @param def   Default value returned if too few arguments available
       */
      template<class T>
      typename from_nasal_ptr<T>::return_type
      getArg(size_t index, const T& def = T()) const
      {
        if( index >= argc )
          return def;

        return from_nasal<T>(args[index]);
      }

      /**
       * Get the argument with given index. Raises a Nasal runtime error if
       * there are to few arguments available.
       */
      template<class T>
      typename from_nasal_ptr<T>::return_type
      requireArg(size_t index) const
      {
        if( index >= argc )
          naRuntimeError(c, "Missing required arg #%d", index);

        return from_nasal<T>(args[index]);
      }

      template<class T>
      naRef to_nasal(T arg) const
      {
        return nasal::to_nasal(c, arg);
      }

      template<class T>
      typename from_nasal_ptr<T>::return_type
      from_nasal(naRef ref) const
      {
        return (*from_nasal_ptr<T>::get())(c, ref);
      }

      naContext   c;
      naRef       me;
      size_t      argc;
      naRef      *args;
  };

} // namespace nasal


#endif /* SG_NASAL_CALL_CONTEXT_HXX_ */
