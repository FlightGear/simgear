// stars.hxx -- manage star data
//
// Written by Curtis Olson, started March 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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


#ifndef _STARS_HXX
#define _STARS_HXX


#include <plib/sg.h>

#include <simgear/misc/fgpath.hxx>


#define FG_MAX_STARS 850


class FGStars {

    int nstars;
    sgdVec3 *stars;
    
    FGPath data_path;

public:

    // Constructor
    FGStars();
    FGStars( FGPath path );

    // Destructor
    ~FGStars();

    // load the stars database
    bool load();

    // stars
    inline int getNumStars() const { return nstars; }
    inline sgdVec3 *getStars() { return stars; }
};


#endif // _STARS_HXX
