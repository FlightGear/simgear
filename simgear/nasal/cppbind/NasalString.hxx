// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012  Thomas Geymayer <tomgey@gmail.com>

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
      explicit String(naRef str);

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
