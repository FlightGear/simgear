// stars.hxx -- manage star data
//
// Written by Curtis Olson, started March 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


#ifndef _SG_STARDATA_HXX
#define _SG_STARDATA_HXX


#include <plib/sg.h>

#include <simgear/misc/fgpath.hxx>


#define SG_MAX_STARS 850


class SGStarData {

    int nstars;
    sgdVec3 *stars;
    
    FGPath data_path;

public:

    // Constructor
    SGStarData();
    SGStarData( FGPath path );

    // Destructor
    ~SGStarData();

    // load the stars database
    bool load();

    // stars
    inline int getNumStars() const { return nstars; }
    inline sgdVec3 *getStars() { return stars; }
};


#endif // _SG_STARDATA_HXX
