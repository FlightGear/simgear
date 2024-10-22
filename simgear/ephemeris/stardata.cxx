// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2000 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief Manage star data
 */

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
