// Copyright (C) 2010  Tim Moore moore@bricoworks.com
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

#include "project.hxx"

#include <osg/Math>
#include <osg/Matrixd>

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

} // of namespace simgear

