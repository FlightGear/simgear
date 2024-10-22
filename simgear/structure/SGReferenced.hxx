// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2005-2006 by Mathias Froehlich

#ifndef SGReferenced_HXX
#define SGReferenced_HXX

#include "SGAtomic.hxx"

/// Base class for all reference counted SimGear objects
/// Classes derived from this one are meant to be managed with
/// the SGSharedPtr class.
///
/// For more info see SGSharedPtr. For using weak references see
/// SGWeakReferenced.

class SGReferenced {
public:
  SGReferenced(void) : _refcount(0u)
  {}
  /// Do not copy reference counts. Each new object has it's own counter
  SGReferenced(const SGReferenced&) : _refcount(0u)
  {}
  /// Do not copy reference counts. Each object has it's own counter
  SGReferenced& operator=(const SGReferenced&)
  { return *this; }

  static unsigned get(const SGReferenced* ref)
  { if (ref) return ++(ref->_refcount); else return 0; }
  static unsigned put(const SGReferenced* ref) noexcept
  { if (ref) return --(ref->_refcount); else return 0; }
  static unsigned count(const SGReferenced* ref)
  { if (ref) return ref->_refcount; else return 0; }
  static bool shared(const SGReferenced* ref)
  { if (ref) return 1u < ref->_refcount; else return false; }

private:
  mutable SGAtomic _refcount;
};

#endif
