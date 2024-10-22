// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2010 Torsten Dreyer

#include <simgear_config.h>

#include <cstdlib>

#include "inputvalue.hxx"

#include <simgear/misc/strutils.hxx>

using namespace simgear;

//------------------------------------------------------------------------------
PeriodicalValue::PeriodicalValue( SGPropertyNode& prop_root,
                                  SGPropertyNode& cfg )
{
  SGPropertyNode_ptr minNode = cfg.getChild( "min" );
  SGPropertyNode_ptr maxNode = cfg.getChild( "max" );
  if( !minNode || !maxNode )
  {
      SG_LOG(
          SG_GENERAL,
          SG_ALERT,
          "periodical defined, but no <min> and/or <max> tag. Period ignored.");
  }
  else
  {
      minPeriod = new Value(prop_root, *minNode);
      maxPeriod = new Value(prop_root, *maxNode);
  }
}

//------------------------------------------------------------------------------
double PeriodicalValue::normalize( double value ) const
{
  return SGMiscd::normalizePeriodic( minPeriod->get_value(),
                                     maxPeriod->get_value(),
                                     value );
}

//------------------------------------------------------------------------------
double PeriodicalValue::normalizeSymmetric( double value ) const
{
  double minValue = minPeriod->get_value();
  double maxValue = maxPeriod->get_value();
  
  value = SGMiscd::normalizePeriodic( minValue, maxValue, value );
  double width_2 = (maxValue - minValue)/2;
  return value > width_2 ? width_2 - value : value;
}

//------------------------------------------------------------------------------
Value::Value(SGPropertyNode& prop_root,
             SGPropertyNode& cfg,
             double value,
             double offset,
             double scale)
{
  parse(prop_root, cfg, value, offset, scale);
}

Value::Value(double value) : _value(value)
{
}

Value::~Value()
{
    if (_pathNode) {
        _pathNode->removeChangeListener(this);
    }
}

void Value::initPropertyFromInitialValue()
{
    double s = get_scale();
    if( s != 0 )
      _property->setDoubleValue( (_value - get_offset())/s );
    else
      _property->setDoubleValue(0); // if scale is zero, value*scale is zero
}

//------------------------------------------------------------------------------
void Value::parse(SGPropertyNode& prop_root,
                  SGPropertyNode& cfg,
                  double aValue,
                  double aOffset,
                  double aScale)
{
  _value = aValue;
  _property.reset();
  _offset.reset();
  _scale.reset();
  _min.reset();
  _max.reset();
  _periodical.reset();

  SGPropertyNode * n;

  if( (n = cfg.getChild("condition")) != NULL )
    _condition = sgReadCondition(&prop_root, n);

  if( (n = cfg.getChild( "scale" )) != NULL )
    _scale = new Value(prop_root, *n, aScale);

  if( (n = cfg.getChild( "offset" )) != NULL )
    _offset = new Value(prop_root, *n, aOffset);

  if( (n = cfg.getChild( "max" )) != NULL )
    _max = new Value(prop_root, *n);

  if( (n = cfg.getChild( "min" )) != NULL )
    _min = new Value(prop_root, *n);

  if( (n = cfg.getChild( "abs" )) != NULL )
    _abs = n->getBoolValue();

  if( (n = cfg.getChild( "period" )) != NULL )
    _periodical = new PeriodicalValue(prop_root, *n);


  SGPropertyNode *valueNode = cfg.getChild("value");
  if( valueNode != NULL )
    _value = valueNode->getDoubleValue();

  if( (n = cfg.getChild("expression")) != NULL )
  {
    _expression = SGReadDoubleExpression(&prop_root, n->getChild(0));
    return;
  }

    if ((n = cfg.getChild("property-path"))) {
        // cache the root node, in case of changes
        _rootNode = &prop_root;
        const auto trimmed = simgear::strutils::strip(n->getStringValue());
        _pathNode = prop_root.getNode(trimmed, true);
        _pathNode->addChangeListener(this);
        
        // if <property> is defined, should we use it to initialise
        // the path prop? not doing so for now.
        
        const auto path = simgear::strutils::strip(_pathNode->getStringValue());
        if (!path.empty()) {
            _property = _rootNode->getNode(path);
        }

        return;
    }

    
  // if no <property> element, check for <prop> element for backwards
  // compatibility
  if(    (n = cfg.getChild("property"))
      || (n = cfg.getChild("prop"    )) )
  {
      // tolerate leading & trailing whitespace from XML, in the property name
      const auto trimmed = simgear::strutils::strip(n->getStringValue());
    _property = prop_root.getNode(trimmed, true);
    if( valueNode )
    {
        initPropertyFromInitialValue();
    }

    return;
  } // of have a <property> or <prop>

  const std::string nodeText = cfg.getStringValue();
  if (!valueNode && !nodeText.empty()) {
    char* endp = nullptr;
    auto startp = nodeText.c_str();
    // try to convert to a double value. If the textnode does not start with a number
    // endp will point to the beginning of the string. We assume this should be
    // a property name
    _value = strtod(startp, &endp);
    if (endp == startp) {
        const auto trimmed = simgear::strutils::strip(nodeText);
        _property = prop_root.getNode(trimmed, true);
    }
  }
}

void Value::set_value(double aValue)
{
    if (!_property)
      return;
      
    double s = get_scale();
    if( s != 0 )
        _property->setDoubleValue( (aValue - get_offset())/s );
    else
        _property->setDoubleValue( 0 ); // if scale is zero, value*scale is zero
}

double Value::get_value() const
{
    double value = _value;

    if (_expression) {
        // compute the expression value
        value = _expression->getValue(NULL);

        if (SGMiscd::isNaN(value)) {
      SG_LOG(SG_GENERAL, SG_DEV_ALERT, "Value: read NaN from expression");
        }
    } else if( _property != NULL ) {
        value = _property->getDoubleValue();

        if (SGMiscd::isNaN(value)) {
      SG_LOG(SG_GENERAL, SG_DEV_ALERT, "Value: read NaN from:" << _property->getPath());
        }
    } else {
        if (SGMiscd::isNaN(value)) {
      SG_LOG(SG_GENERAL, SG_DEV_ALERT, "Value: value is NaN.");
        }
    }
    
    if( _scale ) 
        value *= _scale->get_value();

    if( _offset ) 
        value += _offset->get_value();

    if( _min ) {
        double m = _min->get_value();
        if( value < m )
            value = m;
    }

    if( _max ) {
        double m = _max->get_value();
        if( value > m )
            value = m;
    }

    if( _periodical ) {
      value = _periodical->normalize( value );
    }

    return _abs ? fabs(value) : value;
}

bool Value::is_enabled() const
{
    if (_pathNode && !_property) {
        // if we have a configurable path, and it's currently not valid,
        // mark ourselves as disabled
        return false;
    }
    
    if (_condition) {
        return _condition->test();
    }

    return true; // default to enabled
}

void Value::collectDependentProperties(std::set<const SGPropertyNode*>& props) const
{
    if (_property)      props.insert(_property);
    if (_offset)        _offset->collectDependentProperties(props);
    if (_scale)         _scale->collectDependentProperties(props);
    if (_min)           _min->collectDependentProperties(props);
    if (_max)           _max->collectDependentProperties(props);
    if (_expression)    _expression->collectDependentProperties(props);
    if (_pathNode)      props.insert(_pathNode);
}

void Value::valueChanged(SGPropertyNode* node)
{
    assert(node == _pathNode);
    const auto path = strutils::strip(_pathNode->getStringValue());
    if (path.empty()) {
        // don't consider an empty string to mean the root node, that's not
        // useful behaviour
        _property.reset();
        return;
    }
    
    // important we don't create here: this allows an invalid path
    // to give us a null _property, which causes us to be marked as
    // disabled, allowing another input to be used
    auto propNode = _rootNode->getNode(path);
    if (propNode) {
        _property = propNode;
    } else {
        _property.reset();
    }
}

void ValueList::collectDependentProperties(std::set<const SGPropertyNode*>& props) const
{
    for (auto& iv: *this) {
        iv->collectDependentProperties(props);
    }
}

Value_ptr ValueList::get_active() const
{
    for (auto it = begin(); it != end(); ++it) {
        if ((*it)->is_enabled())
            return *it;
    }

    return {};
}
