// animation.cxx - classes to manage model animation.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.


#include <string.h>             // for strcmp()
#include <math.h>

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
// Implementation of SGAnimation
////////////////////////////////////////////////////////////////////////

// Initialize the static data member
double SGAnimation::sim_time_sec = 0.0;

SGAnimation::SGAnimation (SGPropertyNode_ptr props, ssgBranch * branch)
    : _branch(branch)
{
    _branch->setName(props->getStringValue("name", 0));
}

SGAnimation::~SGAnimation ()
{
}

void
SGAnimation::init ()
{
}

void
SGAnimation::update()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGNullAnimation
////////////////////////////////////////////////////////////////////////

SGNullAnimation::SGNullAnimation (SGPropertyNode_ptr props)
  : SGAnimation(props, new ssgBranch)
{
}

SGNullAnimation::~SGNullAnimation ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGRangeAnimation
////////////////////////////////////////////////////////////////////////

SGRangeAnimation::SGRangeAnimation (SGPropertyNode_ptr props)
  : SGAnimation(props, new ssgRangeSelector)
{
    float ranges[] = { props->getFloatValue("min-m", 0),
                       props->getFloatValue("max-m", 5000) };
    ((ssgRangeSelector *)_branch)->setRanges(ranges, 2);
                       
}

SGRangeAnimation::~SGRangeAnimation ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGBillboardAnimation
////////////////////////////////////////////////////////////////////////

SGBillboardAnimation::SGBillboardAnimation (SGPropertyNode_ptr props)
    : SGAnimation(props, new ssgCutout(props->getBoolValue("spherical", true)))
{
}

SGBillboardAnimation::~SGBillboardAnimation ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGSelectAnimation
////////////////////////////////////////////////////////////////////////

SGSelectAnimation::SGSelectAnimation( SGPropertyNode *prop_root,
                                  SGPropertyNode_ptr props )
  : SGAnimation(props, new ssgSelector),
    _condition(0)
{
  SGPropertyNode_ptr node = props->getChild("condition");
  if (node != 0)
    _condition = sgReadCondition(prop_root, node);
}

SGSelectAnimation::~SGSelectAnimation ()
{
  delete _condition;
}

void
SGSelectAnimation::update()
{
  if (_condition != 0 && _condition->test()) 
      ((ssgSelector *)_branch)->select(0xffff);
  else
      ((ssgSelector *)_branch)->select(0x0000);
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGSpinAnimation
////////////////////////////////////////////////////////////////////////

SGSpinAnimation::SGSpinAnimation( SGPropertyNode *prop_root,
                              SGPropertyNode_ptr props,
                              double sim_time_sec )
  : SGAnimation(props, new ssgTransform),
    _prop((SGPropertyNode *)prop_root->getNode(props->getStringValue("property", "/null"), true)),
    _factor(props->getDoubleValue("factor", 1.0)),
    _position_deg(props->getDoubleValue("starting-position-deg", 0)),
    _last_time_sec( sim_time_sec )
{
    _center[0] = 0;
    _center[1] = 0;
    _center[2] = 0;
    if (props->hasValue("axis/x1-m")) {
        double x1,y1,z1,x2,y2,z2;
        x1 = props->getFloatValue("axis/x1-m");
        y1 = props->getFloatValue("axis/y1-m");
        z1 = props->getFloatValue("axis/z1-m");
        x2 = props->getFloatValue("axis/x2-m");
        y2 = props->getFloatValue("axis/y2-m");
        z2 = props->getFloatValue("axis/z2-m");
        _center[0] = (x1+x2)/2;
        _center[1]= (y1+y2)/2;
        _center[2] = (z1+z2)/2;
        float vector_length = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) + (z2-z1)*(z2-z1));
        _axis[0] = (x2-x1)/vector_length;
        _axis[1] = (y2-y1)/vector_length;
        _axis[2] = (z2-z1)/vector_length;
    } else {
       _axis[0] = props->getFloatValue("axis/x", 0);
       _axis[1] = props->getFloatValue("axis/y", 0);
       _axis[2] = props->getFloatValue("axis/z", 0);
    }
    if (props->hasValue("center/x-m")) {
       _center[0] = props->getFloatValue("center/x-m", 0);
       _center[1] = props->getFloatValue("center/y-m", 0);
       _center[2] = props->getFloatValue("center/z-m", 0);
    }
    sgNormalizeVec3(_axis);
}

SGSpinAnimation::~SGSpinAnimation ()
{
}

void
SGSpinAnimation::update()
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
// Implementation of SGTimedAnimation
////////////////////////////////////////////////////////////////////////

SGTimedAnimation::SGTimedAnimation (SGPropertyNode_ptr props)
  : SGAnimation(props, new ssgSelector),
    _duration_sec(props->getDoubleValue("duration-sec", 1.0)),
    _last_time_sec(0),
    _step(-1)
{
}

SGTimedAnimation::~SGTimedAnimation ()
{
}

void
SGTimedAnimation::update()
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
// Implementation of SGRotateAnimation
////////////////////////////////////////////////////////////////////////

SGRotateAnimation::SGRotateAnimation( SGPropertyNode *prop_root,
                                  SGPropertyNode_ptr props )
    : SGAnimation(props, new ssgTransform),
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
    _center[0] = 0;
    _center[1] = 0;
    _center[2] = 0;
    if (props->hasValue("axis/x1-m")) {
        double x1,y1,z1,x2,y2,z2;
        x1 = props->getFloatValue("axis/x1-m");
        y1 = props->getFloatValue("axis/y1-m");
        z1 = props->getFloatValue("axis/z1-m");
        x2 = props->getFloatValue("axis/x2-m");
        y2 = props->getFloatValue("axis/y2-m");
        z2 = props->getFloatValue("axis/z2-m");
        _center[0] = (x1+x2)/2;
        _center[1]= (y1+y2)/2;
        _center[2] = (z1+z2)/2;
        float vector_length = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) + (z2-z1)*(z2-z1));
        _axis[0] = (x2-x1)/vector_length;
        _axis[1] = (y2-y1)/vector_length;
        _axis[2] = (z2-z1)/vector_length;
    } else {
       _axis[0] = props->getFloatValue("axis/x", 0);
       _axis[1] = props->getFloatValue("axis/y", 0);
       _axis[2] = props->getFloatValue("axis/z", 0);
    }
    if (props->hasValue("center/x-m")) {
       _center[0] = props->getFloatValue("center/x-m", 0);
       _center[1] = props->getFloatValue("center/y-m", 0);
       _center[2] = props->getFloatValue("center/z-m", 0);
    }
    sgNormalizeVec3(_axis);
}

SGRotateAnimation::~SGRotateAnimation ()
{
  delete _table;
}

void
SGRotateAnimation::update()
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
// Implementation of SGTranslateAnimation
////////////////////////////////////////////////////////////////////////

SGTranslateAnimation::SGTranslateAnimation( SGPropertyNode *prop_root,
                                        SGPropertyNode_ptr props )
  : SGAnimation(props, new ssgTransform),
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

SGTranslateAnimation::~SGTranslateAnimation ()
{
  delete _table;
}

void
SGTranslateAnimation::update()
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


////////////////////////////////////////////////////////////////////////
// Implementation of SGTexRotateAnimation
////////////////////////////////////////////////////////////////////////

SGTexRotateAnimation::SGTexRotateAnimation( SGPropertyNode *prop_root,
                                  SGPropertyNode_ptr props )
    : SGAnimation(props, new ssgTexTrans),
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

SGTexRotateAnimation::~SGTexRotateAnimation ()
{
  delete _table;
}

void
SGTexRotateAnimation::update()
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
  ((ssgTexTrans *)_branch)->setTransform(_matrix);
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGTexTranslateAnimation
////////////////////////////////////////////////////////////////////////

SGTexTranslateAnimation::SGTexTranslateAnimation( SGPropertyNode *prop_root,
                                        SGPropertyNode_ptr props )
  : SGAnimation(props, new ssgTexTrans),
      _prop((SGPropertyNode *)prop_root->getNode(props->getStringValue("property", "/null"), true)),
    _offset_m(props->getDoubleValue("offset-m", 0.0)),
    _factor(props->getDoubleValue("factor", 1.0)),
    _step(props->getDoubleValue("step",0.0)),
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

SGTexTranslateAnimation::~SGTexTranslateAnimation ()
{
  delete _table;
}

void
SGTexTranslateAnimation::update()
{
  if (_table == 0) {
    if(_step > 0) {
      // apply stepping of input value
      if(_prop->getDoubleValue() > 0) 
       _position_m = ((floor(_prop->getDoubleValue()/_step) * _step) + _offset_m) * _factor;
     else
      _position_m = ((ceil(_prop->getDoubleValue()/_step) * _step) + _offset_m) * _factor;
    } else {
       _position_m = (_prop->getDoubleValue() + _offset_m) * _factor;
    }
    if (_has_min && _position_m < _min_m)
      _position_m = _min_m;
    if (_has_max && _position_m > _max_m)
      _position_m = _max_m;
  } else {
    _position_m = _table->interpolate(_prop->getDoubleValue());
  }
  set_translation(_matrix, _position_m, _axis);
  ((ssgTexTrans *)_branch)->setTransform(_matrix);
}


// end of animation.cxx
