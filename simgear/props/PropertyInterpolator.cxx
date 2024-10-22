// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Adapter for interpolating different types of properties.
 */

#include <simgear_config.h>

#include "PropertyInterpolator.hxx"
#include "props.hxx"

#include <cassert>
#include <cmath>

namespace simgear
{

  //----------------------------------------------------------------------------
  PropertyInterpolator::~PropertyInterpolator()
  {

  }

  //----------------------------------------------------------------------------
  void PropertyInterpolator::reset(const SGPropertyNode& target)
  {
    _cur_t = 0;
    setTarget(target);
  }

  //----------------------------------------------------------------------------
  void PropertyInterpolator::setEasingFunction(easing_func_t easing)
  {
    _easing = easing ? easing : easing_functions[0].func;
  }

  //----------------------------------------------------------------------------
  double PropertyInterpolator::update(SGPropertyNode& prop, double dt)
  {
    if( _cur_t == 0 )
      init(prop);

    _cur_t += dt / _duration;

    double unused = _cur_t - 1;
    if( unused > 0 )
      _cur_t = 1;

    write(prop, _easing(_cur_t) );

    if( _cur_t == 1 )
      // Reset timer to allow animation to be run again.
      _cur_t = 0;

    return unused;
  }

  //----------------------------------------------------------------------------
  PropertyInterpolator::PropertyInterpolator():
    _duration(1),
    _cur_t(0)
  {
    setEasingFunction(0);
  }

  //----------------------------------------------------------------------------
  void NumericInterpolator::setTarget(const SGPropertyNode& target)
  {
    _end = target.getDoubleValue();
  }

  //----------------------------------------------------------------------------
  void NumericInterpolator::init(const SGPropertyNode& prop)
  {
    // If unable to get start value, immediately change to target value
    double value_start = prop.getType() == props::NONE
                       ? _end
                       : prop.getDoubleValue();

    _diff = _end - value_start;
  }

  //----------------------------------------------------------------------------
  void NumericInterpolator::write(SGPropertyNode& prop, double t)
  {
    double cur = _end - (1 - t) * _diff;

    if( prop.getType() == props::INT || prop.getType() == props::LONG )
      prop.setLongValue( static_cast<long>(std::floor(cur + 0.5)) );
    else
      prop.setDoubleValue(cur);
  }

} // namespace simgear
