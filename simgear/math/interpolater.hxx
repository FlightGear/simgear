/**
 * \file interpolater.hxx
 * Routines to handle linear interpolation from a table of x,y The
 * table must be sorted by "x" in ascending order
 */

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


#ifndef _INTERPOLATER_H
#define _INTERPOLATER_H


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/compiler.h>

#include STL_STRING
SG_USING_STD(string);

#define MAX_TABLE_SIZE 32

/**
 * A class that provids a simple linear 2d interpolation lookup table.
 * The actual table is expected to be loaded from a file.  The
 * independant variable must be strictly ascending.  The dependent
 * variable can be anything.
 */
class SGInterpTable {
    int size;
    double table[MAX_TABLE_SIZE][2];

public:

    /**
     * Constructor. Loads the interpolation table from the specified file.
     * @param file name of interpolation file
     */
    SGInterpTable( const string& file );

    /**
     * Given an x value, linearly interpolate the y value from the table.
     * @param x independent variable
     * @return interpolated dependent variable
     */
    double interpolate(double x);

    /** Destructor */
    ~SGInterpTable();
};


#endif // _INTERPOLATER_H


