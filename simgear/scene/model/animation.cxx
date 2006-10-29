// animation.cxx - classes to manage model animation.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <string.h>             // for strcmp()
#include <math.h>

#include <osg/AlphaFunc>
#include <osg/AutoTransform>
#include <osg/ColorMatrix>
#include <osg/Drawable>
#include <osg/Geode>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/StateSet>
#include <osg/Switch>
#include <osg/TexMat>

#include <simgear/math/interpolater.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include "animation.hxx"
#include "personality.hxx"
#include "model.hxx"


////////////////////////////////////////////////////////////////////////
// Static utility functions.
////////////////////////////////////////////////////////////////////////

/**
 * Set up the transform matrix for a spin or rotation.
 */
static void
set_rotation (osg::Matrix &matrix, double position_deg,
              const osg::Vec3 &center, const osg::Vec3 &axis)
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

 matrix(0, 0) = t * x * x + c ;
 matrix(0, 1) = t * y * x - s * z ;
 matrix(0, 2) = t * z * x + s * y ;
 matrix(0, 3) = SG_ZERO;
 
 matrix(1, 0) = t * x * y + s * z ;
 matrix(1, 1) = t * y * y + c ;
 matrix(1, 2) = t * z * y - s * x ;
 matrix(1, 3) = SG_ZERO;
 
 matrix(2, 0) = t * x * z - s * y ;
 matrix(2, 1) = t * y * z + s * x ;
 matrix(2, 2) = t * z * z + c ;
 matrix(2, 3) = SG_ZERO;

  // hint to the compiler to put these into FP registers
 x = center[0];
 y = center[1];
 z = center[2];
 
 matrix(3, 0) = x - x*matrix(0, 0) - y*matrix(1, 0) - z*matrix(2, 0);
 matrix(3, 1) = y - x*matrix(0, 1) - y*matrix(1, 1) - z*matrix(2, 1);
 matrix(3, 2) = z - x*matrix(0, 2) - y*matrix(1, 2) - z*matrix(2, 2);
 matrix(3, 3) = SG_ONE;
}

/**
 * Set up the transform matrix for a translation.
 */
static void
set_translation (osg::Matrix &matrix, double position_m, const osg::Vec3 &axis)
{
  osg::Vec3 xyz = axis * position_m;
  matrix.makeIdentity();
  matrix(3, 0) = xyz[0];
  matrix(3, 1) = xyz[1];
  matrix(3, 2) = xyz[2];
}

/**
 * Set up the transform matrix for a scale operation.
 */
static void
set_scale (osg::Matrix &matrix, double x, double y, double z)
{
  matrix.makeIdentity();
  matrix(0, 0) = x;
  matrix(1, 1) = y;
  matrix(2, 2) = z;
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

SGAnimation::SGAnimation (SGPropertyNode_ptr props, osg::Group * branch)
    : _branch(branch),
    animation_type(0)
{
    _branch->setName(props->getStringValue("name", "Animation"));
    if ( props->getBoolValue( "enable-hot", true ) ) {
        _branch->setNodeMask(SG_NODEMASK_TERRAIN_BIT|_branch->getNodeMask());
    } else {
        _branch->setNodeMask(~SG_NODEMASK_TERRAIN_BIT&_branch->getNodeMask());
    }
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
  : SGAnimation(props, new osg::Group)
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
  : SGAnimation(props, new osg::LOD),
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
    static_cast<osg::LOD*>(_branch)->setRange(0, ranges[0], ranges[1]);
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
  static_cast<osg::LOD*>(_branch)->setRange(0, ranges[0], ranges[1]);
  return 2;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGBillboardAnimation
////////////////////////////////////////////////////////////////////////

SGBillboardAnimation::SGBillboardAnimation (SGPropertyNode_ptr props)
  : SGAnimation(props, new osg::AutoTransform)
{
//OSGFIXME: verify
  bool spherical = props->getBoolValue("spherical", true);
  osg::AutoTransform* autoTrans = static_cast<osg::AutoTransform*>(_branch);
  if (spherical) {
    autoTrans->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
  } else {
    autoTrans->setAutoRotateMode(osg::AutoTransform::NO_ROTATION);
    autoTrans->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
  }
  autoTrans->setAutoScaleToScreen(false);
}

SGBillboardAnimation::~SGBillboardAnimation ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGSelectAnimation
////////////////////////////////////////////////////////////////////////

SGSelectAnimation::SGSelectAnimation( SGPropertyNode *prop_root,
                                  SGPropertyNode_ptr props )
  : SGAnimation(props, new osg::Switch),
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
SGSelectAnimation::operator()(osg::Node* node, osg::NodeVisitor* nv)
{ 
  if (_condition != 0 && _condition->test())
    static_cast<osg::Switch*>(_branch)->setAllChildrenOn();
  else
    static_cast<osg::Switch*>(_branch)->setAllChildrenOff();
  traverse(node, nv);
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGSpinAnimation
////////////////////////////////////////////////////////////////////////

SGSpinAnimation::SGSpinAnimation( SGPropertyNode *prop_root,
                              SGPropertyNode_ptr props,
                              double sim_time_sec )
  : SGAnimation(props, new osg::MatrixTransform),
    _use_personality( props->getBoolValue("use-personality",false) ),
    _prop((SGPropertyNode *)prop_root->getNode(props->getStringValue("property", "/null"), true)),
    _factor( props, "factor", 1.0 ),
    _position_deg( props, "starting-position-deg", 0.0 ),
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
    
    _axis.normalize();
}

SGSpinAnimation::~SGSpinAnimation ()
{
}

int
SGSpinAnimation::update()
{
  if ( _condition == 0 || _condition->test() ) {
    double dt;
    float velocity_rpms;
    if ( _use_personality && current_object ) {
      SGPersonalityBranch *key = current_object;
      if ( !key->getIntValue( this, INIT_SPIN ) ) {
        key->setDoubleValue( _factor.shuffle(), this, FACTOR_SPIN );
        key->setDoubleValue( _position_deg.shuffle(), this, POSITION_DEG_SPIN );

        key->setDoubleValue( sim_time_sec, this, LAST_TIME_SEC_SPIN );
        key->setIntValue( 1, this, INIT_SPIN );
      }

      _factor = key->getDoubleValue( this, FACTOR_SPIN );
      _position_deg = key->getDoubleValue( this, POSITION_DEG_SPIN );
      _last_time_sec = key->getDoubleValue( this, LAST_TIME_SEC_SPIN );
      dt = sim_time_sec - _last_time_sec;
      _last_time_sec = sim_time_sec;
      key->setDoubleValue( _last_time_sec, this, LAST_TIME_SEC_SPIN );

      velocity_rpms = (_prop->getDoubleValue() * _factor / 60.0);
      _position_deg += (dt * velocity_rpms * 360);
      _position_deg -= 360*floor(_position_deg/360);
      key->setDoubleValue( _position_deg, this, POSITION_DEG_SPIN );
    } else {
      dt = sim_time_sec - _last_time_sec;
      _last_time_sec = sim_time_sec;

      velocity_rpms = (_prop->getDoubleValue() * _factor / 60.0);
      _position_deg += (dt * velocity_rpms * 360);
      _position_deg -= 360*floor(_position_deg/360);
    }

    osg::Matrix _matrix;
    set_rotation(_matrix, _position_deg, _center, _axis);
    static_cast<osg::MatrixTransform*>(_branch)->setMatrix(_matrix);
  }
  return 1;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGTimedAnimation
////////////////////////////////////////////////////////////////////////

SGTimedAnimation::SGTimedAnimation (SGPropertyNode_ptr props)
  : SGAnimation(props, new osg::Switch),
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
        for ( unsigned i = 0; i < getBranch()->getNumChildren(); i++ ) {
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
    }
    // Sanity check : total duration shouldn't equal zero
    if (_duration_sec < 0.01)
        _duration_sec = 0.01;
    if ( _total_duration_sec < 0.01 )
        _total_duration_sec = 0.01;

    static_cast<osg::Switch*>(getBranch())->setSingleChildOn(_step);
}

int
SGTimedAnimation::update()
{
    if ( _use_personality && current_object ) {
        SGPersonalityBranch *key = current_object;
        if ( !key->getIntValue( this, INIT_TIMED ) ) {
            double total = 0;
            double offset = 1.0;
            for ( size_t i = 0; i < _branch_duration_specs.size(); i++ ) {
                DurationSpec &sp = _branch_duration_specs[ i ];
                double v = sp._min + sg_random() * ( sp._max - sp._min );
                key->setDoubleValue( v, this, BRANCH_DURATION_SEC_TIMED, i );
                if ( i == 0 )
                    offset = v;
                total += v;
            }
            // Sanity check : total duration shouldn't equal zero
            if ( total < 0.01 ) {
                total = 0.01;
            }
            offset *= sg_random();
            key->setDoubleValue( sim_time_sec - offset, this, LAST_TIME_SEC_TIMED );
            key->setDoubleValue( total, this, TOTAL_DURATION_SEC_TIMED );
            key->setIntValue( 0, this, STEP_TIMED );
            key->setIntValue( 1, this, INIT_TIMED );
        }

        _step = key->getIntValue( this, STEP_TIMED );
        _last_time_sec = key->getDoubleValue( this, LAST_TIME_SEC_TIMED );
        _total_duration_sec = key->getDoubleValue( this, TOTAL_DURATION_SEC_TIMED );
        _last_time_sec -= _total_duration_sec*floor((sim_time_sec - _last_time_sec)/_total_duration_sec);
        double duration = _duration_sec;
        if ( _step < _branch_duration_specs.size() ) {
            duration = key->getDoubleValue( this, BRANCH_DURATION_SEC_TIMED, _step );
        }
        if ( ( sim_time_sec - _last_time_sec ) >= duration ) {
            _last_time_sec += duration;
            _step += 1;
            if ( _step >= getBranch()->getNumChildren() )
                _step = 0;
        }
        static_cast<osg::Switch*>(getBranch())->setSingleChildOn(_step);
        key->setDoubleValue( _last_time_sec, this, LAST_TIME_SEC_TIMED );
        key->setIntValue( _step, this, STEP_TIMED );
    } else {
        _last_time_sec -= _total_duration_sec*floor((sim_time_sec - _last_time_sec)/_total_duration_sec);
        double duration = _duration_sec;
        if ( _step < _branch_duration_sec.size() ) {
            duration = _branch_duration_sec[ _step ];
        }
        if ( ( sim_time_sec - _last_time_sec ) >= duration ) {
            _last_time_sec += duration;
            _step += 1;
            if ( _step >= getBranch()->getNumChildren() )
                _step = 0;
            static_cast<osg::Switch*>(getBranch())->setSingleChildOn(_step);
        }
    }
    return 1;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGRotateAnimation
////////////////////////////////////////////////////////////////////////

SGRotateAnimation::SGRotateAnimation( SGPropertyNode *prop_root,
                                  SGPropertyNode_ptr props )
  : SGAnimation(props, new osg::MatrixTransform),
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
    if (props->hasValue("axis/x") || props->hasValue("axis/y") || props->hasValue("axis/z")) {
       _axis[0] = props->getFloatValue("axis/x", 0);
       _axis[1] = props->getFloatValue("axis/y", 0);
       _axis[2] = props->getFloatValue("axis/z", 0);
    } else {
        double x1,y1,z1,x2,y2,z2;
        x1 = props->getFloatValue("axis/x1-m", 0);
        y1 = props->getFloatValue("axis/y1-m", 0);
        z1 = props->getFloatValue("axis/z1-m", 0);
        x2 = props->getFloatValue("axis/x2-m", 0);
        y2 = props->getFloatValue("axis/y2-m", 0);
        z2 = props->getFloatValue("axis/z2-m", 0);
        _center[0] = (x1+x2)/2;
        _center[1]= (y1+y2)/2;
        _center[2] = (z1+z2)/2;
        float vector_length = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) + (z2-z1)*(z2-z1));
        _axis[0] = (x2-x1)/vector_length;
        _axis[1] = (y2-y1)/vector_length;
        _axis[2] = (z2-z1)/vector_length;
    }
    if (props->hasValue("center/x-m") || props->hasValue("center/y-m")
            || props->hasValue("center/z-m")) {
        _center[0] = props->getFloatValue("center/x-m", 0);
        _center[1] = props->getFloatValue("center/y-m", 0);
        _center[2] = props->getFloatValue("center/z-m", 0);
    }
    _axis.normalize();
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
    osg::Matrix _matrix;
    set_rotation(_matrix, _position_deg, _center, _axis);
    static_cast<osg::MatrixTransform*>(_branch)->setMatrix(_matrix);
  }
  return 2;
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGBlendAnimation
////////////////////////////////////////////////////////////////////////

SGBlendAnimation::SGBlendAnimation( SGPropertyNode *prop_root,
                                        SGPropertyNode_ptr props )
  : SGAnimation(props, new osg::Group),
    _use_personality( props->getBoolValue("use-personality",false) ),
    _prop((SGPropertyNode *)prop_root->getNode(props->getStringValue("property", "/null"), true)),
    _table(read_interpolation_table(props)),
    _prev_value(1.0),
    _offset(props,"offset",0.0),
    _factor(props,"factor",1.0),
    _has_min(props->hasValue("min")),
    _min(props->getDoubleValue("min", 0.0)),
    _has_max(props->hasValue("max")),
    _max(props->getDoubleValue("max", 1.0))
{
  // OSGFIXME: does ot work like that!!!
  // depends on a not so wide available extension

  _colorMatrix = new osg::ColorMatrix;
  osg::StateSet* stateSet = _branch->getOrCreateStateSet();
  stateSet->setAttribute(_colorMatrix.get());
}

SGBlendAnimation::~SGBlendAnimation ()
{
    delete _table;
}

int
SGBlendAnimation::update()
{
  double _blend;

  if ( _use_personality && current_object ) {
    SGPersonalityBranch *key = current_object;
    if ( !key->getIntValue( this, INIT_BLEND ) ) {
      key->setDoubleValue( _factor.shuffle(), this, FACTOR_BLEND );
      key->setDoubleValue( _offset.shuffle(), this, OFFSET_BLEND );

      key->setIntValue( 1, this, INIT_BLEND );
    }

    _factor = key->getDoubleValue( this, FACTOR_BLEND );
    _offset = key->getDoubleValue( this, OFFSET_BLEND );
  }

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
    _colorMatrix->getMatrix()(3, 3) = _blend;
  }
  return 1;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGTranslateAnimation
////////////////////////////////////////////////////////////////////////

SGTranslateAnimation::SGTranslateAnimation( SGPropertyNode *prop_root,
                                        SGPropertyNode_ptr props )
  : SGAnimation(props, new osg::MatrixTransform),
    _use_personality( props->getBoolValue("use-personality",false) ),
    _prop((SGPropertyNode *)prop_root->getNode(props->getStringValue("property", "/null"), true)),
    _offset_m( props, "offset-m", 0.0 ),
    _factor( props, "factor", 1.0 ),
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
  _axis.normalize();
}

SGTranslateAnimation::~SGTranslateAnimation ()
{
  delete _table;
}

int
SGTranslateAnimation::update()
{
  if (_condition == 0 || _condition->test()) {
    if ( _use_personality && current_object ) {
      SGPersonalityBranch *key = current_object;
      if ( !key->getIntValue( this, INIT_TRANSLATE ) ) {
        key->setDoubleValue( _factor.shuffle(), this, FACTOR_TRANSLATE );
        key->setDoubleValue( _offset_m.shuffle(), this, OFFSET_TRANSLATE );
      }

      _factor = key->getDoubleValue( this, FACTOR_TRANSLATE );
      _offset_m = key->getDoubleValue( this, OFFSET_TRANSLATE );

      key->setIntValue( 1, this, INIT_TRANSLATE );
    }

    if (_table == 0) {
      _position_m = (_prop->getDoubleValue() * _factor) + _offset_m;
      if (_has_min && _position_m < _min_m)
        _position_m = _min_m;
      if (_has_max && _position_m > _max_m)
        _position_m = _max_m;
    } else {
      _position_m = _table->interpolate(_prop->getDoubleValue());
    }

    osg::Matrix _matrix;
    set_translation(_matrix, _position_m, _axis);
    static_cast<osg::MatrixTransform*>(_branch)->setMatrix(_matrix);
  }
  return 2;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGScaleAnimation
////////////////////////////////////////////////////////////////////////

SGScaleAnimation::SGScaleAnimation( SGPropertyNode *prop_root,
                                        SGPropertyNode_ptr props )
  : SGAnimation(props, new osg::MatrixTransform),
    _use_personality( props->getBoolValue("use-personality",false) ),
    _prop((SGPropertyNode *)prop_root->getNode(props->getStringValue("property", "/null"), true)),
    _x_factor(props,"x-factor",1.0),
    _y_factor(props,"y-factor",1.0),
    _z_factor(props,"z-factor",1.0),
    _x_offset(props,"x-offset",1.0),
    _y_offset(props,"y-offset",1.0),
    _z_offset(props,"z-offset",1.0),
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
  if ( _use_personality && current_object ) {
    SGPersonalityBranch *key = current_object;
    if ( !key->getIntValue( this, INIT_SCALE ) ) {
      key->setDoubleValue( _x_factor.shuffle(), this, X_FACTOR_SCALE );
      key->setDoubleValue( _x_offset.shuffle(), this, X_OFFSET_SCALE );
      key->setDoubleValue( _y_factor.shuffle(), this, Y_FACTOR_SCALE );
      key->setDoubleValue( _y_offset.shuffle(), this, Y_OFFSET_SCALE );
      key->setDoubleValue( _z_factor.shuffle(), this, Z_FACTOR_SCALE );
      key->setDoubleValue( _z_offset.shuffle(), this, Z_OFFSET_SCALE );

      key->setIntValue( 1, this, INIT_SCALE );
    }

    _x_factor = key->getDoubleValue( this, X_FACTOR_SCALE );
    _x_offset = key->getDoubleValue( this, X_OFFSET_SCALE );
    _y_factor = key->getDoubleValue( this, Y_FACTOR_SCALE );
    _y_offset = key->getDoubleValue( this, Y_OFFSET_SCALE );
    _z_factor = key->getDoubleValue( this, Z_FACTOR_SCALE );
    _z_offset = key->getDoubleValue( this, Z_OFFSET_SCALE );
  }

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

  osg::Matrix _matrix;
  set_scale(_matrix, _x_scale, _y_scale, _z_scale );
  static_cast<osg::MatrixTransform*>(_branch)->setMatrix(_matrix);
  return 2;
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGTexRotateAnimation
////////////////////////////////////////////////////////////////////////

SGTexRotateAnimation::SGTexRotateAnimation( SGPropertyNode *prop_root,
                                  SGPropertyNode_ptr props )
    : SGAnimation(props, new osg::Group),
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
  SGPropertyNode *node = props->getChild("condition");
  if (node != 0)
    _condition = sgReadCondition(prop_root, node);

  _center[0] = props->getFloatValue("center/x", 0);
  _center[1] = props->getFloatValue("center/y", 0);
  _center[2] = props->getFloatValue("center/z", 0);
  _axis[0] = props->getFloatValue("axis/x", 0);
  _axis[1] = props->getFloatValue("axis/y", 0);
  _axis[2] = props->getFloatValue("axis/z", 0);
  _axis.normalize();

  osg::StateSet* stateSet = _branch->getOrCreateStateSet();
  _texMat = new osg::TexMat;
  stateSet->setTextureAttribute(0, _texMat.get());
}

SGTexRotateAnimation::~SGTexRotateAnimation ()
{
  delete _table;
}

int
SGTexRotateAnimation::update()
{
  if (_condition && !_condition->test())
    return 1;

  if (_table == 0) {
   _position_deg = _prop->getDoubleValue() * _factor + _offset_deg;
   if (_has_min && _position_deg < _min_deg)
     _position_deg = _min_deg;
   if (_has_max && _position_deg > _max_deg)
     _position_deg = _max_deg;
  } else {
    _position_deg = _table->interpolate(_prop->getDoubleValue());
  }
  osg::Matrix _matrix;
  set_rotation(_matrix, _position_deg, _center, _axis);
  _texMat->setMatrix(_matrix);
  return 2;
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGTexTranslateAnimation
////////////////////////////////////////////////////////////////////////

SGTexTranslateAnimation::SGTexTranslateAnimation( SGPropertyNode *prop_root,
                                        SGPropertyNode_ptr props )
  : SGAnimation(props, new osg::Group),
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
    _position(props->getDoubleValue("starting-position", 0)),
    _condition(0)
{
  SGPropertyNode *node = props->getChild("condition");
  if (node != 0)
    _condition = sgReadCondition(prop_root, node);

  _axis[0] = props->getFloatValue("axis/x", 0);
  _axis[1] = props->getFloatValue("axis/y", 0);
  _axis[2] = props->getFloatValue("axis/z", 0);
  _axis.normalize();

  osg::StateSet* stateSet = _branch->getOrCreateStateSet();
  _texMat = new osg::TexMat;
  stateSet->setTextureAttribute(0, _texMat.get());
}

SGTexTranslateAnimation::~SGTexTranslateAnimation ()
{
  delete _table;
}

int
SGTexTranslateAnimation::update()
{
  if (_condition && !_condition->test())
    return 1;

  if (_table == 0) {
    _position = (apply_mods(_prop->getDoubleValue(), _step, _scroll) + _offset) * _factor;
    if (_has_min && _position < _min)
      _position = _min;
    if (_has_max && _position > _max)
      _position = _max;
  } else {
    _position = _table->interpolate(apply_mods(_prop->getDoubleValue(), _step, _scroll));
  }
  osg::Matrix _matrix;
  set_translation(_matrix, _position, _axis);
  _texMat->setMatrix(_matrix);
  return 2;
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGTexMultipleAnimation
////////////////////////////////////////////////////////////////////////

SGTexMultipleAnimation::SGTexMultipleAnimation( SGPropertyNode *prop_root,
                                        SGPropertyNode_ptr props )
  : SGAnimation(props, new osg::Group),
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
      _transform[i].axis.normalize();
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
      _transform[i].axis.normalize();
      _num_transforms++;
    }
  }
  osg::StateSet* stateSet = _branch->getOrCreateStateSet();
  _texMat = new osg::TexMat;
  stateSet->setTextureAttribute(0, _texMat.get());
}

SGTexMultipleAnimation::~SGTexMultipleAnimation ()
{
   delete [] _transform;
}

int
SGTexMultipleAnimation::update()
{
  int i;
  osg::Matrix tmatrix;
  tmatrix.makeIdentity();
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
      osg::Matrix matrix;
      set_translation(matrix, _transform[i].position, _transform[i].axis);
      tmatrix = matrix*tmatrix;

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

      osg::Matrix matrix;
      set_rotation(matrix, _transform[i].position, _transform[i].center, _transform[i].axis);
      tmatrix = matrix*tmatrix;
    }
  }
  _texMat->setMatrix(tmatrix);
  return 2;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGAlphaTestAnimation
////////////////////////////////////////////////////////////////////////

SGAlphaTestAnimation::SGAlphaTestAnimation(SGPropertyNode_ptr props)
  : SGAnimation(props, new osg::Group)
{
  _alpha_clamp = props->getFloatValue("alpha-factor", 0.0);
}

SGAlphaTestAnimation::~SGAlphaTestAnimation ()
{
}

void SGAlphaTestAnimation::init()
{
  osg::StateSet* stateSet = _branch->getOrCreateStateSet();
  osg::AlphaFunc* alphaFunc = new osg::AlphaFunc;
  alphaFunc->setFunction(osg::AlphaFunc::GREATER);
  alphaFunc->setReferenceValue(_alpha_clamp);
  stateSet->setAttribute(alphaFunc);
  stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);
  stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGMaterialAnimation
////////////////////////////////////////////////////////////////////////

SGMaterialAnimation::SGMaterialAnimation( SGPropertyNode *prop_root,
        SGPropertyNode_ptr props, const SGPath &texture_path)
    : SGAnimation(props, new osg::Group),
    _last_condition(false),
    _prop_root(prop_root),
    _prop_base(""),
    _texture_base(texture_path),
    _read(0),
    _update(0),
    _global(props->getBoolValue("global", false))
{
    SGPropertyNode_ptr n;
    n = props->getChild("condition");
    _condition = n ? sgReadCondition(_prop_root, n) : 0;
    n = props->getChild("property-base");
    if (n) {
        _prop_base = n->getStringValue();
        if (!_prop_base.empty() && _prop_base.end()[-1] != '/')
            _prop_base += '/';
    }

    initColorGroup(props->getChild("diffuse"), &_diff, DIFFUSE);
    initColorGroup(props->getChild("ambient"), &_amb, AMBIENT);
    initColorGroup(props->getChild("emission"), &_emis, EMISSION);
    initColorGroup(props->getChild("specular"), &_spec, SPECULAR);

    _shi = props->getFloatValue("shininess", -1.0);
    if (_shi >= 0.0)
        _update |= SHININESS;

    SGPropertyNode_ptr group = props->getChild("transparency");
    if (group) {
        _trans.value = group->getFloatValue("alpha", -1.0);
        _trans.factor = group->getFloatValue("factor", 1.0);
        _trans.offset = group->getFloatValue("offset", 0.0);
        _trans.min = group->getFloatValue("min", 0.0);
        if (_trans.min < 0.0)
            _trans.min = 0.0;
        _trans.max = group->getFloatValue("max", 1.0);
        if (_trans.max > 1.0)
            _trans.max = 1.0;
        if (_trans.dirty())
            _update |= TRANSPARENCY;

        n = group->getChild("alpha-prop");
        _trans.value_prop = n ? _prop_root->getNode(path(n->getStringValue()), true) : 0;
        n = group->getChild("factor-prop");
        _trans.factor_prop = n ? _prop_root->getNode(path(n->getStringValue()), true) : 0;
        n = group->getChild("offset-prop");
        _trans.offset_prop = n ? _prop_root->getNode(path(n->getStringValue()), true) : 0;
        if (_trans.live())
            _read |= TRANSPARENCY;
    }

    _thresh = props->getFloatValue("threshold", -1.0);
    if (_thresh >= 0.0)
        _update |= THRESHOLD;

    string _texture_str = props->getStringValue("texture", "");
    if (!_texture_str.empty()) {
        _texture = _texture_base;
        _texture.append(_texture_str);
        _update |= TEXTURE;
    }

    n = props->getChild("shininess-prop");
    _shi_prop = n ? _prop_root->getNode(path(n->getStringValue()), true) : 0;
    n = props->getChild("threshold-prop");
    _thresh_prop = n ? _prop_root->getNode(path(n->getStringValue()), true) : 0;
    n = props->getChild("texture-prop");
    _tex_prop = n ? _prop_root->getNode(path(n->getStringValue()), true) : 0;

    _static_update = _update;

    _alphaFunc = new osg::AlphaFunc(osg::AlphaFunc::GREATER, 0);
    _texture2D = SGLoadTexture2D(_texture);
}

void SGMaterialAnimation::initColorGroup(SGPropertyNode_ptr group, ColorSpec *col, int flag)
{
    if (!group)
        return;

    col->red = group->getFloatValue("red", -1.0);
    col->green = group->getFloatValue("green", -1.0);
    col->blue = group->getFloatValue("blue", -1.0);
    col->factor = group->getFloatValue("factor", 1.0);
    col->offset = group->getFloatValue("offset", 0.0);
    if (col->dirty())
        _update |= flag;

    SGPropertyNode *n;
    n = group->getChild("red-prop");
    col->red_prop = n ? _prop_root->getNode(path(n->getStringValue()), true) : 0;
    n = group->getChild("green-prop");
    col->green_prop = n ? _prop_root->getNode(path(n->getStringValue()), true) : 0;
    n = group->getChild("blue-prop");
    col->blue_prop = n ? _prop_root->getNode(path(n->getStringValue()), true) : 0;
    n = group->getChild("factor-prop");
    col->factor_prop = n ? _prop_root->getNode(path(n->getStringValue()), true) : 0;
    n = group->getChild("offset-prop");
    col->offset_prop = n ? _prop_root->getNode(path(n->getStringValue()), true) : 0;
    if (col->live())
        _read |= flag;
}

void SGMaterialAnimation::init()
{
    if (!_global)
        cloneMaterials(_branch);

    // OSGFIXME
    osg::StateSet* stateSet = _branch->getOrCreateStateSet();
    if (_update & THRESHOLD) {
      stateSet->setAttribute(_alphaFunc.get(), osg::StateAttribute::OVERRIDE);
      stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    }
    if (_update & TEXTURE) {
      stateSet->setTextureAttribute(0, _texture2D.get(), osg::StateAttribute::OVERRIDE);
      stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    }
}

int SGMaterialAnimation::update()
{
    if (_condition) {
        bool cond = _condition->test();
        if (cond && !_last_condition)
            _update |= _static_update;

        _last_condition = cond;
        if (!cond)
            return 2;
    }

    if (_read & DIFFUSE)
        updateColorGroup(&_diff, DIFFUSE);
    if (_read & AMBIENT)
        updateColorGroup(&_amb, AMBIENT);
    if (_read & EMISSION)
        updateColorGroup(&_emis, EMISSION);
    if (_read & SPECULAR)
        updateColorGroup(&_spec, SPECULAR);

    float f;
    if (_shi_prop) {
        f = _shi;
        _shi = _shi_prop->getFloatValue();
        if (_shi != f)
            _update |= SHININESS;
    }
    if (_read & TRANSPARENCY) {
        PropSpec tmp = _trans;
        if (_trans.value_prop)
            _trans.value = _trans.value_prop->getFloatValue();
        if (_trans.factor_prop)
            _trans.factor = _trans.factor_prop->getFloatValue();
        if (_trans.offset_prop)
            _trans.offset = _trans.offset_prop->getFloatValue();
        if (_trans != tmp)
            _update |= TRANSPARENCY;
    }
    if (_thresh_prop) {
        f = _thresh;
        _thresh = _thresh_prop->getFloatValue();
        if (_thresh != f)
            _update |= THRESHOLD;
    }
    if (_tex_prop) {
        string t = _tex_prop->getStringValue();
        if (!t.empty() && t != _texture_str) {
            _texture_str = t;
            _texture = _texture_base;
            _texture.append(t);
            _update |= TEXTURE;
        }
    }
    if (_update) {
        setMaterialBranch(_branch);
        _update = 0;
    }
    return 2;
}

void SGMaterialAnimation::updateColorGroup(ColorSpec *col, int flag)
{
    ColorSpec tmp = *col;
    if (col->red_prop)
        col->red = col->red_prop->getFloatValue();
    if (col->green_prop)
        col->green = col->green_prop->getFloatValue();
    if (col->blue_prop)
        col->blue = col->blue_prop->getFloatValue();
    if (col->factor_prop)
        col->factor = col->factor_prop->getFloatValue();
    if (col->offset_prop)
        col->offset = col->offset_prop->getFloatValue();
    if (*col != tmp)
        _update |= flag;
}

class SGMaterialAnimationCloneVisitor : public osg::NodeVisitor {
public:
  SGMaterialAnimationCloneVisitor() :
    osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
  {
    setVisitorType(osg::NodeVisitor::NODE_VISITOR);
  }
  virtual void apply(osg::Node& node)
  {
    traverse(node);
    osg::StateSet* stateSet = node.getStateSet();
    if (!stateSet)
      return;
    if (1 < stateSet->referenceCount()) {
      osg::CopyOp copyOp(osg::CopyOp::DEEP_COPY_STATESETS);
      osg::Object* object = stateSet->clone(copyOp);
      stateSet = static_cast<osg::StateSet*>(object);
      node.setStateSet(stateSet);
    }
    cloneMaterial(stateSet);
  }
  virtual void apply(osg::Geode& node)
  {
    apply((osg::Node&)node);
    traverse(node);
    unsigned nDrawables = node.getNumDrawables();
    for (unsigned i = 0; i < nDrawables; ++i) {
      osg::Drawable* drawable = node.getDrawable(i);
      osg::StateSet* stateSet = drawable->getStateSet();
      if (!stateSet)
        continue;
      if (1 < stateSet->referenceCount()) {
        osg::CopyOp copyOp(osg::CopyOp::DEEP_COPY_STATESETS);
        osg::Object* object = stateSet->clone(copyOp);
        stateSet = static_cast<osg::StateSet*>(object);
        drawable->setStateSet(stateSet);
      }
      cloneMaterial(stateSet);
    }
  }
  void cloneMaterial(osg::StateSet* stateSet)
  {
    
    osg::StateAttribute* stateAttr;
    stateAttr = stateSet->getAttribute(osg::StateAttribute::MATERIAL);
    if (!stateAttr)
      return;
    osg::CopyOp copyOp(osg::CopyOp::DEEP_COPY_STATEATTRIBUTES);
    osg::Object* object = stateAttr->clone(copyOp);
    osg::Material* material = static_cast<osg::Material*>(object);
    materialList.push_back(material);
    while (stateSet->getAttribute(osg::StateAttribute::MATERIAL)) {
      stateSet->removeAttribute(osg::StateAttribute::MATERIAL);
    }
    stateSet->setAttribute(material);
  }
  std::vector<osg::Material*> materialList;
};

void SGMaterialAnimation::cloneMaterials(osg::Group *b)
{
  SGMaterialAnimationCloneVisitor cloneVisitor;
  b->accept(cloneVisitor);
  _materialList.swap(cloneVisitor.materialList);
}

void SGMaterialAnimation::setMaterialBranch(osg::Group *b)
{
  std::vector<osg::Material*>::iterator i;
  for (i = _materialList.begin(); i != _materialList.end(); ++i) {
    osg::Material* material = *i;
    if (_update & DIFFUSE) {
      osg::Vec4 v = _diff.rgba();
      float alpha = material->getDiffuse(osg::Material::FRONT_AND_BACK)[3];
      material->setColorMode(osg::Material::DIFFUSE);
      material->setDiffuse(osg::Material::FRONT_AND_BACK,
                           osg::Vec4(v[0], v[1], v[2], alpha));
    }
    if (_update & AMBIENT) {
      material->setColorMode(osg::Material::AMBIENT);
      material->setDiffuse(osg::Material::FRONT_AND_BACK, _amb.rgba());
    }
    if (_update & EMISSION)
      material->setEmission(osg::Material::FRONT_AND_BACK, _emis.rgba());
    if (_update & SPECULAR)
      material->setSpecular(osg::Material::FRONT_AND_BACK, _spec.rgba());
    if (_update & SHININESS)
      material->setShininess(osg::Material::FRONT_AND_BACK,
                             clamp(_shi, 0.0, 128.0));
    if (_update & TRANSPARENCY) {
      osg::Vec4 v = material->getDiffuse(osg::Material::FRONT_AND_BACK);
      float trans = _trans.value * _trans.factor + _trans.offset;
      trans = trans < _trans.min ? _trans.min : trans > _trans.max ? _trans.max : trans;
      material->setDiffuse(osg::Material::FRONT_AND_BACK,
                           osg::Vec4(v[0], v[1], v[2], trans));
    }
    if (_update & THRESHOLD)
        _alphaFunc->setReferenceValue(clamp(_thresh));
    // OSGFIXME
//     if (_update & TEXTURE)
//         s->setTexture(_texture.c_str());
//     if (_update & (TEXTURE|TRANSPARENCY)) {
//         SGfloat alpha = s->getMaterial(GL_DIFFUSE)[3];
//         ssgTexture *tex = s->getTexture();
//         if ((tex && tex->hasAlpha()) || alpha < 0.999) {
//             s->setColourMaterial(GL_DIFFUSE);
//             s->enable(GL_COLOR_MATERIAL);
//             s->enable(GL_BLEND);
//             s->enable(GL_ALPHA_TEST);
//             s->setTranslucent();
//             s->disable(GL_COLOR_MATERIAL);
//         } else {
//             s->disable(GL_BLEND);
//             s->disable(GL_ALPHA_TEST);
//             s->setOpaque();
//         }
//     }
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGFlashAnimation
////////////////////////////////////////////////////////////////////////
class SGFlashAnimationTransform : public osg::Transform {
public:
  SGFlashAnimationTransform(SGPropertyNode* props)
  {
    getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

    _axis[0] = props->getFloatValue("axis/x", 0);
    _axis[1] = props->getFloatValue("axis/y", 0);
    _axis[2] = props->getFloatValue("axis/z", 1);
    _axis.normalize();
    
    _center[0] = props->getFloatValue("center/x-m", 0);
    _center[1] = props->getFloatValue("center/y-m", 0);
    _center[2] = props->getFloatValue("center/z-m", 0);
    
    _offset = props->getFloatValue("offset", 0.0);
    _factor = props->getFloatValue("factor", 1.0);
    _power = props->getFloatValue("power", 1.0);
    _two_sides = props->getBoolValue("two-sides", false);
    
    _min_v = props->getFloatValue("min", 0.0);
    _max_v = props->getFloatValue("max", 1.0);
  }

  virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const 
  {
    double scale_factor = computeScaleFactor(nv);
    osg::Matrix transform;
    transform(0,0) = scale_factor;
    transform(1,1) = scale_factor;
    transform(2,2) = scale_factor;
    transform(3,0) = _center[0] * ( 1 - scale_factor );
    transform(3,1) = _center[1] * ( 1 - scale_factor );
    transform(3,2) = _center[2] * ( 1 - scale_factor );
    if (_referenceFrame == RELATIVE_RF)
      matrix.preMult(transform);
    else
      matrix = transform;

    return true;
  }
  
  virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const
  {
    double scale_factor = computeScaleFactor(nv);
    if (fabs(scale_factor) <= std::numeric_limits<double>::min())
      return false;
    osg::Matrix transform;
    double rScaleFactor = 1/scale_factor;
    transform(0,0) = rScaleFactor;
    transform(1,1) = rScaleFactor;
    transform(2,2) = rScaleFactor;
    transform(3,0) = rScaleFactor*_center[0] * ( scale_factor - 1 );
    transform(3,1) = rScaleFactor*_center[1] * ( scale_factor - 1 );
    transform(3,2) = rScaleFactor*_center[2] * ( scale_factor - 1 );
    if (_referenceFrame == RELATIVE_RF)
      matrix.postMult(transform);
    else
      matrix = transform;
    return true;
  }

  double computeScaleFactor(osg::NodeVisitor* nv) const
  {
    if (!nv)
      return 1;

    osg::Vec3 localEyeToCenter = nv->getEyePoint() - _center;
    localEyeToCenter.normalize();

    double cos_angle = localEyeToCenter*_axis;
    double scale_factor = 0;
    if ( _two_sides && cos_angle < 0 )
      scale_factor = _factor * pow( -cos_angle, _power ) + _offset;
    else if ( cos_angle > 0 )
      scale_factor = _factor * pow( cos_angle, _power ) + _offset;
    
    if ( scale_factor < _min_v )
      scale_factor = _min_v;
    if ( scale_factor > _max_v )
      scale_factor = _max_v;

    return scale_factor;
  }

private:
  osg::Vec3 _axis, _center;
  double _power, _factor, _offset, _min_v, _max_v;
  bool _two_sides;
};

SGFlashAnimation::SGFlashAnimation(SGPropertyNode_ptr props)
  : SGAnimation( props, new SGFlashAnimationTransform(props) )
{
}

SGFlashAnimation::~SGFlashAnimation()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGDistScaleAnimation
////////////////////////////////////////////////////////////////////////
class SGDistScaleTransform : public osg::Transform {
public:
  SGDistScaleTransform(SGPropertyNode* props)
  {
    getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

    _factor = props->getFloatValue("factor", 1.0);
    _offset = props->getFloatValue("offset", 0.0);
    _min_v = props->getFloatValue("min", 0.0);
    _max_v = props->getFloatValue("max", 1.0);
    _has_min = props->hasValue("min");
    _has_max = props->hasValue("max");
    _table = read_interpolation_table(props);
    _center[0] = props->getFloatValue("center/x-m", 0);
    _center[1] = props->getFloatValue("center/y-m", 0);
    _center[2] = props->getFloatValue("center/z-m", 0);
  }
  ~SGDistScaleTransform()
  {
    delete _table;
  }

  virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const 
  {
    osg::Matrix transform;
    double scale_factor = computeScaleFactor(nv);
    transform(0,0) = scale_factor;
    transform(1,1) = scale_factor;
    transform(2,2) = scale_factor;
    transform(3,0) = _center[0] * ( 1 - scale_factor );
    transform(3,1) = _center[1] * ( 1 - scale_factor );
    transform(3,2) = _center[2] * ( 1 - scale_factor );
    if (_referenceFrame == RELATIVE_RF)
      matrix.preMult(transform);
    else
      matrix = transform;
    return true;
  }
  
  virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const
  {
    double scale_factor = computeScaleFactor(nv);
    if (fabs(scale_factor) <= std::numeric_limits<double>::min())
      return false;
    osg::Matrix transform;
    double rScaleFactor = 1/scale_factor;
    transform(0,0) = rScaleFactor;
    transform(1,1) = rScaleFactor;
    transform(2,2) = rScaleFactor;
    transform(3,0) = rScaleFactor*_center[0] * ( scale_factor - 1 );
    transform(3,1) = rScaleFactor*_center[1] * ( scale_factor - 1 );
    transform(3,2) = rScaleFactor*_center[2] * ( scale_factor - 1 );
    if (_referenceFrame == RELATIVE_RF)
      matrix.postMult(transform);
    else
      matrix = transform;
    return true;
  }

  double computeScaleFactor(osg::NodeVisitor* nv) const
  {
    if (!nv)
      return 1;

    osg::Vec3 localEyeToCenter = _center - nv->getEyePoint();
    double scale_factor = localEyeToCenter.length();
    if (_table == 0) {
      scale_factor = _factor * scale_factor + _offset;
      if ( _has_min && scale_factor < _min_v )
        scale_factor = _min_v;
      if ( _has_max && scale_factor > _max_v )
        scale_factor = _max_v;
    } else {
      scale_factor = _table->interpolate( scale_factor );
    }

    return scale_factor;
  }


private:
  osg::Vec3 _center;
  float _factor, _offset, _min_v, _max_v;
  bool _has_min, _has_max;
  SGInterpTable * _table;
};

SGDistScaleAnimation::SGDistScaleAnimation(SGPropertyNode_ptr props)
  : SGAnimation( props, new SGDistScaleTransform(props) )
{
}

SGDistScaleAnimation::~SGDistScaleAnimation()
{
}

////////////////////////////////////////////////////////////////////////
// Implementation of SGShadowAnimation
////////////////////////////////////////////////////////////////////////

SGShadowAnimation::SGShadowAnimation ( SGPropertyNode *prop_root,
                                       SGPropertyNode_ptr props )
  : SGAnimation(props, new osg::Group),
    _condition(0),
    _condition_value(true)
{
    animation_type = 1;
    SGPropertyNode_ptr node = props->getChild("condition");
    if (node != 0) {
        _condition = sgReadCondition(prop_root, node);
        _condition_value = false;
    }
}

SGShadowAnimation::~SGShadowAnimation ()
{
    delete _condition;
}

int
SGShadowAnimation::update()
{
    if (_condition)
        _condition_value = _condition->test();

    if ( _condition_value ) {
        _branch->setNodeMask(SG_NODEMASK_SHADOW_BIT|_branch->getNodeMask());
    } else {
        _branch->setNodeMask(~SG_NODEMASK_SHADOW_BIT&_branch->getNodeMask());
    }
    return 2;
}

bool SGShadowAnimation::get_condition_value(void) {
    return _condition_value;
}

// end of animation.cxx
