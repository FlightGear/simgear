// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2004-2009 Mathias Froehlich

#ifndef SGWeakPtr_HXX
#define SGWeakPtr_HXX

#include "SGWeakReferenced.hxx"

/**
 * Class for handling weak references to classes derived from SGWeakReferenced
 * or SGVirtualWeakReferenced.
 */
template<typename T>
class SGWeakPtr final {
public:
  typedef T element_type;

  SGWeakPtr(void)
  { }
  SGWeakPtr(const SGWeakPtr& p) : mWeakData(p.mWeakData)
  { }
  SGWeakPtr(T* ptr)
  { assign(ptr); }
  template<typename U>
  SGWeakPtr(const SGSharedPtr<U>& p)
  { assign(p.get()); }
  template<typename U>
  SGWeakPtr(const SGWeakPtr<U>& p)
  { SGSharedPtr<T> sharedPtr = p.lock(); assign(sharedPtr.get()); }
  
  ~SGWeakPtr(void)    // non-virtual intentional
  { }
  
  template<typename U>
  SGWeakPtr& operator=(const SGSharedPtr<U>& p)
  { assign(p.get()); return *this; }
  template<typename U>
  SGWeakPtr& operator=(const SGWeakPtr<U>& p)
  { SGSharedPtr<T> sharedPtr = p.lock(); assign(sharedPtr.get()); return *this; }
  SGWeakPtr& operator=(const SGWeakPtr& p)
  { mWeakData = p.mWeakData; return *this; }

  template<typename U>
  bool operator==(const SGWeakPtr<U>& rhs) const
  { return mWeakData == rhs.mWeakData; }
  template<typename U>
  bool operator!=(const SGWeakPtr<U>& rhs) const
  { return mWeakData != rhs.mWeakData; }
  template<typename U>
  bool operator<(const SGWeakPtr<U>& rhs) const
  { return mWeakData < rhs.mWeakData; }

  SGSharedPtr<T> lock(void) const
  {
    if (!mWeakData)
      return SGSharedPtr<T>();
    SGSharedPtr<T> sharedPtr;
    sharedPtr.assignNonRef(mWeakData->getPointer<T>());
    return sharedPtr;
  }

  bool expired() const
  { return !mWeakData || mWeakData->mRefcount == 0; }

  void reset()
  { mWeakData.reset(); }
  void clear()
  { mWeakData.reset(); }
  void swap(SGWeakPtr& weakPtr)
  { mWeakData.swap(weakPtr.mWeakData); }

private:
  void assign(T* p)
  {
    if (p)
      mWeakData = p->mWeakData;
    else
      mWeakData = 0;
  }
  
  // The indirect reference itself.
  SGSharedPtr<SGWeakReferenced::WeakData> mWeakData;
};

#endif
