// magvar.hxx -- magnetic variation wrapper class
//
// Written by Curtis Olson, started July 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifndef _MAGVAR_HXX
#define _MAGVAR_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


class SGMagVar {

private:

    double magvar;
    double magdip;

public:

    SGMagVar();
    ~SGMagVar();

    // recalculate the magnetic offset and dip
    void update( double lon, double lat, double alt_m, double jd );

    double get_magvar() const { return magvar; }
    double get_magdip() const { return magdip; }
};


// lookup the magvar for any arbitrary location (doesn't save state
// and note that this is a fair amount of cpu work)
double sgGetMagVar( double lon, double lat, double alt_m, double jd );


#endif // _MAGVAR_HXX
