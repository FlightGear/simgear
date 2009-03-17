// moon.hxx -- model earth's moon
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


#ifndef _SG_MOON_HXX_
#define _SG_MOON_HXX_


#include <osg/ref_ptr>
#include <osg/MatrixTransform>
#include <osg/Material>

#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGReferenced.hxx>

#include <simgear/misc/sg_path.hxx>


class SGMoon : public SGReferenced {

    osg::ref_ptr<osg::MatrixTransform> moon_transform;
    osg::ref_ptr<osg::Material> orb_material;

    double prev_moon_angle;

public:

    // Constructor
    SGMoon( void );

    // Destructor
    ~SGMoon( void );

    // build the moon object
    osg::Node *build( SGPath path, double moon_size );

    // repaint the moon colors based on current value of moon_anglein
    // degrees relative to verticle
    // 0 degrees = high noon
    // 90 degrees = moon rise/set
    // 180 degrees = darkest midnight
    bool repaint( double moon_angle );

    // reposition the moon at the specified right ascension and
    // declination, offset by our current position (p) so that it
    // appears fixed at a great distance from the viewer.  Also add in
    // an optional rotation (i.e. for the current time of day.)
    bool reposition( double rightAscension, double declination,
		     double moon_dist  );
};


#endif // _SG_MOON_HXX_
