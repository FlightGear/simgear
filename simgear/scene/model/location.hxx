// location.hxx -- class for determining model location in the flightgear world.
//
// Written by Jim Wilson, David Megginson, started April 2002.
//
// Copyright (C) 2002  Jim Wilson, David Megginson
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


#ifndef _SG_LOCATION_HXX
#define _SG_LOCATION_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/math/point3d.hxx>

#include <plib/sg.h>		// plib include


// Define a structure containing view information
class SGLocation
{

public:
    // Constructor
    SGLocation( void );

    // Destructor
    virtual ~SGLocation( void );

    //////////////////////////////////////////////////////////////////////
    // Part 2: user settings.
    //////////////////////////////////////////////////////////////////////

    // Geodetic position of model...
    virtual double getLongitude_deg () const { return _lon_deg; }
    virtual double getLatitude_deg () const { return _lat_deg; }
    virtual double getAltitudeASL_ft () const { return _alt_ft; }
    virtual void setPosition (double lon_deg, double lat_deg, double alt_ft);


    // Reference orientation rotations...
    //   These are rotations that represent the plane attitude effect on
    //   the view (in Pilot view).  IE The view frustrum rotates as the plane
    //   turns, pitches, and rolls.
    //   In model view (lookat/chaseview) these end up changing the angle that
    //   the eye is looking at the ojbect (ie the model).
    //   FIXME: the FGModel class should have its own version of these so that
    //   it can generate it's own model rotations.
    virtual double getRoll_deg () const { return _roll_deg; }
    virtual double getPitch_deg () const {return _pitch_deg; }
    virtual double getHeading_deg () const {return _heading_deg; }
    virtual void setOrientation (double roll_deg, double pitch_deg, double heading_deg);


    //////////////////////////////////////////////////////////////////////
    // Part 3: output vectors and matrices in FlightGear coordinates.
    //////////////////////////////////////////////////////////////////////

    // Vectors and positions...
    
    //! Get the absolute view position in fgfs coordinates.
    virtual double * get_absolute_view_pos( );
    
    //! Return the position relative to the given scenery center.
    virtual float * get_view_pos( const Point3D& scenery_center );
    
    // Get world up vector
    virtual float *get_world_up()
    { recalcAbsolutePosition(); return _world_up; }
    
    // Get surface east vector
    virtual float *get_surface_east()
    { recalcAbsolutePosition(); return _surface_east; }
    
    // Get surface south vector
    virtual float *get_surface_south()
    { recalcAbsolutePosition(); return _surface_south; }
    
    // Elevation of ground under location (based on scenery output)...
    void set_cur_elev_m ( double elev )      { _cur_elev_m = elev; }
    inline double get_cur_elev_m ()          { return _cur_elev_m; }
    
    // Matrices...
    virtual const sgVec4 *getTransformMatrix() {
        recalcOrientation();
        return TRANS;
    }
    virtual const sgVec4 *getCachedTransformMatrix() { return TRANS; }
    
    virtual const sgVec4 *getUpMatrix(const Point3D& scenery_center)  {
        recalcAbsolutePosition();
        return UP;
    }
    virtual const sgVec4 *getCachedUpMatrix() { return UP; }

private:

    //////////////////////////////////////////////////////////////////
    // private data                                                 //
    //////////////////////////////////////////////////////////////////

    // flag forcing a recalc of derived view parameters
    mutable bool _orientation_dirty, _position_dirty;

    mutable sgdVec3 _absolute_view_pos;
    mutable sgVec3 _relative_view_pos;

    double _lon_deg;
    double _lat_deg;
    double _alt_ft;

    double _roll_deg;
    double _pitch_deg;
    double _heading_deg;

    // elevation of ground under this location...
    double _cur_elev_m;

    // surface vector heading south
    mutable sgVec3 _surface_south;

    // surface vector heading east (used to unambiguously align sky
    // with sun)
    mutable sgVec3 _surface_east;

    // world up vector (normal to the plane tangent to the earth's
    // surface at the spot we are directly above)
    mutable sgVec3 _world_up;

    // sg versions of our friendly matrices
    mutable sgMat4 TRANS, UP;

    //////////////////////////////////////////////////////////////////
    // private functions                                            //
    //////////////////////////////////////////////////////////////////

    void recalcOrientation() const;
    void recalcAbsolutePosition() const;
};


#endif // _SG_LOCATION_HXX
