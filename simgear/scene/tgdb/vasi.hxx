// vasi.hxx -- a class to hold some critical vasi data
//
// Written by Curtis Olson, started December 2003.
//
// Copyright (C) 2003  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _SG_VASI_HXX
#define _SG_VASI_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <simgear/compiler.h>

#include STL_STRING
SG_USING_STD(string);

#include <plib/ssg.h>		// plib include

#include <simgear/math/sg_geodesy.hxx>


class SGVASIUserData : public ssgBase
{

private:

    sgdVec3 abs_pos;
    double alt_m;
    ssgLeaf *leaf;

public:

    SGVASIUserData( sgdVec3 pos_cart, ssgLeaf *l ) {
        sgdCopyVec3( abs_pos, pos_cart );

        double lat, lon;
        sgCartToGeod( abs_pos, &lat, &lon, &alt_m );

        leaf = l;
    }

    ~SGVASIUserData() {}

    double get_alt_m() { return alt_m; }
    double *get_abs_pos() { return abs_pos; }
    int i;

    // color the vasi/papi correctly based on angle
    void set_color( float angle_deg ) {
        int count = leaf->getNumColours();
        double trans = 0.05;
        double color = 1.0;
        double ref;
        float *entry;

        if ( count == 12 ) {
            // PAPI configuration

            // papi D
            ref = 3.5;
            if ( angle_deg < ref - trans ) {
                color = 0.0;
            } else if ( angle_deg < ref + trans ) {
                color = 1.0 - (ref + trans - angle_deg) * (1 / (2 * trans) );
            } else {
                color = 1.0;
            }
            for ( i = 0; i < 3; ++i ) {
                entry = leaf->getColour( i );
                entry[1] = color;
                entry[2] = color;
            }

            // papi C
            ref = 3.167;
            if ( angle_deg < ref - trans ) {
                color = 0.0;
            } else if ( angle_deg < ref + trans ) {
                color = 1.0 - (ref + trans - angle_deg) * (1 / (2 * trans) );
            } else {
                color = 1.0;
            }
            for ( i = 3; i < 6; ++i ) {
                entry = leaf->getColour( i );
                entry[1] = color;
                entry[2] = color;
            }

            // papi B
            ref = 2.833;
            if ( angle_deg < ref - trans ) {
                color = 0.0;
            } else if ( angle_deg < ref + trans ) {
                color = 1.0 - (ref + trans - angle_deg) * (1 / (2 * trans) );
            } else {
                color = 1.0;
            }
            for ( i = 6; i < 9; ++i ) {
                entry = leaf->getColour( i );
                entry[1] = color;
                entry[2] = color;
            }

            // papi A
            ref = 2.5;
            if ( angle_deg < ref - trans ) {
                color = 0.0;
            } else if ( angle_deg < ref + trans ) {
                color = 1.0 - (ref + trans - angle_deg) * (1 / (2 * trans) );
            } else {
                color = 1.0;
            }
            for ( i = 9; i < 12; ++i ) {
                entry = leaf->getColour( i );
                entry[1] = color;
                entry[2] = color;
            }
        } else if ( count == 36 ) {
            // probably vasi, first 18 are downwind bar (2.5 deg)
            ref = 2.5;
            if ( angle_deg < ref - trans ) {
                color = 0.0;
            } else if ( angle_deg < ref + trans ) {
                color = 1.0 - (ref + trans - angle_deg) * (1 / (2 * trans) );
            } else {
                color = 1.0;
            }
            for ( int i = 0; i < 18; ++i ) {
                entry = leaf->getColour( i );
                entry[1] = color;
                entry[2] = color;
            }

            // last 6 are upwind bar (3.0 deg)
            ref = 3.0;
            if ( angle_deg < ref - trans ) {
                color = 0.0;
            } else if ( angle_deg < ref + trans ) {
                color = 1.0 - (ref + trans - angle_deg) * (1 / (2 * trans) );
            } else {
                color = 1.0;
            }
            for ( int i = 18; i < 36; ++i ) {
                entry = leaf->getColour( i );
                entry[1] = color;
                entry[2] = color;
            }
        } else {
            // fail safe
            cout << "unknown vasi/papi configuration, count = " << count << endl;
            for ( int i = 0; i < count; ++i ) {
                entry = leaf->getColour( i );
                entry[1] = color;
                entry[2] = color;
            }
        }
    }
};


#endif // _SG_VASI_HXX
