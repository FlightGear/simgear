// Subsystem that manages interpolation of properties.
//
// Copyright (C) 2013  Thomas Geymayer <tomgey@gmail.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#include "PropertyInterpolationMgr.hxx"
#include "PropertyInterpolator.hxx"

#ifndef SIMGEAR_HEADLESS
# include <simgear/scene/util/ColorInterpolator.hxx>
#endif

#include <simgear/props/props.hxx>
#include <simgear_config.h>

#include <algorithm>

namespace simgear
{

  //----------------------------------------------------------------------------
  PropertyInterpolationMgr::PropertyInterpolationMgr()
  {
    addInterpolatorFactory<NumericInterpolator>("numeric");
#ifndef SIMGEAR_HEADLESS
    addInterpolatorFactory<ColorInterpolator>("color");
#endif

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
    for( InterpolatorList::iterator it = _interpolators.begin();
                                    it != _interpolators.end();
                                  ++it )
    {
      for(double unused_time = dt;;)
      {
        PropertyInterpolatorRef interp = it->second;
        unused_time = interp->update(it->first, unused_time);

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
  PropertyInterpolatorRef
  PropertyInterpolationMgr::createInterpolator( const std::string& type,
                                                const SGPropertyNode* target,
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

    PropertyInterpolatorRef interp;
    interp = (*interpolator_factory->second)(target);
    interp->_type = type;
    interp->_duration = duration;
    interp->_easing = easing_func->second;

    return interp;
  }

  //----------------------------------------------------------------------------
  void PropertyInterpolationMgr::interpolate( SGPropertyNode* prop,
                                              PropertyInterpolatorRef interp )
  {
    // Search for active interpolator on given property
    InterpolatorList::iterator it = std::find_if
    (
      _interpolators.begin(),
      _interpolators.end(),
      PredicateIsSameProp(prop)
    );

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

} // namespace simgear
