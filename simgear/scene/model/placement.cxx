// placement.cxx - manage the placment of a 3D model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <string.h>             // for strcmp()

#include <vector>

#include <plib/sg.h>
#include <plib/ssg.h>
#include <plib/ul.h>

#include "location.hxx"

#include "placement.hxx"

SG_USING_STD(vector);



////////////////////////////////////////////////////////////////////////
// Implementation of SGModelPlacement.
////////////////////////////////////////////////////////////////////////

SGModelPlacement::SGModelPlacement ()
  : _lon_deg(0),
    _lat_deg(0),
    _elev_ft(0),
    _roll_deg(0),
    _pitch_deg(0),
    _heading_deg(0),
    _selector(new ssgSelector),
    _position(new ssgTransform),
    _location(new SGLocation)
{
}

SGModelPlacement::~SGModelPlacement ()
{
}

void
SGModelPlacement::init( ssgBranch * model )
{
  if (model != 0) {
      _position->addKid(model);
  }
  _selector->addKid(_position);
  _selector->clrTraversalMaskBits(SSGTRAV_HOT);
}

void
SGModelPlacement::update( const Point3D scenery_center )
{
  _location->setPosition( _lon_deg, _lat_deg, _elev_ft );
  _location->setOrientation( _roll_deg, _pitch_deg, _heading_deg );

  sgCopyMat4( POS, _location->getTransformMatrix(scenery_center) );

  sgVec3 trans;
  sgCopyVec3(trans, _location->get_view_pos());

  for(int i = 0; i < 4; i++) {
    float tmp = POS[i][3];
    for( int j=0; j<3; j++ ) {
      POS[i][j] += (tmp * trans[j]);
    }
  }
  _position->setTransform(POS);
}

bool
SGModelPlacement::getVisible () const
{
  return (_selector->getSelect() != 0);
}

void
SGModelPlacement::setVisible (bool visible)
{
  _selector->select(visible);
}

void
SGModelPlacement::setLongitudeDeg (double lon_deg)
{
  _lon_deg = lon_deg;
}

void
SGModelPlacement::setLatitudeDeg (double lat_deg)
{
  _lat_deg = lat_deg;
}

void
SGModelPlacement::setElevationFt (double elev_ft)
{
  _elev_ft = elev_ft;
}

void
SGModelPlacement::setPosition (double lon_deg, double lat_deg, double elev_ft)
{
  _lon_deg = lon_deg;
  _lat_deg = lat_deg;
  _elev_ft = elev_ft;
}

void
SGModelPlacement::setRollDeg (double roll_deg)
{
  _roll_deg = roll_deg;
}

void
SGModelPlacement::setPitchDeg (double pitch_deg)
{
  _pitch_deg = pitch_deg;
}

void
SGModelPlacement::setHeadingDeg (double heading_deg)
{
  _heading_deg = heading_deg;
}

void
SGModelPlacement::setOrientation (double roll_deg, double pitch_deg,
                                  double heading_deg)
{
  _roll_deg = roll_deg;
  _pitch_deg = pitch_deg;
  _heading_deg = heading_deg;
}

// end of model.cxx
