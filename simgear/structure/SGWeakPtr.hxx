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

#ifndef SGWeakPtr_HXX
#define SGWeakPtr_HXX

#include "SGWeakReferenced.hxx"

/**
 * Class for handling weak references to classes derived from SGWeakReferenced
 * or SGVirtualWeakReferenced.
 */
template<typename T>
class SGWeakPtr {
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
  ~SGWeakPtr(void)
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
