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

#pragma once

#include <osg/MatrixTransform>

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/props/propsfwd.hxx>

namespace simgear {
class SGReaderWriterOptions;
}

class SGSun : public SGReferenced {
public:
    SGSun() = default;

    // return the sun object
    osg::Node* build(double sun_size, SGPropertyNode* property_tree_Node,
                     const simgear::SGReaderWriterOptions* options);

    /*
     * Reposition the sun at the specified right ascension and declination,
     * offset by our current position (p) so that it appears fixed at a great
     * distance from the viewer. Also add in an optional rotation (i.e. for the
     * current time of day).
     */
    bool reposition(double rightAscension, double declination,
                    double sun_dist, double lat, double alt_asl, double sun_angle);

private:
    double prev_sun_angle{-9999.0};
    osg::ref_ptr<osg::MatrixTransform> sun_transform;
    SGPropertyNode_ptr env_node;
};
