

// Copyright (C) 2011  Curtis L Olson <curt@flightgear.org>
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

#include <simgear/math/project.hxx>
#include <simgear/misc/PathOptions.hxx>
#include <simgear/math/SGMath.hxx>

#include <osg/Math>
#include <osg/Matrixd>
#include <osgDB/Registry>

namespace simgear
{
GLint project(GLdouble objX, GLdouble objY, GLdouble objZ,
              const GLdouble *model, const GLdouble *proj, const GLint *view,
              GLdouble* winX, GLdouble* winY, GLdouble* winZ)
{
    using namespace osg;
    Vec4d obj(objX, objY, objZ, 1.0);
    Matrixd Mmodel(model), Mproj(proj);
    Matrixd Mwin = (Matrixd::translate(1.0, 1.0, 1.0)
                    * Matrixd::scale(0.5 * view[2], 0.5 * view[3], 0.5)
                    * Matrixd::translate(view[0], view[1], 0.0));
    Vec4d result = obj * Mmodel * Mproj * Mwin;
    if (equivalent(result.w(), 0.0))
        return GL_FALSE;
    result = result / result.w();
    *winX = result.x();  *winY = result.y();  *winZ = result.z();
    return GL_TRUE;
}

osgDB::ReaderWriter::Options* makeOptionsFromPath(const SGPath& path)
{
    using namespace osgDB;
    ReaderWriter::Options *options
        = new ReaderWriter::Options(*(Registry::instance()->getOptions()));
    options->setDatabasePath(path.str());
    return options;
}


}

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

