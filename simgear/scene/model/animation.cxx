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
#include <simgear/math/sg_random.h>

#include "animation.hxx"
#include "custtrans.hxx"
#include "personality.hxx"


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
 * Set up the transform matrix for a scale operation.
 */
static void
set_scale (sgMat4 &matrix, double x, double y, double z)
{
  sgMakeIdentMat4( matrix );
  matrix[0][0] = x;
  matrix[1][1] = y;
  matrix[2][2] = z;
}

/**
 * Recursively process all kids to change the alpha values
 */
static void
change_alpha( ssgBase *_branch, float _blend )
{
  int i;

  for (i = 0; i < ((ssgBranch *)_branch)->getNumKids(); i++)
    change_alpha( ((ssgBranch *)_branch)->getKid(i), _blend );

  if ( !_branch->isAKindOf(ssgTypeLeaf())
       && !_branch->isAKindOf(ssgTypeVtxTable())
       && !_branch->isAKindOf(ssgTypeVTable()) )
    return;

  int num_colors = ((ssgLeaf *)_branch)->getNumColours();
// unsigned int select_ = (_blend == 1.0) ? false : true;

  for (i = 0; i < num_colors; i++)
  {
//    ((ssgSelector *)_branch)->select( select_ );
    float *color =  ((ssgLeaf *)_branch)->getColour(i);
    color[3] = _blend;
  }
}

/**
 * Modify property value by step and scroll settings in texture translations
 */
static double
apply_mods(double property, double step, double scroll)
{

  double modprop;
  if(step > 0) {
    double scrollval = 0.0;
    if(scroll > 0) {
      // calculate scroll amount (for odometer like movement)
      double remainder  =  step - fmod(fabs(property), step);
      if (remainder < scroll) {
        scrollval = (scroll - remainder) / scroll * step;
      }
    }
  // apply stepping of input value
  if(property > 0) 
     modprop = ((floor(property/step) * step) + scrollval);
  else
     modprop = ((ceil(property/step) * step) + scrollval);
  } else {
     modprop = property;
  }
  return modprop;

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
SGPersonalityBranch *SGAnimation::current_object = 0;

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

int
SGAnimation::update()
{
    return 1;
}

void
SGAnimation::restore()
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

SGRangeAnimation::SGRangeAnimation (SGPropertyNode *prop_root,
                                    SGPropertyNode_ptr props)
  : SGAnimation(props, new ssgRangeSelector),
    _min(0.0), _max(0.0), _min_factor(1.0), _max_factor(1.0),
    _condition(0)
{
    SGPropertyNode_ptr node = props->getChild("condition");
    if (node != 0)
       _condition = sgReadCondition(prop_root, node);

    float ranges[2];

    node = props->getChild( "min-factor" );
    if (node != 0) {
       _min_factor = props->getFloatValue("min-factor", 1.0);
    }
    node = props->getChild( "max-factor" );
    if (node != 0) {
       _max_factor = props->getFloatValue("max-factor", 1.0);
    }
    node = props->getChild( "min-property" );
    if (node != 0) {
       _min_prop = (SGPropertyNode *)prop_root->getNode(node->getStringValue(), true);
       ranges[0] = _min_prop->getFloatValue() * _min_factor;
    } else {
       _min = props->getFloatValue("min-m", 0);
       ranges[0] = _min * _min_factor;
    }
    node = props->getChild( "max-property" );
    if (node != 0) {
       _max_prop = (SGPropertyNode *)prop_root->getNode(node->getStringValue(), true);
       ranges[1] = _max_prop->getFloatValue() * _max_factor;
    } else {
       _max = props->getFloatValue("max-m", 0);
       ranges[1] = _max * _max_factor;
    }
    ((ssgRangeSelector *)_branch)->setRanges(ranges, 2);
}

SGRangeAnimation::~SGRangeAnimation ()
{
}

int
SGRangeAnimation::update()
{
  float ranges[2];
  if ( _condition == 0 || _condition->test() ) {
    if (_min_prop != 0) {
      ranges[0] = _min_prop->getFloatValue() * _min_factor;
    } else {
      ranges[0] = _min * _min_factor;
    }
    if (_max_prop != 0) {
      ranges[1] = _max_prop->getFloatValue() * _max_factor;
    } else {
      ranges[1] = _max * _max_factor;
    }
  } else {
    ranges[0] = 0.f;
    ranges[1] = 1000000000.f;
  }
  ((ssgRangeSelector *)_branch)->setRanges(ranges, 2);
  return 1;
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

int
SGSelectAnimation::update()
{
  if (_condition != 0 && _condition->test()) 
      ((ssgSelector *)_branch)->select(0xffff);
  else
      ((ssgSelector *)_branch)->select(0x0000);
  return 1;
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
    _last_time_sec( sim_time_sec ),
    _condition(0)
{
    SGPropertyNode_ptr node = props->getChild("condition");
    if (node != 0)
        _condition = sgReadCondition(prop_root, node);

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

int
SGSpinAnimation::update()
{
  if ( _condition == 0 || _condition->test() ) {
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
  return 1;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGTimedAnimation
////////////////////////////////////////////////////////////////////////

SGTimedAnimation::SGTimedAnimation (SGPropertyNode_ptr props)
  : SGAnimation(props, new ssgSelector),
    _use_personality( props->getBoolValue("use-personality",false) ),
    _duration_sec(props->getDoubleValue("duration-sec", 1.0)),
    _last_time_sec( sim_time_sec ),
    _total_duration_sec( 0 ),
    _step( 0 )
    
{
    vector<SGPropertyNode_ptr> nodes = props->getChildren( "branch-duration-sec" );
    size_t nb = nodes.size();
    for ( size_t i = 0; i < nb; i++ ) {
        size_t ind = nodes[ i ]->getIndex();
        while ( ind >= _branch_duration_specs.size() ) {
            _branch_duration_specs.push_back( DurationSpec( _duration_sec ) );
        }
        SGPropertyNode_ptr rNode = nodes[ i ]->getChild("random");
        if ( rNode == 0 ) {
            _branch_duration_specs[ ind ] = DurationSpec( nodes[ i ]->getDoubleValue() );
        } else {
            _branch_duration_specs[ ind ] = DurationSpec( rNode->getDoubleValue( "min", 0.0 ),
                                                          rNode->getDoubleValue( "max", 1.0 ) );
        }
    }
}

SGTimedAnimation::~SGTimedAnimation ()
{
}

void
SGTimedAnimation::init()
{
    if ( !_use_personality ) {
        for ( size_t i = 0; i < getBranch()->getNumKids(); i++ ) {
            double v;
            if ( i < _branch_duration_specs.size() ) {
                DurationSpec &sp = _branch_duration_specs[ i ];
                v = sp._min + sg_random() * ( sp._max - sp._min );
            } else {
                v = _duration_sec;
            }
            _branch_duration_sec.push_back( v );
            _total_duration_sec += v;
        }
        // Sanity check : total duration shouldn't equal zero
        if ( _total_duration_sec < 0.01 ) {
            _total_duration_sec = 0.01;
        }
    }
    ((ssgSelector *)getBranch())->selectStep(_step);
}

int
SGTimedAnimation::update()
{
    if ( _use_personality ) {
        SGPersonalityBranch *key = current_object;
        if ( !key->getIntValue( this, INIT ) ) {
            double total = 0;
            double offset = 1.0;
            for ( size_t i = 0; i < _branch_duration_specs.size(); i++ ) {
                DurationSpec &sp = _branch_duration_specs[ i ];
                double v = sp._min + sg_random() * ( sp._max - sp._min );
                key->setDoubleValue( v, this, BRANCH_DURATION_SEC, i );
                if ( i == 0 )
                    offset = v;
                total += v;
            }
            // Sanity check : total duration shouldn't equal zero
            if ( total < 0.01 ) {
                total = 0.01;
            }
            offset *= sg_random();
            key->setDoubleValue( sim_time_sec - offset, this, LAST_TIME_SEC );
            key->setDoubleValue( total, this, TOTAL_DURATION_SEC );
            key->setIntValue( 0, this, STEP );
            key->setIntValue( 1, this, INIT );
        }

        _step = key->getIntValue( this, STEP );
        _last_time_sec = key->getDoubleValue( this, LAST_TIME_SEC );
        _total_duration_sec = key->getDoubleValue( this, TOTAL_DURATION_SEC );
        while ( ( sim_time_sec - _last_time_sec ) >= _total_duration_sec ) {
            _last_time_sec += _total_duration_sec;
        }
        double duration = _duration_sec;
        if ( _step < (int)_branch_duration_specs.size() ) {
            duration = key->getDoubleValue( this, BRANCH_DURATION_SEC, _step );
        }
        if ( ( sim_time_sec - _last_time_sec ) >= duration ) {
            _last_time_sec += duration;
            _step += 1;
            if ( _step >= getBranch()->getNumKids() )
                _step = 0;
        }
        ((ssgSelector *)getBranch())->selectStep( _step );
        key->setDoubleValue( _last_time_sec, this, LAST_TIME_SEC );
        key->setIntValue( _step, this, STEP );
    } else {
        while ( ( sim_time_sec - _last_time_sec ) >= _total_duration_sec ) {
            _last_time_sec += _total_duration_sec;
        }
        double duration = _duration_sec;
        if ( _step < (int)_branch_duration_sec.size() ) {
            duration = _branch_duration_sec[ _step ];
        }
        if ( ( sim_time_sec - _last_time_sec ) >= duration ) {
            _last_time_sec += duration;
            _step += 1;
            if ( _step >= getBranch()->getNumKids() )
                _step = 0;
            ((ssgSelector *)getBranch())->selectStep( _step );
        }
    }
    return 1;
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
      _position_deg(props->getDoubleValue("starting-position-deg", 0)),
      _condition(0)
{
    SGPropertyNode_ptr node = props->getChild("condition");
    if (node != 0)
      _condition = sgReadCondition(prop_root, node);

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

int
SGRotateAnimation::update()
{
  if (_condition == 0 || _condition->test()) {
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
  return 1;
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGBlendAnimation
////////////////////////////////////////////////////////////////////////

SGBlendAnimation::SGBlendAnimation( SGPropertyNode *prop_root,
                                        SGPropertyNode_ptr props )
  : SGAnimation(props, new ssgTransform),
    _prop((SGPropertyNode *)prop_root->getNode(props->getStringValue("property", "/null"), true)),
    _table(read_interpolation_table(props)),
    _prev_value(1.0),
    _offset(props->getDoubleValue("offset", 0.0)),
    _factor(props->getDoubleValue("factor", 1.0)),
    _has_min(props->hasValue("min")),
    _min(props->getDoubleValue("min", 0.0)),
    _has_max(props->hasValue("max")),
    _max(props->getDoubleValue("max", 1.0))
{
}

SGBlendAnimation::~SGBlendAnimation ()
{
    delete _table;
}

int
SGBlendAnimation::update()
{
  double _blend;

  if (_table == 0) {
    _blend = 1.0 - (_prop->getDoubleValue() * _factor + _offset);

    if (_has_min && (_blend < _min))
      _blend = _min;
    if (_has_max && (_blend > _max))
      _blend = _max;
  } else {
    _blend = _table->interpolate(_prop->getDoubleValue());
  }

  if (_blend != _prev_value) {
    _prev_value = _blend;
    change_alpha( _branch, _blend );
  }
  return 1;
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
    _position_m(props->getDoubleValue("starting-position-m", 0)),
    _condition(0)
{
  SGPropertyNode_ptr node = props->getChild("condition");
  if (node != 0)
    _condition = sgReadCondition(prop_root, node);

  _axis[0] = props->getFloatValue("axis/x", 0);
  _axis[1] = props->getFloatValue("axis/y", 0);
  _axis[2] = props->getFloatValue("axis/z", 0);
  sgNormalizeVec3(_axis);
}

SGTranslateAnimation::~SGTranslateAnimation ()
{
  delete _table;
}

int
SGTranslateAnimation::update()
{
  if (_condition == 0 || _condition->test()) {
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
  return 1;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGScaleAnimation
////////////////////////////////////////////////////////////////////////

SGScaleAnimation::SGScaleAnimation( SGPropertyNode *prop_root,
                                        SGPropertyNode_ptr props )
  : SGAnimation(props, new ssgTransform),
      _prop((SGPropertyNode *)prop_root->getNode(props->getStringValue("property", "/null"), true)),
    _x_factor(props->getDoubleValue("x-factor", 1.0)),
    _y_factor(props->getDoubleValue("y-factor", 1.0)),
    _z_factor(props->getDoubleValue("z-factor", 1.0)),
    _x_offset(props->getDoubleValue("x-offset", 1.0)),
    _y_offset(props->getDoubleValue("y-offset", 1.0)),
    _z_offset(props->getDoubleValue("z-offset", 1.0)),
    _table(read_interpolation_table(props)),
    _has_min_x(props->hasValue("x-min")),
    _has_min_y(props->hasValue("y-min")),
    _has_min_z(props->hasValue("z-min")),
    _min_x(props->getDoubleValue("x-min")),
    _min_y(props->getDoubleValue("y-min")),
    _min_z(props->getDoubleValue("z-min")),
    _has_max_x(props->hasValue("x-max")),
    _has_max_y(props->hasValue("y-max")),
    _has_max_z(props->hasValue("z-max")),
    _max_x(props->getDoubleValue("x-max")),
    _max_y(props->getDoubleValue("y-max")),
    _max_z(props->getDoubleValue("z-max"))
{
}

SGScaleAnimation::~SGScaleAnimation ()
{
  delete _table;
}

int
SGScaleAnimation::update()
{
  if (_table == 0) {
      _x_scale = _prop->getDoubleValue() * _x_factor + _x_offset;
    if (_has_min_x && _x_scale < _min_x)
      _x_scale = _min_x;
    if (_has_max_x && _x_scale > _max_x)
      _x_scale = _max_x;
  } else {
    _x_scale = _table->interpolate(_prop->getDoubleValue());
  }

  if (_table == 0) {
    _y_scale = _prop->getDoubleValue() * _y_factor + _y_offset;
    if (_has_min_y && _y_scale < _min_y)
      _y_scale = _min_y;
    if (_has_max_y && _y_scale > _max_y)
      _y_scale = _max_y;
  } else {
    _y_scale = _table->interpolate(_prop->getDoubleValue());
  }

  if (_table == 0) {
    _z_scale = _prop->getDoubleValue() * _z_factor + _z_offset;
    if (_has_min_z && _z_scale < _min_z)
      _z_scale = _min_z;
    if (_has_max_z && _z_scale > _max_z)
      _z_scale = _max_z;
  } else {
    _z_scale = _table->interpolate(_prop->getDoubleValue());
  }

  set_scale(_matrix, _x_scale, _y_scale, _z_scale );
  ((ssgTransform *)_branch)->setTransform(_matrix);
  return 1;
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
  _center[0] = props->getFloatValue("center/x", 0);
  _center[1] = props->getFloatValue("center/y", 0);
  _center[2] = props->getFloatValue("center/z", 0);
  _axis[0] = props->getFloatValue("axis/x", 0);
  _axis[1] = props->getFloatValue("axis/y", 0);
  _axis[2] = props->getFloatValue("axis/z", 0);
  sgNormalizeVec3(_axis);
}

SGTexRotateAnimation::~SGTexRotateAnimation ()
{
  delete _table;
}

int
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
  return 1;
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGTexTranslateAnimation
////////////////////////////////////////////////////////////////////////

SGTexTranslateAnimation::SGTexTranslateAnimation( SGPropertyNode *prop_root,
                                        SGPropertyNode_ptr props )
  : SGAnimation(props, new ssgTexTrans),
      _prop((SGPropertyNode *)prop_root->getNode(props->getStringValue("property", "/null"), true)),
    _offset(props->getDoubleValue("offset", 0.0)),
    _factor(props->getDoubleValue("factor", 1.0)),
    _step(props->getDoubleValue("step",0.0)),
    _scroll(props->getDoubleValue("scroll",0.0)),
    _table(read_interpolation_table(props)),
    _has_min(props->hasValue("min")),
    _min(props->getDoubleValue("min")),
    _has_max(props->hasValue("max")),
    _max(props->getDoubleValue("max")),
    _position(props->getDoubleValue("starting-position", 0))
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

int
SGTexTranslateAnimation::update()
{
  if (_table == 0) {
    _position = (apply_mods(_prop->getDoubleValue(), _step, _scroll) + _offset) * _factor;
    if (_has_min && _position < _min)
      _position = _min;
    if (_has_max && _position > _max)
      _position = _max;
  } else {
    _position = _table->interpolate(apply_mods(_prop->getDoubleValue(), _step, _scroll));
  }
  set_translation(_matrix, _position, _axis);
  ((ssgTexTrans *)_branch)->setTransform(_matrix);
  return 1;
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGTexMultipleAnimation
////////////////////////////////////////////////////////////////////////

SGTexMultipleAnimation::SGTexMultipleAnimation( SGPropertyNode *prop_root,
                                        SGPropertyNode_ptr props )
  : SGAnimation(props, new ssgTexTrans),
      _prop((SGPropertyNode *)prop_root->getNode(props->getStringValue("property", "/null"), true))
{
  unsigned int i;
  // Load animations
  vector<SGPropertyNode_ptr> transform_nodes = props->getChildren("transform");
  _transform = new TexTransform [transform_nodes.size()];
  _num_transforms = 0;
  for (i = 0; i < transform_nodes.size(); i++) {
    SGPropertyNode_ptr transform_props = transform_nodes[i];

    if (!strcmp("textranslate",transform_props->getStringValue("subtype", 0))) {

      // transform is a translation
      _transform[i].subtype = 0;

      _transform[i].prop = (SGPropertyNode *)prop_root->getNode(transform_props->getStringValue("property", "/null"), true);

      _transform[i].offset = transform_props->getDoubleValue("offset", 0.0);
      _transform[i].factor = transform_props->getDoubleValue("factor", 1.0);
      _transform[i].step = transform_props->getDoubleValue("step",0.0);
      _transform[i].scroll = transform_props->getDoubleValue("scroll",0.0);
      _transform[i].table = read_interpolation_table(transform_props);
      _transform[i].has_min = transform_props->hasValue("min");
      _transform[i].min = transform_props->getDoubleValue("min");
      _transform[i].has_max = transform_props->hasValue("max");
      _transform[i].max = transform_props->getDoubleValue("max");
      _transform[i].position = transform_props->getDoubleValue("starting-position", 0);

      _transform[i].axis[0] = transform_props->getFloatValue("axis/x", 0);
      _transform[i].axis[1] = transform_props->getFloatValue("axis/y", 0);
      _transform[i].axis[2] = transform_props->getFloatValue("axis/z", 0);
      sgNormalizeVec3(_transform[i].axis);
      _num_transforms++;
    } else if (!strcmp("texrotate",transform_nodes[i]->getStringValue("subtype", 0))) {

      // transform is a rotation
      _transform[i].subtype = 1;

      _transform[i].prop = (SGPropertyNode *)prop_root->getNode(transform_props->getStringValue("property", "/null"), true);
      _transform[i].offset = transform_props->getDoubleValue("offset-deg", 0.0);
      _transform[i].factor = transform_props->getDoubleValue("factor", 1.0);
      _transform[i].table = read_interpolation_table(transform_props);
      _transform[i].has_min = transform_props->hasValue("min-deg");
      _transform[i].min = transform_props->getDoubleValue("min-deg");
      _transform[i].has_max = transform_props->hasValue("max-deg");
      _transform[i].max = transform_props->getDoubleValue("max-deg");
      _transform[i].position = transform_props->getDoubleValue("starting-position-deg", 0);

      _transform[i].center[0] = transform_props->getFloatValue("center/x", 0);
      _transform[i].center[1] = transform_props->getFloatValue("center/y", 0);
      _transform[i].center[2] = transform_props->getFloatValue("center/z", 0);
      _transform[i].axis[0] = transform_props->getFloatValue("axis/x", 0);
      _transform[i].axis[1] = transform_props->getFloatValue("axis/y", 0);
      _transform[i].axis[2] = transform_props->getFloatValue("axis/z", 0);
      sgNormalizeVec3(_transform[i].axis);
      _num_transforms++;
    }
  }
}

SGTexMultipleAnimation::~SGTexMultipleAnimation ()
{
  // delete _table;
  delete _transform;
}

int
SGTexMultipleAnimation::update()
{
  int i;
  sgMat4 tmatrix;
  sgMakeIdentMat4(tmatrix);
  for (i = 0; i < _num_transforms; i++) {

    if(_transform[i].subtype == 0) {

      // subtype 0 is translation
      if (_transform[i].table == 0) {
        _transform[i].position = (apply_mods(_transform[i].prop->getDoubleValue(), _transform[i].step,_transform[i].scroll) + _transform[i].offset) * _transform[i].factor;
        if (_transform[i].has_min && _transform[i].position < _transform[i].min)
          _transform[i].position = _transform[i].min;
        if (_transform[i].has_max && _transform[i].position > _transform[i].max)
          _transform[i].position = _transform[i].max;
      } else {
         _transform[i].position = _transform[i].table->interpolate(apply_mods(_transform[i].prop->getDoubleValue(), _transform[i].step,_transform[i].scroll));
      }
      set_translation(_transform[i].matrix, _transform[i].position, _transform[i].axis);
      sgPreMultMat4(tmatrix, _transform[i].matrix);

    } else if (_transform[i].subtype == 1) {

      // subtype 1 is rotation

      if (_transform[i].table == 0) {
        _transform[i].position = _transform[i].prop->getDoubleValue() * _transform[i].factor + _transform[i].offset;
        if (_transform[i].has_min && _transform[i].position < _transform[i].min)
         _transform[i].position = _transform[i].min;
       if (_transform[i].has_max && _transform[i].position > _transform[i].max)
         _transform[i].position = _transform[i].max;
     } else {
        _transform[i].position = _transform[i].table->interpolate(_transform[i].prop->getDoubleValue());
      }
      set_rotation(_transform[i].matrix, _transform[i].position, _transform[i].center, _transform[i].axis);
      sgPreMultMat4(tmatrix, _transform[i].matrix);
    }
  }
  ((ssgTexTrans *)_branch)->setTransform(tmatrix);
  return 1;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGAlphaTestAnimation
////////////////////////////////////////////////////////////////////////

SGAlphaTestAnimation::SGAlphaTestAnimation(SGPropertyNode_ptr props)
  : SGAnimation(props, new ssgBranch)
{
  _alpha_clamp = props->getFloatValue("alpha-factor", 0.0);
}

SGAlphaTestAnimation::~SGAlphaTestAnimation ()
{
}

void SGAlphaTestAnimation::init()
{
  setAlphaClampToBranch(_branch,_alpha_clamp);
}

void SGAlphaTestAnimation::setAlphaClampToBranch(ssgBranch *b, float clamp)
{
  int nb = b->getNumKids();
  for (int i = 0; i<nb; i++) {
    ssgEntity *e = b->getKid(i);
    if (e->isAKindOf(ssgTypeLeaf())) {
      ssgSimpleState*s = (ssgSimpleState*)((ssgLeaf*)e)->getState();
      s->enable( GL_ALPHA_TEST );
      s->setAlphaClamp( clamp );
    } else if (e->isAKindOf(ssgTypeBranch())) {
      setAlphaClampToBranch( (ssgBranch*)e, clamp );
    }
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGFlashAnimation
////////////////////////////////////////////////////////////////////////
SGFlashAnimation::SGFlashAnimation(SGPropertyNode_ptr props)
  : SGAnimation( props, new SGCustomTransform )
{
  _axis[0] = props->getFloatValue("axis/x", 0);
  _axis[1] = props->getFloatValue("axis/y", 0);
  _axis[2] = props->getFloatValue("axis/z", 1);

  _center[0] = props->getFloatValue("center/x-m", 0);
  _center[1] = props->getFloatValue("center/y-m", 0);
  _center[2] = props->getFloatValue("center/z-m", 0);

  _offset = props->getFloatValue("offset", 0.0);
  _factor = props->getFloatValue("factor", 1.0);
  _power = props->getFloatValue("power", 1.0);
  _two_sides = props->getBoolValue("two-sides", false);

  _min_v = props->getFloatValue("min", 0.0);
  _max_v = props->getFloatValue("max", 1.0);

  ((SGCustomTransform *)_branch)->setTransCallback( &SGFlashAnimation::flashCallback, this );
}

SGFlashAnimation::~SGFlashAnimation()
{
}

void SGFlashAnimation::flashCallback( sgMat4 r, sgFrustum *f, sgMat4 m, void *d )
{
  ((SGFlashAnimation *)d)->flashCallback( r, f, m );
}

void SGFlashAnimation::flashCallback( sgMat4 r, sgFrustum *f, sgMat4 m )
{
  sgVec3 transformed_axis;
  sgXformVec3( transformed_axis, _axis, m );
  sgNormalizeVec3( transformed_axis );

  sgVec3 view;
  sgFullXformPnt3( view, _center, m );
  sgNormalizeVec3( view );

  float cos_angle = -sgScalarProductVec3( transformed_axis, view );
  float scale_factor = 0.f;
  if ( _two_sides && cos_angle < 0 )
    scale_factor = _factor * (float)pow( -cos_angle, _power ) + _offset;
  else if ( cos_angle > 0 )
    scale_factor = _factor * (float)pow( cos_angle, _power ) + _offset;

  if ( scale_factor < _min_v )
      scale_factor = _min_v;
  if ( scale_factor > _max_v )
      scale_factor = _max_v;

  sgMat4 transform;
  sgMakeIdentMat4( transform );
  transform[0][0] = scale_factor;
  transform[1][1] = scale_factor;
  transform[2][2] = scale_factor;
  transform[3][0] = _center[0] * ( 1 - scale_factor );
  transform[3][1] = _center[1] * ( 1 - scale_factor );
  transform[3][2] = _center[2] * ( 1 - scale_factor );

  sgCopyMat4( r, m );
  sgPreMultMat4( r, transform );
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGDistScaleAnimation
////////////////////////////////////////////////////////////////////////
SGDistScaleAnimation::SGDistScaleAnimation(SGPropertyNode_ptr props)
  : SGAnimation( props, new SGCustomTransform ),
    _factor(props->getFloatValue("factor", 1.0)),
    _offset(props->getFloatValue("offset", 0.0)),
    _min_v(props->getFloatValue("min", 0.0)),
    _max_v(props->getFloatValue("max", 1.0)),
    _has_min(props->hasValue("min")),
    _has_max(props->hasValue("max")),
    _table(read_interpolation_table(props))
{
  _center[0] = props->getFloatValue("center/x-m", 0);
  _center[1] = props->getFloatValue("center/y-m", 0);
  _center[2] = props->getFloatValue("center/z-m", 0);

  ((SGCustomTransform *)_branch)->setTransCallback( &SGDistScaleAnimation::distScaleCallback, this );
}

SGDistScaleAnimation::~SGDistScaleAnimation()
{
}

void SGDistScaleAnimation::distScaleCallback( sgMat4 r, sgFrustum *f, sgMat4 m, void *d )
{
  ((SGDistScaleAnimation *)d)->distScaleCallback( r, f, m );
}

void SGDistScaleAnimation::distScaleCallback( sgMat4 r, sgFrustum *f, sgMat4 m )
{
  sgVec3 view;
  sgFullXformPnt3( view, _center, m );

  float scale_factor = sgLengthVec3( view );
  if (_table == 0) {
    scale_factor = _factor * scale_factor + _offset;
    if ( _has_min && scale_factor < _min_v )
      scale_factor = _min_v;
    if ( _has_max && scale_factor > _max_v )
      scale_factor = _max_v;
  } else {
    scale_factor = _table->interpolate( scale_factor );
  }

  sgMat4 transform;
  sgMakeIdentMat4( transform );
  transform[0][0] = scale_factor;
  transform[1][1] = scale_factor;
  transform[2][2] = scale_factor;
  transform[3][0] = _center[0] * ( 1 - scale_factor );
  transform[3][1] = _center[1] * ( 1 - scale_factor );
  transform[3][2] = _center[2] * ( 1 - scale_factor );

  sgCopyMat4( r, m );
  sgPreMultMat4( r, transform );
}

// end of animation.cxx
