// animation.hxx - classes to manage model animation.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef _SG_ANIMATION_HXX
#define _SG_ANIMATION_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <vector>

SG_USING_STD(vector);

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/math/point3d.hxx>
#include <simgear/props/props.hxx>


// Don't pull in the headers, since we don't need them here.
class ssgBranch;
class ssgCutout;
class ssgEntity;
class ssgRangeSelector;
class ssgSelector;
class ssgTransform;

class SGInterpTable;
class FGCondition;
class FGLocation;


// Has anyone done anything *really* stupid, like making min and max macros?
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif



//////////////////////////////////////////////////////////////////////
// Animation classes
//////////////////////////////////////////////////////////////////////

/**
 * Abstract base class for all animations.
 */
class Animation :  public ssgBase
{
public:

  Animation (SGPropertyNode_ptr props, ssgBranch * branch);

  virtual ~Animation ();

  /**
   * Get the SSG branch holding the animation.
   */
  virtual ssgBranch * getBranch () { return _branch; }

  /**
   * Initialize the animation, after children have been added.
   */
  virtual void init ();

  /**
   * Update the animation.
   */
  virtual void update();

  /**
   * Set the value of sim_time_sec.  This needs to be called every
   * frame in order for the time based animations to work correctly.
   */
  static void set_sim_time_sec( double val ) { sim_time_sec = val; }

protected:

  static double sim_time_sec;

  ssgBranch * _branch;

};


/**
 * A no-op animation.
 */
class NullAnimation : public Animation
{
public:
  NullAnimation (SGPropertyNode_ptr props);
  virtual ~NullAnimation ();
};


/**
 * A range, or level-of-detail (LOD) animation.
 */
class RangeAnimation : public Animation
{
public:
  RangeAnimation (SGPropertyNode_ptr props);
  virtual ~RangeAnimation ();
};


/**
 * Animation to turn and face the screen.
 */
class BillboardAnimation : public Animation
{
public:
  BillboardAnimation (SGPropertyNode_ptr props);
  virtual ~BillboardAnimation ();
};


/**
 * Animation to select alternative versions of the same object.
 */
class SelectAnimation : public Animation
{
public:
  SelectAnimation( SGPropertyNode *prop_root,
                   SGPropertyNode_ptr props );
  virtual ~SelectAnimation ();
  virtual void update();
private:
  FGCondition * _condition;
};


/**
 * Animation to spin an object around a center point.
 *
 * This animation rotates at a specific velocity.
 */
class SpinAnimation : public Animation
{
public:
  SpinAnimation( SGPropertyNode *prop_root,
                 SGPropertyNode_ptr props,
                 double sim_time_sec );
  virtual ~SpinAnimation ();
  virtual void update();
private:
  SGPropertyNode_ptr _prop;
  double _factor;
  double _position_deg;
  double _last_time_sec;
  sgMat4 _matrix;
  sgVec3 _center;
  sgVec3 _axis;
};


/**
 * Animation to draw objects for a specific amount of time each.
 */
class TimedAnimation : public Animation
{
public:
    TimedAnimation (SGPropertyNode_ptr props);
    virtual ~TimedAnimation ();
    virtual void update();
private:
    double _duration_sec;
    double _last_time_sec;
    int _step;
};


/**
 * Animation to rotate an object around a center point.
 *
 * This animation rotates to a specific position.
 */
class RotateAnimation : public Animation
{
public:
  RotateAnimation( SGPropertyNode *prop_root, SGPropertyNode_ptr props );
  virtual ~RotateAnimation ();
  virtual void update();
private:
  SGPropertyNode_ptr _prop;
  double _offset_deg;
  double _factor;
  SGInterpTable * _table;
  bool _has_min;
  double _min_deg;
  bool _has_max;
  double _max_deg;
  double _position_deg;
  sgMat4 _matrix;
  sgVec3 _center;
  sgVec3 _axis;
};


/**
 * Animation to slide along an axis.
 */
class TranslateAnimation : public Animation
{
public:
  TranslateAnimation( SGPropertyNode *prop_root,
                      SGPropertyNode_ptr props );
  virtual ~TranslateAnimation ();
  virtual void update();
private:
  SGPropertyNode_ptr _prop;
  double _offset_m;
  double _factor;
  SGInterpTable * _table;
  bool _has_min;
  double _min_m;
  bool _has_max;
  double _max_m;
  double _position_m;
  sgMat4 _matrix;
  sgVec3 _axis;
};


#endif // _SG_ANIMATION_HXX
