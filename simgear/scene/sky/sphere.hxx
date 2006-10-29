// sphere.hxx -- build an ssg sphere object
//
// Pulled straight out of MesaGLU/quadratic.c
//
// Original gluSphere code is Copyright (C) 1999-2000  Brian Paul and
// licensed under the GPL
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
// $Id$


#include <osg/Node>

// return a sphere object as an ssgBranch (and connect in the
// specified ssgSimpleState
osg::Node* SGMakeSphere(double radius, int slices, int stacks);


