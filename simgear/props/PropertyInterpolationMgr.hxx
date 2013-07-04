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

#ifndef SG_PROPERTY_INTERPOLATION_MGR_HXX_
#define SG_PROPERTY_INTERPOLATION_MGR_HXX_

#include "PropertyInterpolator.hxx"

#include <simgear/math/sg_types.hxx>
#include <simgear/misc/make_new.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include <list>

namespace simgear
{

  /**
   * Subsystem that manages interpolation of properties.
   *
   * By default the numeric values of the properties are interpolated. For
   * example, for strings this is probably not the wanted behavior. For this
   * adapter classes can be registered to allow providing specific
   * interpolations for certain types of properties. Using the type "color",
   * provided by ColorInterpolator, strings containing %CSS colors can also be
   * interpolated.
   *
   * Additionally different functions can be used for easing of the animation.
   * By default "linear" (constant animation speed) and "swing" (smooth
   * acceleration and deceleration) are available.
   */
  class PropertyInterpolationMgr:
    public SGSubsystem
  {
    public:
      typedef PropertyInterpolator* (*InterpolatorFactory)();

      PropertyInterpolationMgr();

      /**
       * Update all active interpolators.
       */
      void update(double dt);

      /**
       * Create a new property interpolator.
       *
       * @note To actually use it the interpolator needs to be attached to a
       *       property using PropertyInterpolationMgr::interpolate.
       *
       * @param type        Type of animation ("numeric", "color", etc.)
       * @param target      Property containing target value
       * @param duration    Duration if the animation (in seconds)
       * @param easing      Type of easing ("linear", "swing", etc.)
       */
      PropertyInterpolator*
      createInterpolator( const std::string& type,
                          const SGPropertyNode& target,
                          double duration,
                          const std::string& easing );

      /**
       * Add animation of the given property from its current value to the
       * target value of the interpolator. If no interpolator is given any
       * existing animation of the given property is aborted.
       *
       * @param prop    Property to be interpolated
       * @param interp  Interpolator used for interpolation
       */
      bool interpolate( SGPropertyNode* prop,
                        PropertyInterpolatorRef interp  = 0 );

      bool interpolate( SGPropertyNode* prop,
                        const std::string& type,
                        const SGPropertyNode& target,
                        double duration,
                        const std::string& easing );

      bool interpolate( SGPropertyNode* prop,
                        const std::string& type,
                        const PropertyList& values,
                        const double_list& deltas,
                        const std::string& easing );

      /**
       * Register factory for interpolation type.
       */
      void addInterpolatorFactory( const std::string& type,
                                   InterpolatorFactory factory );
      template<class T>
      void addInterpolatorFactory(const std::string& type)
      {
        addInterpolatorFactory
        (
          type,
          &simgear::make_new_derived<PropertyInterpolator, T>
        );
      }

      /**
       * Register easing function.
       */
      void addEasingFunction(const std::string& type, easing_func_t func);

      /**
       * Set property containing real time delta (not sim time)
       *
       * TODO better pass both deltas to all update methods...
       */
      void setRealtimeProperty(SGPropertyNode* node);

    protected:

      typedef std::map<std::string, InterpolatorFactory> InterpolatorFactoryMap;
      typedef std::map<std::string, easing_func_t> EasingFunctionMap;
      typedef std::pair< SGPropertyNode*,
                         PropertyInterpolatorRef > PropertyInterpolatorPair;
      typedef std::list<PropertyInterpolatorPair> InterpolatorList;

      struct PredicateIsSameProp;

      InterpolatorFactoryMap    _interpolator_factories;
      EasingFunctionMap         _easing_functions;
      InterpolatorList          _interpolators;

      SGPropertyNode_ptr        _rt_prop;
  };

} // namespace simgear

#endif /* SG_PROPERTY_INTERPOLATION_MGR_HXX_ */
