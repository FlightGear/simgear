// OSGUtils.hxx - Useful templates for interfacing to Open Scene Graph
//
// Copyright (C) 2008  Tim Moore timoore@redhat.com
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
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.

#ifndef SIMGEAR_OSGUTILS_HXX
#define SIMGEAR_OSGUTILS_HXX 1

#include <boost/iterator/iterator_facade.hpp>
#include <osg/CopyOp>

namespace simgear
{
// RefPtrAdapter also appears in OpenSceneGraph's
// osgDB/DatabasePager.cpp. I wrote that code too. -- Tim Moore

// Convert function objects that take pointer args into functions that a
// reference to an osg::ref_ptr. This is quite useful for doing STL
// operations on lists of ref_ptr. This code assumes that a function
// with an argument const Foo* should be composed into a function of
// argument type ref_ptr<Foo>&, not ref_ptr<const Foo>&. Some support
// for that should be added to make this more general.
template <typename U>
struct PointerTraits
{
    typedef class NullType {} PointeeType;
};

template <typename U>
struct PointerTraits<U*>
{
    typedef U PointeeType;
};

template <typename U>
struct PointerTraits<const U*>
{
    typedef U PointeeType;
};

template <typename FuncObj>
class RefPtrAdapter
    : public std::unary_function<const osg::ref_ptr<typename PointerTraits<typename FuncObj::argument_type>::PointeeType>,
                                 typename FuncObj::result_type>
{
public:
    typedef typename PointerTraits<typename FuncObj::argument_type>::PointeeType PointeeType;
    typedef osg::ref_ptr<PointeeType> RefPtrType;
    explicit RefPtrAdapter(const FuncObj& funcObj) : _func(funcObj) {}
    typename FuncObj::result_type operator()(const RefPtrType& refPtr) const
    {
        return _func(refPtr.get());
    }
protected:
        FuncObj _func;
};

template <typename FuncObj>
RefPtrAdapter<FuncObj> refPtrAdapt(const FuncObj& func)
{
    return RefPtrAdapter<FuncObj>(func);
}
}
/** Typesafe wrapper around OSG's object clone function. Something
 * very similar is in current OSG sources.
 */
namespace osg
{
template <typename T> class ref_ptr;
class CopyOp;
}

namespace simgear
{
template <typename T>
T* clone(const T* object, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
{
    return static_cast<T*>(object->clone(copyop));
}

template<typename T>
T* clone_ref(const osg::ref_ptr<T>& object,
             const osg::CopyOp& copyop  = osg::CopyOp::SHALLOW_COPY)
{
    return static_cast<T*>(object->clone(copyop));
}

template<typename Container>
class BackRefInsertIterator
    : public boost::iterator_facade<BackRefInsertIterator<Container>,
                                    BackRefInsertIterator<Container>,
                                    boost::incrementable_traversal_tag
                                    >
{
public:
    typedef typename Container::value_type::element_type* PtrType;
    BackRefInsertIterator() : _container(0) {}
    explicit BackRefInsertIterator(Container& container)
        : _container(&container)
    {
    }

    BackRefInsertIterator&
    operator=(const PtrType ptr)
    {
        _container->push_back(ptr);
        return *this;
    }
    
private:
    friend class boost::iterator_core_access;

    void increment()
    {
    }
    
    BackRefInsertIterator& dereference()
    {
        return *this;
    }

    BackRefInsertIterator& dereference() const
    {
        return const_cast<BackRefInsertIterator&>(*this);
    }

    bool equal(const BackRefInsertIterator& rhs)
    {
        return _container == rhs._container;
    }
    
    Container* _container;
};


template<typename Container>
inline BackRefInsertIterator<Container>
backRefInsertIterator(Container& container)
{
    return BackRefInsertIterator<Container>(container);
}
}
#endif
