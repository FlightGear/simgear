// userdata.hxx -- two classes for populating ssg user data slots in association
//                 with our implimenation of random surface objects.
//
// Written by David Megginson, started December 2001.
//
// Copyright (C) 2001 - 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _SG_USERDATA_HXX
#define _SG_USERDATA_HXX

#include <simgear/compiler.h>

#include <osg/Node>

class SGMatModel;
class SGPropertyNode;

/**
 * the application must call sgUserDataInit() and specify the
 * following values (needed by the model loader callback at draw time)
 * before drawing any scenery.
 */
void sgUserDataInit(SGPropertyNode *p);

namespace simgear
{
/**
 * Get the property root for the simulation
 */
SGPropertyNode* getPropertyRoot();
}

#endif // _SG_USERDATA_HXX
