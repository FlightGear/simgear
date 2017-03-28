// Adapter for interpolating string properties representing CSS colors.
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

#include <simgear_config.h>
#include "ColorInterpolator.hxx"
#include "parse_color.hxx"

#include <simgear/props/props.hxx>

namespace simgear
{

  //----------------------------------------------------------------------------
  void ColorInterpolator::setTarget(const SGPropertyNode& target)
  {
    if( !parseColor(target.getStringValue(), _color_end) )
      SG_LOG
      (
        SG_GENERAL, SG_WARN, "ColorInterpolator: failed to parse end color."
      );
  }

  //----------------------------------------------------------------------------
  void ColorInterpolator::init(const SGPropertyNode& prop)
  {
    osg::Vec4 color_start;
    if( !parseColor(prop.getStringValue(), color_start) )
      // If unable to get current color, immediately change to target color
      color_start = _color_end;

    _color_diff = _color_end - color_start;
  }

  //----------------------------------------------------------------------------
  void ColorInterpolator::write(SGPropertyNode& prop, double t)
  {
    osg::Vec4 color_cur = _color_end - _color_diff * (1 - t);
    bool has_alpha = color_cur.a() < 0.999;

    std::ostringstream strm;
    strm << (has_alpha ? "rgba(" : "rgb(");

    // r, g, b (every component in [0, 255])
    for(size_t i = 0; i < 3; ++i)
    {
      if( i > 0 )
        strm << ',';
      strm << static_cast<int>(color_cur._v[i] * 255);
    }

    // Write alpha if a < 1 (alpha is in [0, 1])
    if( has_alpha )
      strm << ',' << color_cur.a();

    strm << ')';

    prop.setStringValue(strm.str());
  }

} // namespace simgear
