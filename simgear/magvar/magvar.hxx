// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2000 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief Magnetic variation wrapper class
 */

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
 * re-implementation of the NIMA WMM 2000 (not derived from their demo
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
    virtual ~SGMagVar();

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
