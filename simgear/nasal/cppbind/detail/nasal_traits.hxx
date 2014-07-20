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

#include <boost/mpl/logical.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/utility/enable_if.hpp>

// Forward declarations
class SGReferenced;
class SGWeakReferenced;
template<class T> class SGSharedPtr;
template<class T> class SGWeakPtr;
template<class T> class SGVec2;

namespace boost
{
  template<class T> class shared_ptr;
  template<class T> class weak_ptr;
}
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
}

// The actual traits...
namespace nasal
{
  template<class T>
  struct is_vec2: public boost::integral_constant<bool, false> {};

#define SG_MAKE_TRAIT(templ,type,attr)\
  template templ\
  struct attr< type >:\
    public boost::integral_constant<bool, true> {};

SG_MAKE_TRAIT(<class T>, SGVec2<T>, is_vec2)
SG_MAKE_TRAIT(<>, osg::Vec2b, is_vec2)
SG_MAKE_TRAIT(<>, osg::Vec2d, is_vec2)
SG_MAKE_TRAIT(<>, osg::Vec2f, is_vec2)
SG_MAKE_TRAIT(<>, osg::Vec2s, is_vec2)

#undef SG_MAKE_TRAIT

  template<class T>
  struct shared_ptr_traits;

  template<class T>
  struct is_strong_ref:
    public boost::integral_constant<bool, false>
  {};

  template<class T>
  struct is_weak_ref:
    public boost::integral_constant<bool, false>
  {};

#define SG_MAKE_SHARED_PTR_TRAIT(strong, weak, intrusive)\
  template<class T>\
  struct shared_ptr_traits<strong<T> >\
  {\
    typedef strong<T> strong_ref;\
    typedef weak<T>   weak_ref;\
    typedef T         element_type;\
    typedef boost::integral_constant<bool, true> is_strong;\
    typedef boost::integral_constant<bool, intrusive> is_intrusive;\
  };\
  template<class T>\
  struct shared_ptr_traits<weak<T> >\
  {\
    typedef strong<T> strong_ref;\
    typedef weak<T>   weak_ref;\
    typedef T         element_type;\
    typedef boost::integral_constant<bool, false> is_strong;\
    typedef boost::integral_constant<bool, intrusive> is_intrusive;\
  };\
  template<class T>\
  struct is_strong_ref<strong<T> >:\
    public boost::integral_constant<bool, true>\
  {};\
  template<class T>\
  struct is_weak_ref<weak<T> >:\
    public boost::integral_constant<bool, true>\
  {};

  SG_MAKE_SHARED_PTR_TRAIT(SGSharedPtr, SGWeakPtr, true)
  SG_MAKE_SHARED_PTR_TRAIT(osg::ref_ptr, osg::observer_ptr, true)
  SG_MAKE_SHARED_PTR_TRAIT(boost::shared_ptr, boost::weak_ptr, false)

#undef SG_MAKE_SHARED_PTR_TRAIT

  template<class T>
  struct supports_weak_ref:
    public boost::integral_constant<bool, true>
  {};

  template<class T>
  struct supports_weak_ref<SGSharedPtr<T> >:
    public boost::integral_constant<
      bool,
      boost::is_base_of<SGWeakReferenced, T>::value
    >
  {};

  template<class T>
  struct shared_ptr_storage
  {
    typedef T                                           storage_type;
    typedef typename T::element_type                    element_type;
    typedef typename shared_ptr_traits<T>::strong_ref   strong_ref;
    typedef typename shared_ptr_traits<T>::weak_ref     weak_ref;

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
    typename boost::enable_if<
      boost::is_same<U, element_type*>,
      element_type*
    >::type
    get(storage_type* ptr)
    {
      return get_pointer(*ptr);
    }
    template<class U>
    static
    typename boost::enable_if<
      boost::mpl::or_<
        boost::is_same<U, strong_ref>,
        boost::mpl::and_<
          boost::is_same<U, weak_ref>,
          supports_weak_ref<U>
        >
      >,
      U
    >::type
    get(storage_type* ptr)
    {
      return U(*ptr);
    }
    template<class U>
    static
    typename boost::enable_if<
      boost::mpl::and_<
        boost::is_same<U, weak_ref>,
        boost::mpl::not_<supports_weak_ref<U> >
      >,
      U
    >::type
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
      typedef typename T::element_type                    storage_type;
      typedef typename T::element_type                    element_type;
      typedef typename shared_ptr_traits<T>::strong_ref   strong_ref;
      typedef typename shared_ptr_traits<T>::weak_ref     weak_ref;

      template<class U>
      static
      typename boost::enable_if<
        boost::is_same<U, element_type*>,
        element_type*
      >::type
      get(storage_type* ptr)
      {
        return ptr;
      }
      template<class U>
      static
      typename boost::enable_if<
        boost::mpl::or_<
          boost::is_same<U, strong_ref>,
          boost::mpl::and_<
            boost::is_same<U, weak_ref>,
            supports_weak_ref<U>
          >
        >,
        U
      >::type
      get(storage_type* ptr)
      {
        return U(ptr);
      }
      template<class U>
      static
      typename boost::enable_if<
        boost::mpl::and_<
          boost::is_same<U, weak_ref>,
          boost::mpl::not_<supports_weak_ref<U> >
        >,
        U
      >::type
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
    typedef T storage_type;
    typedef T element_type;

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
    typedef T storage_type;
    typedef T element_type;


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
