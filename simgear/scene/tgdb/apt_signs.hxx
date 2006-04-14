// apt_signs.hxx -- build airport signs on the fly
//
// Written by Curtis Olson, started July 2001.
//
// Copyright (C) 2001  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _SG_APT_SIGNS_HXX
#define _SG_APT_SIGNS_HXX


#ifndef __cplusplus
# error This library requires C++
#endif


#include <simgear/compiler.h>

#include STL_STRING

#include <plib/ssg.h>		// plib include

class SGMaterialLib;            // forward declaration

SG_USING_STD(string);


// Generate a generic sign
ssgBranch *sgMakeSign( SGMaterialLib *matlib,
                       const string path, const string content );


// Generate a runway sign
ssgBranch *sgMakeRunwaySign( SGMaterialLib *matlib,
                             const string path, const string name );


#endif // _SG_APT_SIGNS_HXX
