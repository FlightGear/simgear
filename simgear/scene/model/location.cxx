// location.cxx -- class for determining model location in the flightgear world.
//
// Written by Jim Wilson, David Megginson, started April 2002.
// Based largely on code by Curtis Olson and Norman Vine.
//
// Copyright (C) 2002  Curtis L. Olson - http://www.flightgear.org/~curt
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


#include <simgear/compiler.h>

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/constants.h>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/vector.hxx>

#include "location.hxx"


/**
 * make model transformation Matrix - based on optimizations by NHV
 */
static void MakeTRANS( sgMat4 dst, const double Theta,
			const double Phi, const double Psi, 
                        const sgMat4 UP)
{
    SGfloat cosTheta = (SGfloat) cos(Theta);
    SGfloat sinTheta = (SGfloat) sin(Theta);
    SGfloat cosPhi   = (SGfloat) cos(Phi);
    SGfloat sinPhi   = (SGfloat) sin(Phi);
    SGfloat sinPsi   = (SGfloat) sin(Psi) ;
    SGfloat cosPsi   = (SGfloat) cos(Psi) ;

    sgMat4 tmp;
	
    tmp[0][0] = cosPhi * cosTheta;
    tmp[0][1] =	sinPhi * cosPsi + cosPhi * -sinTheta * -sinPsi;
    tmp[0][2] =	sinPhi * sinPsi + cosPhi * -sinTheta * cosPsi;

    tmp[1][0] = -sinPhi * cosTheta;
    tmp[1][1] =	cosPhi * cosPsi + -sinPhi * -sinTheta * -sinPsi;
    tmp[1][2] =	cosPhi * sinPsi + -sinPhi * -sinTheta * cosPsi;
	
    tmp[2][0] = sinTheta;
    tmp[2][1] =	cosTheta * -sinPsi;
    tmp[2][2] =	cosTheta * cosPsi;
	
    float a = UP[0][0];
    float b = UP[1][0];
    float c = UP[2][0];
    dst[2][0] = a*tmp[0][0] + b*tmp[0][1] + c*tmp[0][2] ;
    dst[1][0] = a*tmp[1][0] + b*tmp[1][1] + c*tmp[1][2] ;
    dst[0][0] = -(a*tmp[2][0] + b*tmp[2][1] + c*tmp[2][2]) ;
    dst[3][0] = SG_ZERO ;

    a = UP[0][1];
    b = UP[1][1];
    c = UP[2][1];
    dst[2][1] = a*tmp[0][0] + b*tmp[0][1] + c*tmp[0][2] ;
    dst[1][1] = a*tmp[1][0] + b*tmp[1][1] + c*tmp[1][2] ;
    dst[0][1] = -(a*tmp[2][0] + b*tmp[2][1] + c*tmp[2][2]) ;
    dst[3][1] = SG_ZERO ;

    a = UP[0][2];
    c = UP[2][2];
    dst[2][2] = a*tmp[0][0] + c*tmp[0][2] ;
    dst[1][2] = a*tmp[1][0] + c*tmp[1][2] ;
    dst[0][2] = -(a*tmp[2][0] + c*tmp[2][2]) ;
    dst[3][2] = SG_ZERO ;

    dst[2][3] = SG_ZERO ;
    dst[1][3] = SG_ZERO ;
    dst[0][3] = SG_ZERO ;
    dst[3][3] = SG_ONE ;
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGLocation.
////////////////////////////////////////////////////////////////////////

// Constructor
SGLocation::SGLocation( void ):
    _orientation_dirty(true),
    _position_dirty(true),
    _lon_deg(-1000),
    _lat_deg(0),
    _alt_ft(0),
    _roll_deg(0),
    _pitch_deg(0),
    _heading_deg(0),
    _cur_elev_m(0)
{
    sgdZeroVec3(_absolute_view_pos);
    sgMakeRotMat4( UP, 0.0, 0.0, 0.0 );
    sgMakeRotMat4( TRANS, 0.0, 0.0, 0.0 );
}


// Destructor
SGLocation::~SGLocation( void ) {
}

void
SGLocation::setPosition (double lon_deg, double lat_deg, double alt_ft)
{
  _position_dirty = true;
  _lon_deg = lon_deg;
  _lat_deg = lat_deg;
  _alt_ft = alt_ft;
}

void
SGLocation::setOrientation (double roll_deg, double pitch_deg, double heading_deg)
{
  _orientation_dirty = true;
  _roll_deg = roll_deg;
  _pitch_deg = pitch_deg;
  _heading_deg = heading_deg;
}

double *
SGLocation::get_absolute_view_pos()
{
    recalcAbsolutePosition();
    return _absolute_view_pos;
}

float *
SGLocation::get_view_pos( const Point3D& scenery_center ) 
{
    recalcAbsolutePosition();
    for (int i = 0; i < 3; i++)
        _relative_view_pos[i] = _absolute_view_pos[i] - scenery_center[i];
    return _relative_view_pos;
}

void
SGLocation::recalcOrientation() const
{
  if (_orientation_dirty) {
    // Make sure UP matrix is up-to-date.
    recalcAbsolutePosition();

    // Create local matrix with current geodetic position.  Converting
    // the orientation (pitch/roll/heading) to vectors.
    MakeTRANS( TRANS, _pitch_deg * SG_DEGREES_TO_RADIANS,
                      _roll_deg * SG_DEGREES_TO_RADIANS,
                     -_heading_deg * SG_DEGREES_TO_RADIANS,
                      UP );
    _orientation_dirty = false;
  }
}

/*
 * Update values derived from the longitude, latitude and altitude parameters
 * of this instance. This encompasses absolute position in cartesian 
 * coordinates, the local up, east and south vectors and the UP Matrix.
 */
void
SGLocation::recalcAbsolutePosition() const
{
  if (_position_dirty) {
    double lat = _lat_deg * SGD_DEGREES_TO_RADIANS;
    double lon = _lon_deg * SGD_DEGREES_TO_RADIANS;
    double alt = _alt_ft * SG_FEET_TO_METER;

    sgGeodToCart(lat, lon, alt, _absolute_view_pos);
    
     // Make the world up rotation matrix for eye positioin...
    sgMakeRotMat4( UP, _lon_deg, 0.0, -_lat_deg );

    // get the world up radial vector from planet center for output
    sgSetVec3( _world_up, UP[0][0], UP[0][1], UP[0][2] );

    // Calculate the surface east and south vectors using the (normalized)
    // partial derivatives of the up vector Could also be fetched and
    // normalized from the UP rotation matrix, but I doubt this would
    // be more efficient.
    float sin_lon = sin(_lon_deg * SGD_DEGREES_TO_RADIANS);
    float sin_lat = sin(_lat_deg * SGD_DEGREES_TO_RADIANS);
    float cos_lon = cos(_lon_deg * SGD_DEGREES_TO_RADIANS);
    float cos_lat = cos(_lat_deg * SGD_DEGREES_TO_RADIANS);
 
    _surface_south[0] = (sin_lat*cos_lon);
    _surface_south[1] = (sin_lat*sin_lon);
    _surface_south[2] = - cos_lat;
  
    _surface_east[0] = -sin_lon;
    _surface_east[1] = cos_lon;
    _surface_east[2] = 0.f;

    _position_dirty = false;
  }
}
