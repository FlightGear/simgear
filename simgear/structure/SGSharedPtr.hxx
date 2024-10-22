// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2005-2012 by Mathias Froehlich

/**
 * @file
 * @brief Pointer proxy doing reference counting.
 */

#ifndef SGSharedPtr_HXX
#define SGSharedPtr_HXX

#include <simgear/sg_inlines.h>

#include "SGReferenced.hxx"

template<typename T>
class SGWeakPtr;

/// This class is a pointer proxy doing reference counting on the object
/// it is pointing to.
/// The SGSharedPtr class handles reference counting and possible
/// destruction if no nore references are in use automatically.
/// Classes derived from SGReferenced can be handled with SGSharedPtr.
/// Once you have a SGSharedPtr available you can use it just like
/// a usual pointer with the exception that you don't need to delete it.
/// Such a reference is initialized by zero if not initialized with a
/// class pointer.
/// One thing you need to avoid are cyclic loops with such pointers.
/// As long as such a cyclic loop exists the reference count never drops
/// to zero and consequently the objects will never be destroyed.
/// Always try to use directed graphs where the references away from the
/// top node are made with SGSharedPtr's and the back references are done with
/// ordinary pointers or SGWeakPtr's.
/// There is a very good description of OpenSceneGraphs ref_ptr which is
/// pretty much the same than this one at
/// https://web.archive.org/web/20061012014801/http://dburns.dhs.org/OSG/Articles/RefPointers/RefPointers.html

template<typename T>
class SGSharedPtr final {
public:
    typedef T element_type;

    SGSharedPtr(void) noexcept
    : _ptr(0)
    {}

    SGSharedPtr(T* ptr) : _ptr(ptr)
    {
        get(_ptr);
    }

    SGSharedPtr(const SGSharedPtr& p)
    : _ptr(p.get())
    {
        get(_ptr);
    }

    SGSharedPtr(SGSharedPtr&& other) noexcept
    : SGSharedPtr()
    {
        swap(other);
    }

    template<typename U>
    SGSharedPtr(const SGSharedPtr<U>& p)
    : _ptr(p.get())
    {
        get(_ptr);
    }

    template<typename U>
    explicit SGSharedPtr(const SGWeakPtr<U>& p)
    : _ptr(0)
    {
        reset(p.lock().get());
    }

    ~SGSharedPtr(void)      // non-virtual intentional
    {
        reset();
    }

    SGSharedPtr& operator=(const SGSharedPtr& p)
    {
        reset(p.get());
        return *this;
    }

    SGSharedPtr& operator=(SGSharedPtr&& p) noexcept
    {
        // Whether self-move is to be supported at all is controversial, let's
        // take the conservative approach for now
        if (this != &p) {
            swap(p);
            p.reset();
        }
        return *this;
    }

    template<typename U>
    SGSharedPtr& operator=(const SGSharedPtr<U>& p)
    {
      reset(p.get());
      return *this;
    }
    
    template<typename U>
    SGSharedPtr& operator=(U* p)
    {
        reset(p);
        return *this;
    }

    T* operator->(void) const
    {
        return _ptr;
    }
    
    T& operator*(void) const
    {
        return *_ptr;
    }
    
    operator T*(void) const
    {
        return _ptr;
    }
    
    T* ptr(void) const
    {
        return _ptr;
    }
    
    T* get(void) const
    {
        return _ptr;
    }
    
    T* release()
    {
        T* tmp = _ptr;
        _ptr = 0;
        T::put(tmp);
        return tmp;
    }
    
    void reset() noexcept
    {
        if (!T::put(_ptr)) delete _ptr;
        _ptr = 0;
    }
    
    void reset(T* p)
    {
        SGSharedPtr(p).swap(*this);
    }

    bool isShared(void) const
    {
        return T::shared(_ptr);
    }
    
    unsigned getNumRefs(void) const
    {
        return T::count(_ptr);
    }

    bool valid(void) const
    {
        return _ptr != (T*)0;
    }

    void clear()
    {
        reset();
    }
    
    void swap(SGSharedPtr& other) noexcept
    {
        simgear::noexceptSwap(_ptr, other._ptr);
    }

private:
    
    void assignNonRef(T* p)
    {
        reset();
        _ptr = p;
    }

    void get(const T* p) const
    {
        T::get(p);
    }

    // The reference itself.
    T* _ptr;

    template<typename U>
    friend class SGWeakPtr;
};

template<class T>
void swap(SGSharedPtr<T>& a, SGSharedPtr<T>& b) noexcept
{
  a.swap(b);
}

/**
 * Support for boost::mem_fn
 */
template<typename T>
T* get_pointer(SGSharedPtr<T> const & p)
{
  return p.ptr();
}

/**
 * static_cast for SGSharedPtr
 */
template<class T, class U>
SGSharedPtr<T> static_pointer_cast(SGSharedPtr<U> const & r)
{
  return SGSharedPtr<T>( static_cast<T*>(r.get()) );
}

template<class T, class U>
SGSharedPtr<T> dynamic_pointer_cast(SGSharedPtr<U> const& r)
{
    return SGSharedPtr<T>(dynamic_cast<T*>(r.get()));
}

/**
 * Compare two SGSharedPtr<T> objects for equality.
 *
 * @note Only pointer values are compared, not the actual objects they are
 *       pointing at.
 */
template<class T, class U>
bool operator==(const SGSharedPtr<T>& lhs, const SGSharedPtr<U>& rhs)
{
  return lhs.get() == rhs.get();
}

/**
 * Compare two SGSharedPtr<T> objects for equality.
 *
 * @note Only pointer values are compared, not the actual objects they are
 *       pointing at.
 */
template<class T, class U>
bool operator!=(const SGSharedPtr<T>& lhs, const SGSharedPtr<U>& rhs)
{
  return lhs.get() != rhs.get();
}

/**
 * Compare two SGSharedPtr<T> objects for weak ordering.
 *
 * @note Only pointer values are compared, not the actual objects they are
 *       pointing at.
 * @note This allows using SGSharedPtr as key in associative containers like for
 *       example std::map and std::set.
 */
template<class T, class U>
bool operator<(const SGSharedPtr<T>& lhs, const SGSharedPtr<U>& rhs)
{
  return lhs.get() < rhs.get();
}
#endif
