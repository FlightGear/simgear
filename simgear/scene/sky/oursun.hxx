// oursun.hxx -- model earth's sun
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


#ifndef _SG_SUN_HXX_
#define _SG_SUN_HXX_

#include <osg/Array>
#include <osg/Node>
#include <osg/MatrixTransform>

#include <simgear/structure/SGReferenced.hxx>

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>

class SGSun : public SGReferenced {

    osg::ref_ptr<osg::MatrixTransform> sun_transform;

    osg::ref_ptr<osg::Vec4Array> sun_cl;
    osg::ref_ptr<osg::Vec4Array> scene_cl;
    osg::ref_ptr<osg::Vec4Array> ihalo_cl;
    osg::ref_ptr<osg::Vec4Array> ohalo_cl;

    double visibility;
    double prev_sun_angle;
    // distance of light traveling through the atmosphere
    double path_distance;
    double sun_exp2_punch_through;
    double horizon_angle;

    SGPropertyNode_ptr env_node;

public:

    // Constructor
    SGSun( void );

    // Destructor
    ~SGSun( void );

    // return the sun object
    osg::Node* build( SGPath path, double sun_size, SGPropertyNode *property_tree_Node );

    // repaint the sun colors based on current value of sun_anglein
    // degrees relative to verticle
    // 0 degrees = high noon
    // 90 degrees = sun rise/set
    // 180 degrees = darkest midnight
    bool repaint( double sun_angle, double new_visibility );

    // reposition the sun at the specified right ascension and
    // declination, offset by our current position (p) so that it
    // appears fixed at a great distance from the viewer.  Also add in
    // an optional rotation (i.e. for the current time of day.)
    bool reposition( double rightAscension, double declination,
		     double sun_dist, double lat, double alt_asl, double sun_angle );

    // retrun the current color of the sun
    SGVec4f get_color();
    SGVec4f get_scene_color();
};


#endif // _SG_SUN_HXX_
