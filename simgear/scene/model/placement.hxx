// placement.hxx - manage the placment of a 3D model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.


#ifndef _SG_PLACEMENT_HXX
#define _SG_PLACEMENT_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/Switch>
#include <osg/PositionAttitudeTransform>

#include <simgear/math/SGMath.hxx>

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

  virtual void init( osg::Node* model );
  void clear();
  void add( osg::Node* model );
  
  virtual void update();

  virtual osg::Node* getSceneGraph () { return _selector.get(); }

  virtual bool getVisible () const;
  virtual void setVisible (bool visible);

  void setPosition(const SGGeod& position);
  const SGGeod& getPosition() const { return _position; }

  virtual double getRollDeg () const { return _roll_deg; }
  virtual double getPitchDeg () const { return _pitch_deg; }
  virtual double getHeadingDeg () const { return _heading_deg; }

  virtual void setRollDeg (double roll_deg);
  virtual void setPitchDeg (double pitch_deg);
  virtual void setHeadingDeg (double heading_deg);
  virtual void setOrientation (double roll_deg, double pitch_deg,
                               double heading_deg);
  void setOrientation(const SGQuatd& orientation);

  void setReferenceTime(const double& referenceTime);
  void setBodyLinearVelocity(const SGVec3d& velocity);
  void setBodyAngularVelocity(const SGVec3d& velocity);
  
private:
                                // Geodetic position
  SGGeod _position;

                                // Orientation
  double _roll_deg;
  double _pitch_deg;
  double _heading_deg;

  osg::ref_ptr<osg::Switch> _selector;
  osg::ref_ptr<osg::PositionAttitudeTransform> _transform;
};

#endif // _SG_PLACEMENT_HXX
