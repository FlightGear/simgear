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

// depricated - #include <simgear/sg_zlib.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>

#include "interpolater.hxx"

SG_USING_STD(string);

// Constructor -- starts with an empty table.
SGInterpTable::SGInterpTable()
    : size(0)
{
}


// Constructor -- loads the interpolation table from the specified
// file
SGInterpTable::SGInterpTable( const string& file ) 
  : size(0)
{
    SG_LOG( SG_MATH, SG_INFO, "Initializing Interpolator for " << file );

    sg_gzifstream in( file );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << file );
	exit(-1);
    }

    in >> skipcomment;
    while ( in ) {
      double ind, dep;
      in >> ind >> dep;
      in >> skipws;
      table.push_back(Entry(ind, dep));
      size++;
    }
}


// Add an entry to the table.
void SGInterpTable::addEntry (double ind, double dep)
{
  table.push_back(Entry(ind,dep));
  size++;
}


// Given an x value, linearly interpolate the y value from the table
double SGInterpTable::interpolate(double x) const
{
    int i;
    double y;

    if (size == 0.0)
      return 0.0;

    i = 0;

    while ( (i < size) && (x > table[i].ind) ) {
	// cout << "  i = " << i << " table[i].ind = " << table[i].ind << endl;
	// cout << "  size = " << size << endl;
	i++;
    }

    // printf ("i = %d ", i);

    if ( (i == 0) && (x < table[0].ind) ) {
	SG_LOG( SG_MATH, SG_DEBUG, 
		"interpolate(): lookup error, x to small = " << x );
	return table[0].dep;
    }

    // cout << " table[size-1].ind = " << table[size-1].ind << endl;
    if ( x > table[size-1].ind ) {
	SG_LOG( SG_MATH, SG_DEBUG, 
		"interpolate(): lookup error, x to big = " << x );
	return table[size-1].dep;
    }

    // y = y1 + (y0 - y1)(x - x1) / (x0 - x1)
    y = table[i].dep + 
	( (table[i-1].dep - table[i].dep) * 
	  (x - table[i].ind) ) /
	(table[i-1].ind - table[i].ind);

    return(y);
}


// Destructor
SGInterpTable::~SGInterpTable() {
}


