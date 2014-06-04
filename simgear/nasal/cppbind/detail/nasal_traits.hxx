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

#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_base_of.hpp>

// Forward declarations
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
  template<class T> class ref_ptr;
  template<class T> class observer_ptr;

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

#define SG_MAKE_SHARED_PTR_TRAIT(ref, weak)\
  template<class T>\
  struct shared_ptr_traits<ref<T> >\
  {\
    typedef ref<T>  strong_ref;\
    typedef weak<T> weak_ref;\
    typedef T       element_type;\
    typedef boost::integral_constant<bool, true> is_strong;\
  };\
  template<class T>\
  struct shared_ptr_traits<weak<T> >\
  {\
    typedef ref<T>  strong_ref;\
    typedef weak<T> weak_ref;\
    typedef T       element_type;\
    typedef boost::integral_constant<bool, false> is_strong;\
  };\
  template<class T>\
  struct is_strong_ref<ref<T> >:\
    public boost::integral_constant<bool, true>\
  {};\
  template<class T>\
  struct is_weak_ref<weak<T> >:\
    public boost::integral_constant<bool, true>\
  {};

  SG_MAKE_SHARED_PTR_TRAIT(SGSharedPtr, SGWeakPtr)
  SG_MAKE_SHARED_PTR_TRAIT(osg::ref_ptr, osg::observer_ptr)
  SG_MAKE_SHARED_PTR_TRAIT(boost::shared_ptr, boost::weak_ptr)

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

} // namespace nasal
#endif /* SG_NASAL_TRAITS_HXX_ */
