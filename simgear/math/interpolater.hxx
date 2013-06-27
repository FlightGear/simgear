/**
 * \file interpolater.hxx
 * Routines to handle linear interpolation from a table of x,y The
 * table must be sorted by "x" in ascending order
 */

// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _INTERPOLATER_H
#define _INTERPOLATER_H


#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <simgear/structure/SGReferenced.hxx>

#include <map>
#include <string>

class SGPropertyNode;

/**
 * A class that provids a simple linear 2d interpolation lookup table.
 * The actual table is expected to be loaded from a file.  The
 * independant variable must be strictly ascending.  The dependent
 * variable can be anything.
 */
class SGInterpTable : public SGReferenced {
public:

    /**
     * Constructor. Creates a new, empty table.
     */
    SGInterpTable();

    /**
     * Constructor. Loads the interpolation table from an interpolation
     * property node.
     * @param interpolation property node having entry children
     */
    SGInterpTable(const SGPropertyNode* interpolation);

    /**
     * Constructor. Loads the interpolation table from the specified file.
     * @param file name of interpolation file
     */
    SGInterpTable( const std::string& file );


    /**
     * Add an entry to the table, extending the table's length.
     *
     * @param ind The independent variable.
     * @param dep The dependent variable.
     */
    void addEntry (double ind, double dep);
    

    /**
     * Given an x value, linearly interpolate the y value from the table.
     * @param x independent variable
     * @return interpolated dependent variable
     */
    double interpolate(double x) const;

    /** Destructor */
    ~SGInterpTable();

private:
    typedef std::map<double, double> Table;
    Table _table;
};


#endif // _INTERPOLATER_H


