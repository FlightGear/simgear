///@file Wrapper class for keeping Nasal objects save from the garbage collector
//
// Copyright (C) 2013  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef SG_NASAL_OBJECT_HOLDER_HXX_
#define SG_NASAL_OBJECT_HOLDER_HXX_

#include <simgear/nasal/naref.h>
#include <simgear/structure/SGSharedPtr.hxx>

namespace nasal
{

  class ObjectHolder;
  typedef SGSharedPtr<ObjectHolder> ObjectHolderRef;

  /**
   * Prevent a Nasal object from being destroyed by the garbage collector during
   * the lifetime of this object.
   */
  class ObjectHolder:
    public SGReferenced
  {
    public:

      /**
       * @param obj Object to save
       */
      explicit ObjectHolder(naRef obj);

      /**
       *
       */
      ObjectHolder();

      /**
       *
       */
      ~ObjectHolder();

      /**
       * Get captured Nasal object
       */
      naRef get_naRef() const;

      /**
       * Release the managed object
       */
      void reset();

      /**
       * Replaces the managed object (the old object is released)
       *
       * @param obj New object to save
       */
      void reset(naRef obj);

      /**
       * Check if there is a managed object
       */
      bool valid() const;

      /**
       * Save the given object as long as the returned holder exists.
       *
       * @param obj Object to save
       */
      static ObjectHolderRef makeShared(naRef obj);

    protected:
      naRef _ref;
      int _gc_key;
  };

} // namespace nasal

#endif /* SG_NASAL_OBJECT_HOLDER_HXX_ */
