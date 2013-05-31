// Parse and represent CSS border definitions (eg. margin, border-image-width)
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
