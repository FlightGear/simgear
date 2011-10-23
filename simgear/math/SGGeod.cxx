// Copyright (C) 2008  Tim Moore timoore@redhat.com
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "SGMath.hxx"

#ifndef NO_OPENSCENEGRAPH_INTERFACE

osg::Matrix SGGeod::makeSimulationFrameRelative() const
{
    SGQuatd hlOr = SGQuatd::fromLonLat(*this);
    return osg::Matrix(toOsg(hlOr));
}

osg::Matrix SGGeod::makeSimulationFrame() const
{
    osg::Matrix result(makeSimulationFrameRelative());
    SGVec3d coord;
    SGGeodesy::SGGeodToCart(*this, coord);
    result.setTrans(toOsg(coord));
    return result;
}

osg::Matrix SGGeod::makeZUpFrameRelative() const
{
    osg::Matrix result(makeSimulationFrameRelative());
    // 180 degree rotation around Y axis
    osg::Quat flip(0.0, 1.0, 0.0, 0.0);
    result.preMult(osg::Matrix(flip));
    return result;
}

osg::Matrix SGGeod::makeZUpFrame() const
{
    osg::Matrix result(makeZUpFrameRelative());
    SGVec3d coord;
    SGGeodesy::SGGeodToCart(*this, coord);
    result.setTrans(toOsg(coord));
    return result;
}

#endif
