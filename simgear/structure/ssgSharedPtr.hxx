/* -*-c++-*-
 *
 * Copyright (C) 2005-2006 Mathias Froehlich 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef ssgSharedPtr_HXX
#define ssgSharedPtr_HXX

/// This class is a pointer proxy doing reference counting on the object
/// it is pointing to.
/// It is very similar to the SGSharedPtr class but is made to work together
/// with reference counting of plib's ssg reference counters
/// For notes on usage see SGSharedPtr.

template<typename T>
class ssgSharedPtr {
public:
  ssgSharedPtr(void) : _ptr(0)
  {}
  ssgSharedPtr(T* ptr) : _ptr(ptr)
  { get(_ptr); }
  ssgSharedPtr(const ssgSharedPtr& p) : _ptr(p.ptr())
  { get(_ptr); }
  template<typename U>
  ssgSharedPtr(const ssgSharedPtr<U>& p) : _ptr(p.ptr())
  { get(_ptr); }
  ~ssgSharedPtr(void)
  { put(); }
  
  ssgSharedPtr& operator=(const ssgSharedPtr& p)
  { assign(p.ptr()); return *this; }
  template<typename U>
  ssgSharedPtr& operator=(const ssgSharedPtr<U>& p)
  { assign(p.ptr()); return *this; }
  template<typename U>
  ssgSharedPtr& operator=(U* p)
  { assign(p); return *this; }

  T* operator->(void) const
  { return _ptr; }
  T& operator*(void) const
  { return *_ptr; }
  operator T*(void) const
  { return _ptr; }
  T* ptr(void) const
  { return _ptr; }

  bool isShared(void) const
  { if (_ptr) return 1 < _ptr->getRef(); else return false; }
  unsigned getNumRefs(void) const
  { if (_ptr) return _ptr->getRef(); else return 0; }

  bool valid(void) const
  { return _ptr; }

private:
  void assign(T* p)
  { get(p); put(); _ptr = p; }

  static void get(T* p)
  { if (p) p->ref(); }
  void put(void)
  {
    if (!_ptr)
      return;

    assert(0 < _ptr->getRef());
    _ptr->deRef();
    if (_ptr->getRef() == 0) {
      delete _ptr;
      _ptr = 0;
    }
  }
  
  // The reference itself.
  T* _ptr;
};

#endif
