// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief  Test Parse and represent SVG preserveAspectRatio attribute
 */

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
