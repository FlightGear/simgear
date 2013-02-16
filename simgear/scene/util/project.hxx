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
#ifndef SIMGEAR_PROJECT_HXX
#define SIMGEAR_PROJECT_HXX 1

#include <osg/GL>

namespace simgear
{
// Replacement for gluProject. OSG doesn't link in GLU anymore.
extern GLint project(GLdouble objX, GLdouble objY, GLdouble objZ,
                     const GLdouble *model, const GLdouble *proj,
                     const GLint *view,
                     GLdouble* winX, GLdouble* winY, GLdouble* winZ);
}

#endif

