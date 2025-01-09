// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Subsystem that manages interpolation of properties.
 */

#include <simgear_config.h>
#include "PropertyInterpolationMgr.hxx"
#include "PropertyInterpolator.hxx"
#include "props.hxx"

#include <algorithm>
#include <vector>

namespace simgear
{

  //----------------------------------------------------------------------------
  PropertyInterpolationMgr::PropertyInterpolationMgr()
  {
    addInterpolatorFactory<NumericInterpolator>("numeric");

    for( size_t i = 0; easing_functions[i].name; ++i )
      addEasingFunction
      (
        easing_functions[i].name,
        easing_functions[i].func
      );
  }

  //----------------------------------------------------------------------------
  void PropertyInterpolationMgr::update(double dt)
  {
    if( _rt_prop )
      dt = _rt_prop->getDoubleValue();

    for( InterpolatorList::iterator it = _interpolators.begin();
                                    it != _interpolators.end();
                                  ++it )
    {
      for(double unused_time = dt;;)
      {
        PropertyInterpolatorRef interp = it->second;
        unused_time = interp->update(*it->first, unused_time);

        if( unused_time <= 0.0 )
          // No time left for next animation
          break;

        if( interp->_next )
        {
          // Step to next animation. Note that we do not invalidate or delete
          // the current interpolator to allow for looped animations.
          it->second = interp->_next;
        }
        else
        {
          // No more animations so just remove it
          it = _interpolators.erase(it);
          break;
        }
      }

	  // needed to avoid incrementing an invalid iterator when we
	  // erase the last interpolator
	  if (it == _interpolators.end()) {
		  break;
	  }
    }
  }

  //----------------------------------------------------------------------------
  struct PropertyInterpolationMgr::PredicateIsSameProp
  {
    public:
      PredicateIsSameProp(SGPropertyNode* node):
        _node(node)
      {}
      bool operator()(const PropertyInterpolatorPair& interp) const
      {
        return interp.first == _node;
      }
    protected:
      SGPropertyNode *_node;
  };

  //----------------------------------------------------------------------------
  PropertyInterpolator*
  PropertyInterpolationMgr::createInterpolator( const std::string& type,
                                                const SGPropertyNode& target,
                                                double duration,
                                                const std::string& easing )
  {
    InterpolatorFactoryMap::iterator interpolator_factory =
      _interpolator_factories.find(type);
    if( interpolator_factory == _interpolator_factories.end() )
    {
      SG_LOG
      (
        SG_GENERAL,
        SG_WARN,
        "PropertyInterpolationMgr: no factory found for type '" << type << "'"
      );
      return 0;
    }

    EasingFunctionMap::iterator easing_func = _easing_functions.find(easing);
    if( easing_func == _easing_functions.end() )
    {
      SG_LOG
      (
        SG_GENERAL,
        SG_WARN,
        "PropertyInterpolationMgr: no such easing '" << type << "'"
      );
      return 0;
    }

    PropertyInterpolator* interp;
    interp = (*interpolator_factory->second)();
    interp->reset(target);
    interp->_type = type;
    interp->_duration = duration;
    interp->_easing = easing_func->second;

    return interp;
  }

  //----------------------------------------------------------------------------
  bool PropertyInterpolationMgr::interpolate( SGPropertyNode* prop,
                                              PropertyInterpolatorRef interp )
  {
    if( !prop )
      return false;

    // Search for active interpolator on given property
    InterpolatorList::iterator it = std::find_if
    (
      _interpolators.begin(),
      _interpolators.end(),
      PredicateIsSameProp(prop)
    );

    if( !interp )
    {
      // Without new interpolator just remove old one
      if( it != _interpolators.end() )
        _interpolators.erase(it);
      return true;
    }

    if( it != _interpolators.end() )
    {
      // Ensure no circular reference is left
      it->second->_next = 0;

      // and now safely replace old interpolator
      // TODO maybe cache somewhere for reuse or use allocator?
      it->second = interp;
    }
    else
      _interpolators.push_front( std::make_pair(prop, interp) );

    return true;
  }

  //----------------------------------------------------------------------------
  bool PropertyInterpolationMgr::interpolate( SGPropertyNode* prop,
                                              const std::string& type,
                                              const SGPropertyNode& target,
                                              double duration,
                                              const std::string& easing )
  {
    return interpolate
    (
      prop,
      createInterpolator(type, target, duration, easing)
    );
  }

  //----------------------------------------------------------------------------
  bool PropertyInterpolationMgr::interpolate( SGPropertyNode* prop,
                                              const std::string& type,
                                              const PropertyList& values,
                                              const std::vector<double>& deltas,
                                              const std::string& easing )
  {
    if( values.size() != deltas.size() )
      SG_LOG(SG_GENERAL, SG_WARN, "interpolate: sizes do not match");

    size_t num_values = std::min(values.size(), deltas.size());
    PropertyInterpolatorRef first_interp, cur_interp;
    for(size_t i = 0; i < num_values; ++i)
    {
      assert(values[i]);

      PropertyInterpolator* interp =
        createInterpolator(type, *values[i], deltas[i], easing);

      if( !first_interp )
        first_interp = interp;
      else
        cur_interp->_next = interp;

      cur_interp = interp;
    }

    return interpolate(prop, first_interp);
  }

  //----------------------------------------------------------------------------
  void PropertyInterpolationMgr::addInterpolatorFactory
  (
    const std::string& type,
    InterpolatorFactory factory
  )
  {
    if( _interpolator_factories.find(type) != _interpolator_factories.end() )
      SG_LOG
      (
        SG_GENERAL,
        SG_WARN,
        "PropertyInterpolationMgr: replace existing factor for type " << type
      );

    _interpolator_factories[type] = factory;
  }

  //----------------------------------------------------------------------------
  void PropertyInterpolationMgr::addEasingFunction( const std::string& type,
                                                    easing_func_t func )
  {
    // TODO it's probably time for a generic factory map
    if( _easing_functions.find(type) != _easing_functions.end() )
      SG_LOG
      (
        SG_GENERAL,
        SG_WARN,
        "PropertyInterpolationMgr: replace existing easing function " << type
      );

    _easing_functions[type] = func;
  }

  //----------------------------------------------------------------------------
  void PropertyInterpolationMgr::setRealtimeProperty(SGPropertyNode* node)
  {
    _rt_prop = node;
  }

} // namespace simgear
