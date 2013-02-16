///@file Wrapper class for Nasal hashes
//
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

#ifndef SG_NASAL_HASH_HXX_
#define SG_NASAL_HASH_HXX_

#include "from_nasal.hxx"
#include "to_nasal.hxx"

namespace nasal
{

  /**
   * A Nasal Hash
   */
  class Hash
  {
    public:

      /**
       * Create a new Nasal Hash
       *
       * @param c   Nasal context for creating the hash
       */
      Hash(naContext c);

      /**
       * Initialize from an existing Nasal Hash
       *
       * @param hash  Existing Nasal Hash
       * @param c     Nasal context for creating new Nasal objects
       */
      Hash(naRef hash, naContext c);

      /**
       * Set member
       *
       * @param name    Member name
       * @param ref     Reference to Nasal object (naRef)
       */
      void set(const std::string& name, naRef ref);

      /**
       * Set member to anything convertible using to_nasal
       *
       * @param name    Member name
       * @param val     Value (has to be convertible with to_nasal)
       */
      template<class T>
      void set(const std::string& name, const T& val)
      {
        set(name, to_nasal(_context, val));
      }

      /**
       * Get member
       *
       * @param name    Member name
       */
      naRef get(const std::string& name);

      /**
       * Get member converted to given type
       *
       * @tparam T      Type to convert to (using from_nasal)
       * @param name    Member name
       */
      template<class T>
      T get(const std::string& name)
      {
        return from_nasal<T>(_context, get(name));
      }

      /**
       * Create a new child hash (module)
       *
       * @param name  Name of the new hash inside this hash
       */
      Hash createHash(const std::string& name);

      /**
       * Set a new Nasal context. Eg. in FlightGear the context changes every
       * frame, so if using the same Hash instance for multiple frames you have
       * to update the context before using the Hash object.
       */
      void setContext(naContext context);

      /**
       * Get Nasal representation of Hash
       */
      const naRef get_naRef() const;

    protected:

      naRef _hash;
      naContext _context;

  };

} // namespace nasal

#endif /* SG_NASAL_HASH_HXX_ */
