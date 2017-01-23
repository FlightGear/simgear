// stardata.cxx -- manage star data
//
// Written by Curtis Olson, started March 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/io/iostreams/sgstream.hxx>

#include "stardata.hxx"

#if defined (_MSC_VER)
  using std::getline;
#endif

using std::string;

// Constructor
SGStarData::SGStarData( const SGPath& path )
{
    load(path);
}


// Destructor
SGStarData::~SGStarData() {
}


bool SGStarData::load( const SGPath& path ) {

    _stars.clear();

    // build the full path name to the stars data base file
    SGPath tmp = path;
    tmp.append( "stars" );
    SG_LOG( SG_ASTRO, SG_INFO, "  Loading stars from " << tmp );

    sg_gzifstream in( tmp );
    if ( ! in.is_open() ) {
	SG_LOG( SG_ASTRO, SG_ALERT, "Cannot open star file: " << tmp );
        return false;
    }

    double ra, dec, mag;
    char c;
    string name;

    // read in each line of the file
    while ( ! in.eof() ) {
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
        _stars.push_back(SGVec3d(ra, dec, mag));
    }

    SG_LOG( SG_ASTRO, SG_INFO, "  Loaded " << _stars.size() << " stars" );

    return true;
}
