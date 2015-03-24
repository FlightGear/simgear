// magvar.cxx -- magnetic variation wrapper class
//
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

