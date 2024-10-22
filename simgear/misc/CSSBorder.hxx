// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief CSS border definitions and parser (eg. margin, border-image-width)
 */

#ifndef SG_CSSBORDER_HXX_
#define SG_CSSBORDER_HXX_

#include <simgear/math/SGMath.hxx>
#include <simgear/math/SGRect.hxx>
#include <string>

namespace simgear
{

  class CSSBorder
  {
    public:
      union Offsets
      {
        float          val[4];
        struct { float t, r, b, l; };
      };

      union OffsetsTypes
      {
        bool          rel[4];
        struct { bool t_rel, r_rel, b_rel, l_rel; };
      };

      CSSBorder():
        valid(false)
      {}

      bool isValid() const;

      /**
       * Get whether a non-zero offset exists
       */
      bool isNone() const;

      const std::string& getKeyword() const;

      Offsets getRelOffsets(const SGRect<int>& dim) const;
      Offsets getAbsOffsets(const SGRect<int>& dim) const;

      static CSSBorder parse(const std::string& str);

    private:
      Offsets         offsets;
      OffsetsTypes    types;
      std::string     keyword;
      bool            valid;
  };

} // namespace simgear

#endif /* SG_CSSBORDER_HXX_ */
