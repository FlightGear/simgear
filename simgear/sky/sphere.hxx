// sphere.hxx -- build an ssg sphere object
//
// Pulled straight out of MesaGLU/quadratic.c
//
// Original gluSphere code is Copyright (C) 1999-2000  Brian Paul and
// licensed under the GPL
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#include <plib/ssg.h>


// return a sphere object as an ssgBranch (and connect in the
// specified ssgSimpleState
ssgBranch *ssgMakeSphere( ssgSimpleState *state, double radius, int slices,
			  int stacks );


