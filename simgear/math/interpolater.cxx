//
// interpolater.cxx -- routines to handle linear interpolation from a table of
//                     x,y   The table must be sorted by "x" in ascending order
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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


#include <simgear/compiler.h>

#include <stdlib.h> // for exit()

#include STL_STRING

#include <simgear/fg_zlib.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgstream.hxx>

#include "interpolater.hxx"

FG_USING_STD(string);


// Constructor -- loads the interpolation table from the specified
// file
SGInterpTable::SGInterpTable( const string& file ) {
    FG_LOG( FG_MATH, FG_INFO, "Initializing Interpolator for " << file );

    fg_gzifstream in( file );
    if ( !in.is_open() ) {
        FG_LOG( FG_GENERAL, FG_ALERT, "Cannot open file: " << file );
	exit(-1);
    }

    size = 0;
    in >> skipcomment;
    while ( in ) {
	if ( size < MAX_TABLE_SIZE ) {
	    in >> table[size][0] >> table[size][1];
	    in >> skipws;
	    size++;
	} else {
            FG_LOG( FG_MATH, FG_ALERT,
		    "fgInterpolateInit(): Exceed max table size = "
		    << MAX_TABLE_SIZE );
	    exit(-1);
	}
    }
}


// Given an x value, linearly interpolate the y value from the table
double SGInterpTable::interpolate(double x) {
    int i;
    double y;

    i = 0;

    while ( (x > table[i][0]) && (i < size) ) {
	// cout << "  i = " << i << " table[i][0] = " << table[i][0] << endl;
	// cout << "  size = " << size << endl;
	i++;
    }

    // printf ("i = %d ", i);

    if ( (i == 0) && (x < table[0][0]) ) {
	FG_LOG( FG_MATH, FG_DEBUG, 
		"interpolate(): lookup error, x to small = " << x );
	return table[0][1];
    }

    // cout << " table[size-1][0] = " << table[size-1][0] << endl;
    if ( x > table[size-1][0] ) {
	FG_LOG( FG_MATH, FG_DEBUG, 
		"interpolate(): lookup error, x to big = " << x );
	return table[size-1][1];
    }

    // y = y1 + (y0 - y1)(x - x1) / (x0 - x1)
    y = table[i][1] + 
	( (table[i-1][1] - table[i][1]) * 
	  (x - table[i][0]) ) /
	(table[i-1][0] - table[i][0]);

    return(y);
}


// Destructor
SGInterpTable::~SGInterpTable( void ) {
}


