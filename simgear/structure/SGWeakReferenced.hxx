// Copyright (C) 2004-2009  Mathias Froehlich - Mathias.Froehlich@web.de
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef SGWeakReferenced_HXX
#define SGWeakReferenced_HXX

#include "SGReferenced.hxx"
#include "SGSharedPtr.hxx"

#include <boost/type_traits/is_base_of.hpp>
#include <boost/utility/enable_if.hpp>

#ifdef _MSC_VER
# pragma warning(push)
  // C4355: 'this' : used in base member initializer list
  // Tell MSVC we know what we do and really want to do it this way.
# pragma warning(disable: 4355)
#endif

template<typename T>
class SGWeakPtr;
class SGVirtualWeakReferenced;

/**
 * Base class for all reference counted SimGear objects supporting weak
 * references, not incrementing the reference count.
 *
 * Classes derived from this one are meant to be managed with the SGSharedPtr
 * and SGWeakPtr classes.
 *
 * If the class hierarchy contains virtual base classes use
 * SGVirtualWeakReferenced instead.
 */
class SGWeakReferenced {
public:
  /// The object backref and the reference count for this object need to be
  /// there in any case. Also these are per object and shall not be copied nor
  /// assigned.
  /// The reference count for this object is stored in a secondary object that
  /// is shared with all weak pointers to this current object. This way we
  /// have an atomic decision using the reference count of this current object
  /// if the backref is still valid. At the time where the atomic count is
  /// equal to zero the object is considered dead.
  SGWeakReferenced(void) :
    mWeakData(new WeakData(this))
  {}
  SGWeakReferenced(const SGWeakReferenced& weakReferenced) :
    mWeakData(new WeakData(this))
  {}
  ~SGWeakReferenced(void)
  { mWeakData->mWeakReferenced = 0; }

  /// Do not copy the weak backward references ...
  SGWeakReferenced& operator=(const SGWeakReferenced&)
  { return *this; }

  /// The usual operations on weak pointers.
  /// The interface should stay the same then what we have in Referenced.
  static unsigned get(const SGWeakReferenced* ref)
  { if (ref) return ++(ref->mWeakData->mRefcount); else return 0; }
  static unsigned put(const SGWeakReferenced* ref)
  { if (ref) return --(ref->mWeakData->mRefcount); else return 0; }
  static unsigned count(const SGWeakReferenced* ref)
  { if (ref) return ref->mWeakData->mRefcount; else return 0; }

private:
  /// Support for weak references, not increasing the reference count
  /// that is done through that small helper class which holds an uncounted
  /// reference which is zeroed out on destruction of the current object
  class WeakData : public SGReferenced {
  public:
    WeakData(SGWeakReferenced* weakReferenced) :
      mRefcount(0u),
      mWeakReferenced(weakReferenced)
    { }

    template<typename T>
    T* getPointer()
    {
      // Try to increment the reference count if the count is greater
      // then zero. Since it should only be incremented iff it is nonzero, we
      // need to check that value and try to do an atomic test and set. If this
      // fails, try again. The usual lockless algorithm ...
      unsigned count;
      do {
        count = mRefcount;
        if (count == 0)
          return 0;
      } while (!mRefcount.compareAndExchange(count, count + 1));
      // We know that as long as the refcount is not zero, the pointer still
      // points to valid data. So it is safe to work on it.
      return up_cast<T>(mWeakReferenced);
    }

    SGAtomic mRefcount;
    SGWeakReferenced* mWeakReferenced;

  private:
    WeakData(void);
    WeakData(const WeakData&);
    WeakData& operator=(const WeakData&);

    /// Upcast in a class hierarchy with a virtual base class
    template<class T>
    static
    typename boost::enable_if<
      boost::is_base_of<SGVirtualWeakReferenced, T>,
      T*
    >::type
    up_cast(SGWeakReferenced* ptr);

    /// Upcast in a non-virtual class hierarchy
    template<class T>
    static
    typename boost::disable_if<
      boost::is_base_of<SGVirtualWeakReferenced, T>,
      T*
    >::type
    up_cast(SGWeakReferenced* ptr)
    {
      return static_cast<T*>(ptr);
    }
  };

  SGSharedPtr<WeakData> mWeakData;

  template<typename T>
  friend class SGWeakPtr;
};

/**
 * Base class for all reference counted SimGear objects with virtual base
 * classes, supporting weak references.
 *
 * Classes derived from this one are meant to be managed with the SGSharedPtr
 * and SGWeakPtr classes.
 *
 * @code{cpp}
 *
 * class Base1:
 *   public virtual SGVirtualWeakReferenced
 * {};
 *
 * class Base2:
 *   public virtual SGVirtualWeakReferenced
 * {};
 *
 * class Derived:
 *   public Base1,
 *   public Base2
 * {};
 *
 * SGSharedPtr<Derived> ptr( new Derived() );
 * SGWeakPtr<Derived> weak_ptr( ptr );
 * SGSharedPtr<Base1> ptr1( weak_ptr.lock() );
 * SGSharedPtr<Base2> ptr2( weak_ptr.lock() );
 *
 * @endcode
 */
class SGVirtualWeakReferenced:
  public SGWeakReferenced
{
  public:
    virtual ~SGVirtualWeakReferenced() {}
};

/// Upcast in a class hierarchy with a virtual base class
// We (clang) need the definition of SGVirtualWeakReferenced for the static_cast
template<class T>
typename boost::enable_if<
  boost::is_base_of<SGVirtualWeakReferenced, T>,
  T*
>::type
SGWeakReferenced::WeakData::up_cast(SGWeakReferenced* ptr)
{
  // First get the virtual base class, which then can be used to further
  // upcast.
  return dynamic_cast<T*>(static_cast<SGVirtualWeakReferenced*>(ptr));
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#endif
