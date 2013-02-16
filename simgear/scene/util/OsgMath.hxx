// Copyright (C) 2006-2009  Mathias Froehlich - Mathias.Froehlich@web.de
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

#ifndef SIMGEAR_SCENE_UTILS_OSGMATH_HXX
#define SIMGEAR_SCENE_UTILS_OSGMATH_HXX

#include <osg/Vec2f>
#include <osg/Vec2d>
#include <osg/Vec3f>
#include <osg/Vec3d>
#include <osg/Vec4f>
#include <osg/Vec4d>
#include <osg/Quat>
#include <osg/Matrix>

#include <simgear/math/SGMath.hxx>

inline
SGVec2d
toSG(const osg::Vec2d& v)
{ return SGVec2d(v[0], v[1]); }

inline
SGVec2f
toSG(const osg::Vec2f& v)
{ return SGVec2f(v[0], v[1]); }

inline
osg::Vec2d
toOsg(const SGVec2d& v)
{ return osg::Vec2d(v[0], v[1]); }

inline
osg::Vec2f
toOsg(const SGVec2f& v)
{ return osg::Vec2f(v[0], v[1]); }

inline
SGVec3d
toSG(const osg::Vec3d& v)
{ return SGVec3d(v[0], v[1], v[2]); }

inline
SGVec3f
toSG(const osg::Vec3f& v)
{ return SGVec3f(v[0], v[1], v[2]); }

inline
osg::Vec3d
toOsg(const SGVec3d& v)
{ return osg::Vec3d(v[0], v[1], v[2]); }

inline
osg::Vec3f
toOsg(const SGVec3f& v)
{ return osg::Vec3f(v[0], v[1], v[2]); }

inline
SGVec4d
toSG(const osg::Vec4d& v)
{ return SGVec4d(v[0], v[1], v[2], v[3]); }

inline
SGVec4f
toSG(const osg::Vec4f& v)
{ return SGVec4f(v[0], v[1], v[2], v[3]); }

inline
osg::Vec4d
toOsg(const SGVec4d& v)
{ return osg::Vec4d(v[0], v[1], v[2], v[3]); }

inline
osg::Vec4f
toOsg(const SGVec4f& v)
{ return osg::Vec4f(v[0], v[1], v[2], v[3]); }

inline
SGQuatd
toSG(const osg::Quat& q)
{ return SGQuatd(q[0], q[1], q[2], q[3]); }

inline
osg::Quat
toOsg(const SGQuatd& q)
{ return osg::Quat(q[0], q[1], q[2], q[3]); }

// Create a local coordinate frame in the earth-centered frame of
// reference. X points north, Z points down.
// makeSimulationFrameRelative() only includes rotation.
inline
osg::Matrix
makeSimulationFrameRelative(const SGGeod& geod)
{ return osg::Matrix(toOsg(SGQuatd::fromLonLat(geod))); }

inline
osg::Matrix
makeSimulationFrame(const SGGeod& geod)
{
    osg::Matrix result(makeSimulationFrameRelative(geod));
    SGVec3d coord;
    SGGeodesy::SGGeodToCart(geod, coord);
    result.setTrans(toOsg(coord));
    return result;
}

// Create a Z-up local coordinate frame in the earth-centered frame
// of reference. This is what scenery models, etc. expect.
// makeZUpFrameRelative() only includes rotation.
inline
osg::Matrix
makeZUpFrameRelative(const SGGeod& geod)
{
    osg::Matrix result(makeSimulationFrameRelative(geod));
    // 180 degree rotation around Y axis
    result.preMultRotate(osg::Quat(0.0, 1.0, 0.0, 0.0));
    return result;
}

inline
osg::Matrix
makeZUpFrame(const SGGeod& geod)
{
    osg::Matrix result(makeZUpFrameRelative(geod));
    SGVec3d coord;
    SGGeodesy::SGGeodToCart(geod, coord);
    result.setTrans(toOsg(coord));
    return result;
}

#endif
