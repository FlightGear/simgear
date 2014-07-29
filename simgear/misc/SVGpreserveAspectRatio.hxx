///@file
/// Parse and represent SVG preserveAspectRatio attribute.
//
// Copyright (C) 2014  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef SG_SVG_PRESERVE_ASPECT_RATIO_HXX_
#define SG_SVG_PRESERVE_ASPECT_RATIO_HXX_

#include <string>

namespace simgear
{

  /**
   * SVG preserveAspectRatio attribute
   *
   * @see http://www.w3.org/TR/SVG11/coords.html#PreserveAspectRatioAttribute
   */
  class SVGpreserveAspectRatio
  {
    public:
      enum Align
      {
        ALIGN_NONE,
        ALIGN_MIN,
        ALIGN_MID,
        ALIGN_MAX
      };

      SVGpreserveAspectRatio();

      Align alignX() const;
      Align alignY() const;

      bool scaleToFill() const;
      bool scaleToFit() const;
      bool scaleToCrop() const;

      bool meet() const;

      bool operator==(const SVGpreserveAspectRatio& rhs) const;

      /// Parse preserveAspectRatio from string.
      static SVGpreserveAspectRatio parse(const std::string& str);

    private:
      Align _align_x,
            _align_y;
      bool  _meet; //!< uniform scale to fit, if true
                   //   uniform scale to fill+crop, if false
  };

} // namespace simgear

#endif /* SG_SVG_PRESERVE_ASPECT_RATIO_HXX_ */
