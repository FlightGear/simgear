// stars.cxx -- manage star data
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


#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgstream.hxx>

#include "stars.hxx"

#ifdef _MSC_VER
  FG_USING_STD(getline);
#endif

// Constructor
FGStars::FGStars() {
}

FGStars::FGStars( FGPath path ) {
    data_path = FGPath( path );
    load();
}


// Destructor
FGStars::~FGStars() {
}


bool FGStars::load() {

    // -dw- avoid local data > 32k error by dynamic allocation of the
    // array, problem for some compilers
    stars = new sgdVec3[FG_MAX_STARS];

     // build the full path name to the stars data base file
    data_path.append( "stars" );
    FG_LOG( FG_ASTRO, FG_INFO, "  Loading stars from " << data_path.str() );

    fg_gzifstream in( data_path.str() );
    if ( ! in.is_open() ) {
	FG_LOG( FG_ASTRO, FG_ALERT, "Cannot open star file: "
		<< data_path.str() );
	exit(-1);
    }

    double ra, dec, mag;
    char c;
    string name;

    nstars = 0;

    // read in each line of the file
    while ( ! in.eof() && nstars < FG_MAX_STARS ) {
	in >> skipcomment;

        getline( in, name, ',' );
	// cout << "  data = " << name << endl;

	// read name and first comma
	while ( in.get(c) ) {
	    if ( (c != ' ') && (c != ',') ) {
		// push back on the stream
		in.putback(c);
		break;
	    }
	}

	in >> ra;

	// read past optional comma
	while ( in.get(c) ) {
	    if ( (c != ' ') && (c != ',') ) {
		// push back on the stream
		in.putback(c);
		break;
	    }
	}

	in >> dec;

	// read past optional comma
	while ( in.get(c) ) {
	    if ( (c != ' ') && (c != ',') ) {
		// push back on the stream
		in.putback(c);
		break;
	    }
	}

	in >> mag;

	// cout << " star data = " << ra << " " << dec << " " << mag << endl;

	sgdSetVec3( stars[nstars], ra, dec, mag );

	++nstars;
    }

    FG_LOG( FG_ASTRO, FG_INFO, "  Loaded " << nstars << " stars" );

    return true;
}
