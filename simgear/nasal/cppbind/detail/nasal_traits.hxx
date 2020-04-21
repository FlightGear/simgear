///@file
/// Type traits used for converting from and to Nasal types
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

#ifndef SG_NASAL_TRAITS_HXX_
#define SG_NASAL_TRAITS_HXX_

#include <memory>
#include <simgear/std/type_traits.hxx>

// Forward declarations
class SGReferenced;
class SGWeakReferenced;
template<class T> class SGSharedPtr;
template<class T> class SGWeakPtr;
template<class T> class SGVec2;
template<class T> class SGVec4;

namespace osg
{
  class Referenced;
  template<class T> class ref_ptr;
  template<class T> class observer_ptr;

  template<class T, class Y>
  ref_ptr<T> static_pointer_cast(const ref_ptr<Y>&);

  class Vec2b;
  class Vec2d;
  class Vec2f;
  class Vec2s;

  class Vec4f;
  class Vec4d;
}

// The actual traits...
namespace nasal
{
  template<class T>
  struct is_vec2: public std::false_type {};

  template<class T>
  struct is_vec4: public std::false_type {};


#define SG_MAKE_TRAIT(templ,type,attr)\
  template templ\
  struct attr< type >: public std::true_type {};

SG_MAKE_TRAIT(<class T>, SGVec2<T>, is_vec2)
SG_MAKE_TRAIT(<>, osg::Vec2b, is_vec2)
SG_MAKE_TRAIT(<>, osg::Vec2d, is_vec2)
SG_MAKE_TRAIT(<>, osg::Vec2f, is_vec2)
SG_MAKE_TRAIT(<>, osg::Vec2s, is_vec2)

SG_MAKE_TRAIT(<class T>, SGVec4<T>, is_vec4)
SG_MAKE_TRAIT(<>, osg::Vec4d, is_vec4)
SG_MAKE_TRAIT(<>, osg::Vec4f, is_vec4)

#undef SG_MAKE_TRAIT

  template<class T>
  struct shared_ptr_traits;

  template<class T>
  struct is_strong_ref: public std::false_type {};

  template<class T>
  struct is_weak_ref: public std::false_type {};

#define SG_MAKE_SHARED_PTR_TRAIT(strong, weak, intrusive)\
  template<class T>\
  struct shared_ptr_traits<strong<T> >\
  {\
    using strong_ref    = strong<T>;\
    using weak_ref      = weak<T>;\
    using element_type  = T;\
    using is_strong     = std::true_type;\
    using is_intrusive  = std::bool_constant<intrusive>;\
  };\
  template<class T>\
  struct shared_ptr_traits<weak<T> >\
  {\
    using strong_ref    = strong<T>;\
    using weak_ref      = weak<T>;\
    using element_type  = T;\
    using is_strong     = std::false_type;\
    using is_intrusive  = std::bool_constant<intrusive>;\
  };\
  template<class T>\
  struct is_strong_ref<strong<T> >: public std::true_type {};\
  template<class T>\
  struct is_weak_ref<weak<T> >: public std::true_type {};

  SG_MAKE_SHARED_PTR_TRAIT(SGSharedPtr, SGWeakPtr, true)
  SG_MAKE_SHARED_PTR_TRAIT(osg::ref_ptr, osg::observer_ptr, true)
  SG_MAKE_SHARED_PTR_TRAIT(std::shared_ptr, std::weak_ptr, false)

#undef SG_MAKE_SHARED_PTR_TRAIT

  template<class T>
  struct supports_weak_ref: public std::true_type {};

  template<class T>
  struct supports_weak_ref<SGSharedPtr<T> >:
    public std::bool_constant<std::is_base_of<SGWeakReferenced, T>::value>
  {};

  template<class T>
  struct shared_ptr_storage
  {
    using storage_type  = T;
    using element_type  = typename T::element_type;
    using strong_ref    = typename shared_ptr_traits<T>::strong_ref;
    using weak_ref      = typename shared_ptr_traits<T>::weak_ref;

    template<class U>
    static storage_type* ref(U ptr)
    {
      return new storage_type(ptr);
    }
    static void unref(storage_type* ptr)
    {
      delete ptr;
    }

    template<class U>
    static
    std::enable_if_t<std::is_same<U, element_type*>::value, element_type*>
    get(storage_type* ptr)
    {
      return get_pointer(*ptr);
    }

    template<class U>
    static
    std::enable_if_t<
          std::is_same<U, strong_ref>::value
      || (std::is_same<U, weak_ref>::value && supports_weak_ref<U>::value),
      U
    >
    get(storage_type* ptr)
    {
      return U(*ptr);
    }

    template<class U>
    static
    std::enable_if_t<
      std::is_same<U, weak_ref>::value && !supports_weak_ref<U>::value,
      U
    >
    get(storage_type* ptr)
    {
      return U();
    }
  };

  namespace internal
  {
    template<class T>
    struct intrusive_ptr_storage
    {
      using storage_type    = typename T::element_type;
      using element_type    = typename T::element_type;
      using strong_ref      = typename shared_ptr_traits<T>::strong_ref;
      using weak_ref        = typename shared_ptr_traits<T>::weak_ref;

      template<class U>
      static
      std::enable_if_t<std::is_same<U, element_type*>::value, element_type*>
      get(storage_type* ptr)
      {
        return ptr;
      }

      template<class U>
      static
      std::enable_if_t<
            std::is_same<U, strong_ref>::value
        || (std::is_same<U, weak_ref>::value && supports_weak_ref<U>::value),
        U
      >
      get(storage_type* ptr)
      {
        return U(ptr);
      }

      template<class U>
      static
      std::enable_if_t<
        std::is_same<U, weak_ref>::value && !supports_weak_ref<U>::value,
        U
      >
      get(storage_type* ptr)
      {
        return U();
      }
    };
  }

  template<class T>
  struct shared_ptr_storage<SGSharedPtr<T> >:
    public internal::intrusive_ptr_storage<SGSharedPtr<T> >
  {
    using storage_type  = T;
    using element_type  = T;

    static storage_type* ref(element_type* ptr)
    {
      T::get(ptr);
      return ptr;
    }
    static void unref(storage_type* ptr)
    {
      if( !T::put(ptr) )
        delete ptr;
    }
  };

  template<class T>
  struct shared_ptr_storage<osg::ref_ptr<T> >:
    public internal::intrusive_ptr_storage<osg::ref_ptr<T> >
  {
    using storage_type  = T;
    using element_type  = T;


    static storage_type* ref(element_type* ptr)
    {
      ptr->ref();
      return ptr;
    }
    static void unref(storage_type* ptr)
    {
      ptr->unref();
    }
  };

} // namespace nasal
#endif /* SG_NASAL_TRAITS_HXX_ */
