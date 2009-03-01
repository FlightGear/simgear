// placement.cxx - manage the placment of a 3D model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <simgear/scene/util/SGSceneUserData.hxx>

#include "location.hxx"
#include "placementtrans.hxx"

#include "placement.hxx"



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
    _selector(new osg::Switch),
    _position(new SGPlacementTransform),
    _location(new SGLocation)
{
}

SGModelPlacement::~SGModelPlacement ()
{
  delete _location;
}

void
SGModelPlacement::init( osg::Node * model )
{
  if (model != 0) {
      _position->addChild(model);
  }
  _selector->addChild(_position.get());
  _selector->setValue(0, 1);
}

void
SGModelPlacement::update()
{
  _location->setPosition( _lon_deg, _lat_deg, _elev_ft );
  _location->setOrientation( _roll_deg, _pitch_deg, _heading_deg );

  const sgVec4 *t = _location->getTransformMatrix();
  SGMatrixd rotation;
  for (unsigned i = 0; i < 4; ++i)
    for (unsigned j = 0; j < 4; ++j)
      rotation(i, j) = t[j][i];
  SGVec3d pos(_location->get_absolute_view_pos());
  _position->setTransform(pos, rotation);
}

bool
SGModelPlacement::getVisible () const
{
  return _selector->getValue(0);
}

void
SGModelPlacement::setVisible (bool visible)
{
  _selector->setValue(0, visible);
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
SGModelPlacement::setPosition(const SGGeod& position)
{
  _lon_deg = position.getLongitudeDeg();
  _lat_deg = position.getLatitudeDeg();
  _elev_ft = position.getElevationFt();
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

void
SGModelPlacement::setOrientation (const SGQuatd& orientation)
{
  orientation.getEulerDeg(_heading_deg, _pitch_deg, _roll_deg);
}

void
SGModelPlacement::setBodyLinearVelocity(const SGVec3d& linear)
{
  SGSceneUserData* userData;
  userData = SGSceneUserData::getOrCreateSceneUserData(_position);
  SGSceneUserData::Velocity* vel = userData->getOrCreateVelocity();
  SGQuatd orientation = SGQuatd::fromAngleAxisDeg(180, SGVec3d(0, 1, 0));
  vel->linear = orientation.backTransform(linear);
}

void
SGModelPlacement::setBodyAngularVelocity(const SGVec3d& angular)
{
  SGSceneUserData* userData;
  userData = SGSceneUserData::getOrCreateSceneUserData(_position);
  SGSceneUserData::Velocity* vel = userData->getOrCreateVelocity();
  SGQuatd orientation = SGQuatd::fromAngleAxisDeg(180, SGVec3d(0, 1, 0));
  vel->angular = orientation.backTransform(angular);
}

// end of model.cxx
