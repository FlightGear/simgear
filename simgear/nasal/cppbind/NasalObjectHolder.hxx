/// @brief Wrapper class for keeping Nasal objects save from the garbage collector.
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012  Thomas Geymayer <tomgey@gmail.com>

#ifndef SG_NASAL_OBJECT_HOLDER_HXX_
#define SG_NASAL_OBJECT_HOLDER_HXX_

#include <simgear/nasal/nasal.h>
#include <simgear/structure/SGSharedPtr.hxx>

namespace nasal
{

  /**
   * Usable for example as empty base class if a base class is required.(Eg. as
   * parameter for a mixin class).
   */
  struct empty_class {};

  /**
   * Prevent a Nasal object from being destroyed by the garbage collector during
   * the lifetime of this object.
   */
  template<class Base = empty_class>
  class ObjectHolder:
    public Base
  {
    public:

      using Ref = SGSharedPtr<ObjectHolder<Base>>;

      /**
       * @param obj Object to save
       */
      explicit ObjectHolder(naRef obj);

      /**
       *
       */
      ObjectHolder();

      /**
       *
       */
      virtual ~ObjectHolder();

      /**
       * Get captured Nasal object
       */
      naRef get_naRef() const;

      /**
       * Release the managed object
       */
      void reset();

      /**
       * Replaces the managed object (the old object is released)
       *
       * @param obj New object to save
       */
      void reset(naRef obj);

      /**
       * Check if there is a managed object
       */
      bool valid() const;

      /**
       * Save the given object as long as the returned holder exists.
       *
       * @param obj Object to save
       */
      static Ref makeShared(naRef obj);

    protected:
      naRef _ref;
      int _gc_key;
  };

  //----------------------------------------------------------------------------
  template<class Base>
  ObjectHolder<Base>::~ObjectHolder()
  {
    if( !naIsNil(_ref) )
      naGCRelease(_gc_key);
  }

  //----------------------------------------------------------------------------
  template<class Base>
  naRef ObjectHolder<Base>::get_naRef() const
  {
    return _ref;
  }

  //----------------------------------------------------------------------------
  template<class Base>
  void ObjectHolder<Base>::reset()
  {
    if( !naIsNil(_ref) )
      naGCRelease(_gc_key);

    _ref = naNil();
    _gc_key = 0;
  }

  //----------------------------------------------------------------------------
  template<class Base>
  void ObjectHolder<Base>::reset(naRef obj)
  {
    if( !naIsNil(_ref) )
      naGCRelease(_gc_key);

    _ref = obj;
    _gc_key = naGCSave(obj);
  }

  //----------------------------------------------------------------------------
  template<class Base>
  bool ObjectHolder<Base>::valid() const
  {
    return !naIsNil(_ref);
  }

  //----------------------------------------------------------------------------
  template<class Base>
  ObjectHolder<Base>::ObjectHolder(naRef obj):
    _ref(obj),
    _gc_key(naIsNil(obj) ? 0 : naGCSave(obj))
  {

  }

  //----------------------------------------------------------------------------
  template<class Base>
  ObjectHolder<Base>::ObjectHolder():
    _ref(naNil()),
    _gc_key(0)
  {

  }

  //----------------------------------------------------------------------------
  template<class Base>
  typename ObjectHolder<Base>::Ref
  ObjectHolder<Base>::makeShared(naRef obj)
  {
    return Ref( new ObjectHolder<Base>(obj) );
  }

} // namespace nasal

#endif /* SG_NASAL_OBJECT_HOLDER_HXX_ */
