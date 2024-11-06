// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014  Thomas Geymayer <tomgey@gmail.com>

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
