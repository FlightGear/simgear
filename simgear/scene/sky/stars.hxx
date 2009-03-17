// stars.hxx -- model the stars
//
// Written by Durk Talsma. Originally started October 1997, for distribution  
// with the FlightGear project. Version 2 was written in August and 
// September 1998. This code is based upon algorithms and data kindly 
// provided by Mr. Paul Schlyter. (pausch@saaf.se). 
//
// Separated out rendering pieces and converted to ssg by Curt Olson,
// March 2000
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


#ifndef _SG_STARS_HXX_
#define _SG_STARS_HXX_


#include <osg/Array>

#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGReferenced.hxx>


class SGStars : public SGReferenced {

    osg::ref_ptr<osg::Vec4Array> cl;

    int old_phase;		// data for optimization

public:

    // Constructor
    SGStars( void );

    // Destructor
    ~SGStars( void );

    // initialize the stars structure
    osg::Node* build( int num, const SGVec3d star_data[], double star_dist );

    // repaint the planet magnitudes based on current value of
    // sun_angle in degrees relative to verticle (so we can make them
    // relatively dimmer during dawn and dusk
    // 0 degrees = high noon
    // 90 degrees = sun rise/set
    // 180 degrees = darkest midnight
    bool repaint( double sun_angle, int num, const SGVec3d star_data[] );
};


#endif // _SG_STARS_HXX_
