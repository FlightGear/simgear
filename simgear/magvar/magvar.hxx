/** 
 * \file magvar.hxx
 * Magnetic variation wrapper class.
 */

// Written by Curtis Olson, started July 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _MAGVAR_HXX
#define _MAGVAR_HXX


#ifndef __cplusplus
# error This library requires C++
#endif


// forward decls
class SGGeod;

/**
 * Magnetic variation wrapper class.
 *
 * The SGMagVar class calculates the magnetic variation and dip for
 * any position, altitude, and time. It is a complete
 * re-implimentation of the NIMA WMM 2000 (not derived from their demo
 * code.)
 *
 * The SGMagVar class is really a simple wrapper around the core Ed
 * Williams code which does all the hard work.  This class allows you
 * to crunch the math once and then do multiple polls of the
 * data. However, if your position, altitude, or time has changed
 * significantly, you should call the update() method to recrunch new
 * numbers.
 */
class SGMagVar {

private:

    double magvar;
    double magdip;

public:

    /**
     * This creates an instance of the SGMagVar object.
     * You must call the update() method before any queries will be valid.
     */
    SGMagVar();

    /** Destructor */
    ~SGMagVar();

    /** Recalculate the magnetic offset and dip.
     * The update() method requires you to pass in your position and
     * the julian date. Lon and lat are specified in radians, altitude
     * is specified in meters. Julian date can be conveniently
     * calculated by using an instance of the SGTime class.
     * @param lon longitude in radians
     * @param lat latitude in radians
     * @param alt_m altitude above sea level in meters
     * @param jd julian date
     */
    void update( double lon, double lat, double alt_m, double jd );

    /**
     * overloaded variant taking an SGGeod to specify position
     */
    void update( const SGGeod& geod, double jd );

    /** @return the current magnetic variation in radians. */
    double get_magvar() const { return magvar; }

    /** @return the current magnetic dip in radians. */
    double get_magdip() const { return magdip; }
};


/**
 * \relates SGMagVar
 * Lookup the magvar for any arbitrary location (This function doesn't
 * save state like the SGMagVar class.  This function triggers a fair
 * amount of CPU work, so use it cautiously.
 * @return the magvar in radians
 */
double sgGetMagVar( double lon, double lat, double alt_m, double jd );

/**
 * overload version of the above to take a SGGeod
 */
double sgGetMagVar( const SGGeod& pos, double jd );

#endif // _MAGVAR_HXX
