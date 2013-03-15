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

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/PropertyInterpolator.hxx>

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
      typedef PropertyInterpolator*
              (*InterpolatorFactory)(const SGPropertyNode* target);

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
      PropertyInterpolatorRef
      createInterpolator( const std::string& type,
                          const SGPropertyNode* target,
                          double duration = 1.0,
                          const std::string& easing = "swing" );

      /**
       * Add animation of the given property from current its current value to
       * the target value of the interpolator.
       *
       * @param prop    Property to be interpolated
       * @param interp  Interpolator used for interpolation
       */
      void interpolate( SGPropertyNode* prop,
                        PropertyInterpolatorRef interp );

      /**
       * Register factory for interpolation type.
       */
      void addInterpolatorFactory( const std::string& type,
                                   InterpolatorFactory factory );
      template<class T>
      void addInterpolatorFactory(const std::string& type)
      {
        addInterpolatorFactory(type, &PropertyInterpolator::create<T>);
      }

      /**
       * Register easing function.
       */
      void addEasingFunction(const std::string& type, easing_func_t func);

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
  };

} // namespace simgear

#endif /* SG_PROPERTY_INTERPOLATION_MGR_HXX_ */
