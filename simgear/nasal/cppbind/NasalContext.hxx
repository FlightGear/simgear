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

#ifndef SG_NASAL_CONTEXT_HXX_
#define SG_NASAL_CONTEXT_HXX_

#include "from_nasal.hxx"
#include "to_nasal.hxx"

namespace nasal
{
  class Hash;
  class String;

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

      /** Create a new nasal vector and fill it with the given values */
      template<class ... Vals>
      naRef newVector(Vals ... vals)
      {
        return newVector({to_nasal(vals)...});
      }

      /** Raise a nasal runtime error */
      template<class ... Args>
      void runtimeError(const char* fmt, Args ... args) const
      {
        naRuntimeError(c_ctx(), fmt, args...);
      }

      template<class T>
      naRef to_nasal(T arg) const
      {
        return nasal::to_nasal(_ctx, arg);
      }

      template<class T, size_t N>
      naRef to_nasal(const T(&array)[N]) const
      {
        return nasal::to_nasal(_ctx, array);
      }

      template<class T>
      typename from_nasal_ptr<T>::return_type
      from_nasal(naRef ref) const
      {
        return (*from_nasal_ptr<T>::get())(_ctx, ref);
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
      ~Context();

      Context(const Context&) = delete;
      Context& operator=(const Context&) = delete;
  };

} // namespace nasal

#endif /* SG_NASAL_CONTEXT_HXX_ */
