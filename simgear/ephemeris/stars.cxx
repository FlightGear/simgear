// stars.cxx -- manage star data
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


#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgstream.hxx>

#include "stars.hxx"

#ifdef _MSC_VER
  FG_USING_STD(getline);
#endif

// Constructor
SGStarData::SGStarData() {
}

SGStarData::SGStarData( FGPath path ) {
    data_path = FGPath( path );
    load();
}


// Destructor
SGStarData::~SGStarData() {
}


bool SGStarData::load() {

    // -dw- avoid local data > 32k error by dynamic allocation of the
    // array, problem for some compilers
    stars = new sgdVec3[SG_MAX_STARS];

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
    while ( ! in.eof() && nstars < SG_MAX_STARS ) {
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
