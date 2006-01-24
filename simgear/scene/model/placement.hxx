// placement.hxx - manage the placment of a 3D model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.


#ifndef _SG_PLACEMENT_HXX
#define _SG_PLACEMENT_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/math/point3d.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/ssgSharedPtr.hxx>


// Don't pull in the headers, since we don't need them here.
class SGLocation;
class ssgPlacementTransform;


// Has anyone done anything *really* stupid, like making min and max macros?
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


////////////////////////////////////////////////////////////////////////
// Model placement.
////////////////////////////////////////////////////////////////////////

/**
 * A wrapper for a model with a definite placement.
 */
class SGModelPlacement
{
public:

  SGModelPlacement ();
  virtual ~SGModelPlacement ();

  virtual void init( ssgBranch * model );

  virtual void update();

  virtual ssgEntity * getSceneGraph () { return (ssgEntity *)_selector; }

  virtual SGLocation * getSGLocation () { return _location; }

  virtual bool getVisible () const;
  virtual void setVisible (bool visible);

  virtual double getLongitudeDeg () const { return _lon_deg; }
  virtual double getLatitudeDeg () const { return _lat_deg; }
  virtual double getElevationFt () const { return _elev_ft; }

  virtual void setLongitudeDeg (double lon_deg);
  virtual void setLatitudeDeg (double lat_deg);
  virtual void setElevationFt (double elev_ft);
  virtual void setPosition (double lon_deg, double lat_deg, double elev_ft);

  virtual double getRollDeg () const { return _roll_deg; }
  virtual double getPitchDeg () const { return _pitch_deg; }
  virtual double getHeadingDeg () const { return _heading_deg; }

  virtual void setRollDeg (double roll_deg);
  virtual void setPitchDeg (double pitch_deg);
  virtual void setHeadingDeg (double heading_deg);
  virtual void setOrientation (double roll_deg, double pitch_deg,
                               double heading_deg);
  
  ssgPlacementTransform * getTransform(void)
  { return _position; }

private:

                                // Geodetic position
  double _lon_deg;
  double _lat_deg;
  double _elev_ft;

                                // Orientation
  double _roll_deg;
  double _pitch_deg;
  double _heading_deg;

  ssgSharedPtr<ssgSelector> _selector;
  ssgSharedPtr<ssgPlacementTransform> _position;

                                // Location
  SGLocation * _location;
};

#endif // _SG_PLACEMENT_HXX
