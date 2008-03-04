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

#include "SGMath.hxx"

osg::Matrix SGGeod::makeSimulationFrameRelative()
{
    SGQuatd hlOr = SGQuatd::fromLonLat(*this);
    return osg::Matrix(hlOr.osg());
}

osg::Matrix SGGeod::makeSimulationFrame()
{
    osg::Matrix result(makeSimulationFrameRelative());
    SGVec3d coord;
    SGGeodesy::SGGeodToCart(*this, coord);
    result.setTrans(coord.osg());
    return result;
}

osg::Matrix SGGeod::makeZUpFrameRelative()
{
    osg::Matrix result(makeSimulationFrameRelative());
    // 180 degree rotation around Y axis
    osg::Quat flip(0.0, 1.0, 0.0, 0.0);
    result.preMult(osg::Matrix(flip));
    return result;
}

osg::Matrix SGGeod::makeZUpFrame()
{
    osg::Matrix result(makeZUpFrameRelative());
    SGVec3d coord;
    SGGeodesy::SGGeodToCart(*this, coord);
    result.setTrans(coord.osg());
    return result;
}
