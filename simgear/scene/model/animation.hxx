
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
class SGInterpTable;
class SGCondition;


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
class SGAnimation :  public ssgBase
{
public:

  SGAnimation (SGPropertyNode_ptr props, ssgBranch * branch);

  virtual ~SGAnimation ();

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
class SGNullAnimation : public SGAnimation
{
public:
  SGNullAnimation (SGPropertyNode_ptr props);
  virtual ~SGNullAnimation ();
};


/**
 * A range, or level-of-detail (LOD) animation.
 */
class SGRangeAnimation : public SGAnimation
{
public:
  SGRangeAnimation (SGPropertyNode *prop_root,
                    SGPropertyNode_ptr props);
  virtual ~SGRangeAnimation ();
  virtual void update();
private:
  SGPropertyNode_ptr _min_prop;
  SGPropertyNode_ptr _max_prop;
  float _min;
  float _max;
  float _min_factor;
  float _max_factor;
};


/**
 * Animation to turn and face the screen.
 */
class SGBillboardAnimation : public SGAnimation
{
public:
  SGBillboardAnimation (SGPropertyNode_ptr props);
  virtual ~SGBillboardAnimation ();
};


/**
 * Animation to select alternative versions of the same object.
 */
class SGSelectAnimation : public SGAnimation
{
public:
  SGSelectAnimation( SGPropertyNode *prop_root,
                   SGPropertyNode_ptr props );
  virtual ~SGSelectAnimation ();
  virtual void update();
private:
  SGCondition * _condition;
};


/**
 * Animation to spin an object around a center point.
 *
 * This animation rotates at a specific velocity.
 */
class SGSpinAnimation : public SGAnimation
{
public:
  SGSpinAnimation( SGPropertyNode *prop_root,
                 SGPropertyNode_ptr props,
                 double sim_time_sec );
  virtual ~SGSpinAnimation ();
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
class SGTimedAnimation : public SGAnimation
{
public:
    SGTimedAnimation (SGPropertyNode_ptr props);
    virtual ~SGTimedAnimation ();
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
class SGRotateAnimation : public SGAnimation
{
public:
  SGRotateAnimation( SGPropertyNode *prop_root, SGPropertyNode_ptr props );
  virtual ~SGRotateAnimation ();
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
class SGTranslateAnimation : public SGAnimation
{
public:
  SGTranslateAnimation( SGPropertyNode *prop_root,
                      SGPropertyNode_ptr props );
  virtual ~SGTranslateAnimation ();
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

/**
 * Animation to blend an object.
 */
class SGBlendAnimation : public SGAnimation
{
public:
  SGBlendAnimation( SGPropertyNode *prop_root,
                      SGPropertyNode_ptr props );
  virtual ~SGBlendAnimation ();
  virtual void update();
private:
  SGPropertyNode_ptr _prop;
  SGInterpTable * _table;
  double _prev_value;
  double _offset;
  double _factor;
  bool _has_min;
  double _min;
  bool _has_max;
  double _max;
};

/**
 * Animation to scale an object.
 */
class SGScaleAnimation : public SGAnimation
{
public:
  SGScaleAnimation( SGPropertyNode *prop_root,
                        SGPropertyNode_ptr props );
  virtual ~SGScaleAnimation ();
  virtual void update();
private:
  SGPropertyNode_ptr _prop;
  double _x_factor;
  double _y_factor;
  double _z_factor;
  double _x_offset;
  double _y_offset;
  double _z_offset;
  SGInterpTable * _table;
  bool _has_min_x;
  bool _has_min_y;
  bool _has_min_z;
  double _min_x;
  double _min_y;
  double _min_z;
  bool _has_max_x;
  bool _has_max_y;
  bool _has_max_z;
  double _max_x;
  double _max_y;
  double _max_z;
  double _x_scale;
  double _y_scale;
  double _z_scale;
  sgMat4 _matrix;
};

/**
 * Animation to rotate texture mappings around a center point.
 *
 * This animation rotates to a specific position.
 */
class SGTexRotateAnimation : public SGAnimation
{
public:
  SGTexRotateAnimation( SGPropertyNode *prop_root, SGPropertyNode_ptr props );
  virtual ~SGTexRotateAnimation ();
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
 * Animation to slide texture mappings along an axis.
 */
class SGTexTranslateAnimation : public SGAnimation
{
public:
  SGTexTranslateAnimation( SGPropertyNode *prop_root,
                      SGPropertyNode_ptr props );
  virtual ~SGTexTranslateAnimation ();
  virtual void update();
private:
  SGPropertyNode_ptr _prop;
  double _offset;
  double _factor;
  double _step;
  double _scroll;
  SGInterpTable * _table;
  bool _has_min;
  double _min;
  bool _has_max;
  double _max;
  double _position;
  sgMat4 _matrix;
  sgVec3 _axis;
};



/**
 * Classes for handling multiple types of Texture translations on one object
 */

class SGTexMultipleAnimation : public SGAnimation
{
public:
  SGTexMultipleAnimation( SGPropertyNode *prop_root,
                      SGPropertyNode_ptr props );
  virtual ~SGTexMultipleAnimation ();
  virtual void update();
private:
  class TexTransform
    {
    public:
    SGPropertyNode_ptr prop;
    int subtype; //  0=translation, 1=rotation
    double offset;
    double factor;
    double step;
    double scroll;
    SGInterpTable * table;
    bool has_min;
    double min;
    bool has_max;
    double max;
    double position;
    sgMat4 matrix;
    sgVec3 center;
    sgVec3 axis;
  };
  SGPropertyNode_ptr _prop;
  TexTransform* _transform;
  int _num_transforms;
};


/**
 * An animation to enable the alpha test 
 */
class SGAlphaTestAnimation : public SGAnimation
{
public:
  SGAlphaTestAnimation (SGPropertyNode *prop_root,
                        SGPropertyNode_ptr props);
  virtual ~SGAlphaTestAnimation ();
  virtual void update();
private:
  void setAlphaClampToBranch(ssgBranch *b, float clamp);
  bool _done;
  float _alpha_clamp;
};


#endif // _SG_ANIMATION_HXX
