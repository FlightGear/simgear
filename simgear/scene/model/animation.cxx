// animation.hxx - classes to manage model animation.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.


#include <string.h>             // for strcmp()

#include <plib/sg.h>
#include <plib/ssg.h>
#include <plib/ul.h>

#include <simgear/math/interpolater.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>

#include "animation.hxx"



////////////////////////////////////////////////////////////////////////
// Static utility functions.
////////////////////////////////////////////////////////////////////////

/**
 * Set up the transform matrix for a spin or rotation.
 */
static void
set_rotation (sgMat4 &matrix, double position_deg,
              sgVec3 &center, sgVec3 &axis)
{
 float temp_angle = -position_deg * SG_DEGREES_TO_RADIANS ;
 
 float s = (float) sin ( temp_angle ) ;
 float c = (float) cos ( temp_angle ) ;
 float t = SG_ONE - c ;

 // axis was normalized at load time 
 // hint to the compiler to put these into FP registers
 float x = axis[0];
 float y = axis[1];
 float z = axis[2];

 matrix[0][0] = t * x * x + c ;
 matrix[0][1] = t * y * x - s * z ;
 matrix[0][2] = t * z * x + s * y ;
 matrix[0][3] = SG_ZERO;
 
 matrix[1][0] = t * x * y + s * z ;
 matrix[1][1] = t * y * y + c ;
 matrix[1][2] = t * z * y - s * x ;
 matrix[1][3] = SG_ZERO;
 
 matrix[2][0] = t * x * z - s * y ;
 matrix[2][1] = t * y * z + s * x ;
 matrix[2][2] = t * z * z + c ;
 matrix[2][3] = SG_ZERO;

  // hint to the compiler to put these into FP registers
 x = center[0];
 y = center[1];
 z = center[2];
 
 matrix[3][0] = x - x*matrix[0][0] - y*matrix[1][0] - z*matrix[2][0];
 matrix[3][1] = y - x*matrix[0][1] - y*matrix[1][1] - z*matrix[2][1];
 matrix[3][2] = z - x*matrix[0][2] - y*matrix[1][2] - z*matrix[2][2];
 matrix[3][3] = SG_ONE;
}

/**
 * Set up the transform matrix for a translation.
 */
static void
set_translation (sgMat4 &matrix, double position_m, sgVec3 &axis)
{
  sgVec3 xyz;
  sgScaleVec3(xyz, axis, position_m);
  sgMakeTransMat4(matrix, xyz);
}


/**
 * Read an interpolation table from properties.
 */
static SGInterpTable *
read_interpolation_table (SGPropertyNode_ptr props)
{
  SGPropertyNode_ptr table_node = props->getNode("interpolation");
  if (table_node != 0) {
    SGInterpTable * table = new SGInterpTable();
    vector<SGPropertyNode_ptr> entries = table_node->getChildren("entry");
    for (unsigned int i = 0; i < entries.size(); i++)
      table->addEntry(entries[i]->getDoubleValue("ind", 0.0),
                      entries[i]->getDoubleValue("dep", 0.0));
    return table;
  } else {
    return 0;
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of Animation
////////////////////////////////////////////////////////////////////////

// Initialize the static data member
double Animation::sim_time_sec = 0.0;

Animation::Animation (SGPropertyNode_ptr props, ssgBranch * branch)
    : _branch(branch)
{
    _branch->setName(props->getStringValue("name", 0));
}

Animation::~Animation ()
{
}

void
Animation::init ()
{
}

void
Animation::update()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of NullAnimation
////////////////////////////////////////////////////////////////////////

NullAnimation::NullAnimation (SGPropertyNode_ptr props)
  : Animation(props, new ssgBranch)
{
}

NullAnimation::~NullAnimation ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of RangeAnimation
////////////////////////////////////////////////////////////////////////

RangeAnimation::RangeAnimation (SGPropertyNode_ptr props)
  : Animation(props, new ssgRangeSelector)
{
    float ranges[] = { props->getFloatValue("min-m", 0),
                       props->getFloatValue("max-m", 5000) };
    ((ssgRangeSelector *)_branch)->setRanges(ranges, 2);
                       
}

RangeAnimation::~RangeAnimation ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of BillboardAnimation
////////////////////////////////////////////////////////////////////////

BillboardAnimation::BillboardAnimation (SGPropertyNode_ptr props)
    : Animation(props, new ssgCutout(props->getBoolValue("spherical", true)))
{
}

BillboardAnimation::~BillboardAnimation ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of SelectAnimation
////////////////////////////////////////////////////////////////////////

SelectAnimation::SelectAnimation( SGPropertyNode *prop_root,
                                  SGPropertyNode_ptr props )
  : Animation(props, new ssgSelector),
    _condition(0)
{
  SGPropertyNode_ptr node = props->getChild("condition");
  if (node != 0)
    _condition = fgReadCondition(prop_root, node);
}

SelectAnimation::~SelectAnimation ()
{
  delete _condition;
}

void
SelectAnimation::update()
{
  if (_condition != 0 && _condition->test()) 
      ((ssgSelector *)_branch)->select(0xffff);
  else
      ((ssgSelector *)_branch)->select(0x0000);
}



////////////////////////////////////////////////////////////////////////
// Implementation of SpinAnimation
////////////////////////////////////////////////////////////////////////

SpinAnimation::SpinAnimation( SGPropertyNode *prop_root,
                              SGPropertyNode_ptr props,
                              double sim_time_sec )
  : Animation(props, new ssgTransform),
    _prop((SGPropertyNode *)prop_root->getNode(props->getStringValue("property", "/null"), true)),
    _factor(props->getDoubleValue("factor", 1.0)),
    _position_deg(props->getDoubleValue("starting-position-deg", 0)),
    _last_time_sec( sim_time_sec )
{
    _center[0] = props->getFloatValue("center/x-m", 0);
    _center[1] = props->getFloatValue("center/y-m", 0);
    _center[2] = props->getFloatValue("center/z-m", 0);
    _axis[0] = props->getFloatValue("axis/x", 0);
    _axis[1] = props->getFloatValue("axis/y", 0);
    _axis[2] = props->getFloatValue("axis/z", 0);
    sgNormalizeVec3(_axis);
}

SpinAnimation::~SpinAnimation ()
{
}

void
SpinAnimation::update()
{
  double dt = sim_time_sec - _last_time_sec;
  _last_time_sec = sim_time_sec;

  float velocity_rpms = (_prop->getDoubleValue() * _factor / 60.0);
  _position_deg += (dt * velocity_rpms * 360);
  while (_position_deg < 0)
    _position_deg += 360.0;
  while (_position_deg >= 360.0)
    _position_deg -= 360.0;
  set_rotation(_matrix, _position_deg, _center, _axis);
  ((ssgTransform *)_branch)->setTransform(_matrix);
}



////////////////////////////////////////////////////////////////////////
// Implementation of TimedAnimation
////////////////////////////////////////////////////////////////////////

TimedAnimation::TimedAnimation (SGPropertyNode_ptr props)
  : Animation(props, new ssgSelector),
    _duration_sec(props->getDoubleValue("duration-sec", 1.0)),
    _last_time_sec(0),
    _step(-1)
{
}

TimedAnimation::~TimedAnimation ()
{
}

void
TimedAnimation::update()
{
    if ((sim_time_sec - _last_time_sec) >= _duration_sec) {
        _last_time_sec = sim_time_sec;
        _step++;
        if (_step >= getBranch()->getNumKids())
            _step = 0;
        ((ssgSelector *)getBranch())->selectStep(_step);
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of RotateAnimation
////////////////////////////////////////////////////////////////////////

RotateAnimation::RotateAnimation( SGPropertyNode *prop_root,
                                  SGPropertyNode_ptr props )
    : Animation(props, new ssgTransform),
      _prop((SGPropertyNode *)prop_root->getNode(props->getStringValue("property", "/null"), true)),
      _offset_deg(props->getDoubleValue("offset-deg", 0.0)),
      _factor(props->getDoubleValue("factor", 1.0)),
      _table(read_interpolation_table(props)),
      _has_min(props->hasValue("min-deg")),
      _min_deg(props->getDoubleValue("min-deg")),
      _has_max(props->hasValue("max-deg")),
      _max_deg(props->getDoubleValue("max-deg")),
      _position_deg(props->getDoubleValue("starting-position-deg", 0))
{
  _center[0] = props->getFloatValue("center/x-m", 0);
  _center[1] = props->getFloatValue("center/y-m", 0);
  _center[2] = props->getFloatValue("center/z-m", 0);
  _axis[0] = props->getFloatValue("axis/x", 0);
  _axis[1] = props->getFloatValue("axis/y", 0);
  _axis[2] = props->getFloatValue("axis/z", 0);
  sgNormalizeVec3(_axis);
}

RotateAnimation::~RotateAnimation ()
{
  delete _table;
}

void
RotateAnimation::update()
{
  if (_table == 0) {
   _position_deg = _prop->getDoubleValue() * _factor + _offset_deg;
   if (_has_min && _position_deg < _min_deg)
     _position_deg = _min_deg;
   if (_has_max && _position_deg > _max_deg)
     _position_deg = _max_deg;
  } else {
    _position_deg = _table->interpolate(_prop->getDoubleValue());
  }
  set_rotation(_matrix, _position_deg, _center, _axis);
  ((ssgTransform *)_branch)->setTransform(_matrix);
}



////////////////////////////////////////////////////////////////////////
// Implementation of TranslateAnimation
////////////////////////////////////////////////////////////////////////

TranslateAnimation::TranslateAnimation( SGPropertyNode *prop_root,
                                        SGPropertyNode_ptr props )
  : Animation(props, new ssgTransform),
      _prop((SGPropertyNode *)prop_root->getNode(props->getStringValue("property", "/null"), true)),
    _offset_m(props->getDoubleValue("offset-m", 0.0)),
    _factor(props->getDoubleValue("factor", 1.0)),
    _table(read_interpolation_table(props)),
    _has_min(props->hasValue("min-m")),
    _min_m(props->getDoubleValue("min-m")),
    _has_max(props->hasValue("max-m")),
    _max_m(props->getDoubleValue("max-m")),
    _position_m(props->getDoubleValue("starting-position-m", 0))
{
  _axis[0] = props->getFloatValue("axis/x", 0);
  _axis[1] = props->getFloatValue("axis/y", 0);
  _axis[2] = props->getFloatValue("axis/z", 0);
  sgNormalizeVec3(_axis);
}

TranslateAnimation::~TranslateAnimation ()
{
  delete _table;
}

void
TranslateAnimation::update()
{
  if (_table == 0) {
    _position_m = (_prop->getDoubleValue() + _offset_m) * _factor;
    if (_has_min && _position_m < _min_m)
      _position_m = _min_m;
    if (_has_max && _position_m > _max_m)
      _position_m = _max_m;
  } else {
    _position_m = _table->interpolate(_prop->getDoubleValue());
  }
  set_translation(_matrix, _position_m, _axis);
  ((ssgTransform *)_branch)->setTransform(_matrix);
}


// end of animation.cxx
