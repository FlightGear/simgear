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

#ifndef SG_NASAL_OBJECT_HXX_
#define SG_NASAL_OBJECT_HXX_

#include "NasalContext.hxx"
#include "NasalObjectHolder.hxx"
#include "Ghost.hxx"

namespace nasal
{
  /**
   * Object exposed to Nasal including a Nasal hash for data storage.
   */
  class Object:
    public virtual SGVirtualWeakReferenced
  {
    public:

      /**
       *
       * @param impl    Initial implementation hash (nasal part of
       *                implementation)
       */
      Object(naRef impl = naNil());

      void setImpl(naRef obj);
      naRef getImpl() const;

      bool valid() const;

      template<class Ret, class... Args>
      Ret callMethod(const std::string& name, Args ... args)
      {
        if( !_nasal_impl.valid() )
          return Ret();

        Context ctx;
        auto func = get_member<std::function<Ret (Me, Args...)>>(
          ctx, _nasal_impl.get_naRef(), name
        );
        if( func )
          return func(Me(ctx.to_nasal(this)), args...);

        return Ret();
      }

      bool _set(naContext c, const std::string& key, naRef val);
      bool _get(naContext c, const std::string& key, naRef& out);

      static void setupGhost();

    protected:
      ObjectHolder<> _nasal_impl;

  };

  typedef SGSharedPtr<Object> ObjectRef;
  typedef SGWeakPtr<Object> ObjectWeakRef;
  typedef Ghost<ObjectRef> NasalObject;

} // namespace nasal

#endif /* SG_NASAL_OBJECT_HXX_ */
