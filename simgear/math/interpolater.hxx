// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1998 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief Routines to handle linear interpolation from a table of x,y. The table must be sorted by "x" in ascending order
 */

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
class SGPath;

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
     * Constructor. Loads the interpolation table from the specified file.
     * @param file name of interpolation file
     */
    SGInterpTable( const SGPath& path );
    
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
    virtual ~SGInterpTable();

private:
    typedef std::map<double, double> Table;
    Table _table;
};


#endif // _INTERPOLATER_H


