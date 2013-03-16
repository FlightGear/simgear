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

#ifndef SG_COLOR_INTERPOLATOR_HXX_
#define SG_COLOR_INTERPOLATOR_HXX_

#include <simgear/props/PropertyInterpolator.hxx>

#include <osg/Vec4>
#include <string>

namespace simgear
{

  /**
   * Interpolate a string property containing containing a %CSS color.
   */
  class ColorInterpolator:
    public PropertyInterpolator
  {
    protected:
      osg::Vec4 _color_end,
                _color_diff;

      virtual void setTarget(const SGPropertyNode& target);
      virtual void init(const SGPropertyNode& prop);
      virtual void write(SGPropertyNode& prop, double t);


  };

} // namespace simgear

#endif /* SG_COLOR_INTERPOLATOR_HXX_ */
