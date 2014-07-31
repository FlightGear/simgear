///@file
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

#ifndef SG_NASAL_STRING_HXX_
#define SG_NASAL_STRING_HXX_

#include "from_nasal.hxx"
#include "to_nasal.hxx"

namespace nasal
{

  /**
   * Wrapper class for Nasal strings.
   */
  class String
  {
    public:

      static const size_t npos;

      /**
       * Create a new Nasal String
       *
       * @param c   Nasal context for creating the string
       * @param str String data
       */
      String(naContext c, const char* str);

      /**
       * Initialize from an existing Nasal String
       *
       * @param str   Existing Nasal String
       */
      String(naRef str);

      const char* c_str() const;
      const char* begin() const;
      const char* end() const;

      size_t size() const;
      size_t length() const;
      bool empty() const;

      int compare(size_t pos, size_t len, const String& rhs) const;
      bool starts_with(const String& rhs) const;
      bool ends_with(const String& rhs) const;

      size_t find(const char c, size_t pos = 0) const;
      size_t find_first_of(const String& chr, size_t pos = 0) const;
      size_t find_first_not_of(const String& chr, size_t pos = 0) const;

      /**
       * Get Nasal representation of String
       */
      const naRef get_naRef() const;

    protected:

      naRef _str;

  };

} // namespace nasal

#endif /* SG_NASAL_STRING_HXX_ */
