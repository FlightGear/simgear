// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2000 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief Magnetic variation wrapper class
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif


#include <cmath>

#include <simgear/magvar/magvar.hxx>
#include <simgear/math/SGMath.hxx>

#include "coremag.hxx"
#include "magvar.hxx"


SGMagVar::SGMagVar()
  : magvar(0.0),
    magdip(0.0)
{
}

SGMagVar::~SGMagVar() {
}


void SGMagVar::update( double lon, double lat, double alt_m, double jd ) {
    // Calculate local magnetic variation
    double field[6];
    // cout << "alt_m = " << alt_m << endl;
    magvar = calc_magvar( lat, lon, alt_m / 1000.0, (long)jd, field );
    magdip = atan(field[5]/sqrt(field[3]*field[3]+field[4]*field[4]));
}

void SGMagVar::update( const SGGeod& geod, double jd ) {

  update(geod.getLongitudeRad(), geod.getLatitudeRad(),
    geod.getElevationM(), jd);
}


double sgGetMagVar( double lon, double lat, double alt_m, double jd ) {
    // cout << "lat = " << lat << " lon = " << lon << " elev = " << alt_m
    //      << " JD = " << jd << endl;

    double field[6];
    return calc_magvar( lat, lon, alt_m / 1000.0, (long)jd, field );
}

double sgGetMagVar( const SGGeod& pos, double jd )
{
  return sgGetMagVar(pos.getLongitudeRad(), pos.getLatitudeRad(), 
    pos.getElevationM(), jd);
}

