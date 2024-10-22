// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Adapter for interpolating different types of properties.
 */

#ifndef SG_PROPERTY_INTERPOLATOR_HXX_
#define SG_PROPERTY_INTERPOLATOR_HXX_

#include "easing_functions.hxx"
#include "propsfwd.hxx"

#include <memory>
#include <cassert>
#include <string>

#include <iostream>

namespace simgear
{

  class PropertyInterpolator;
  typedef SGSharedPtr<PropertyInterpolator> PropertyInterpolatorRef;

  /**
   * Base class for interpolating different types of properties over time.
   */
  class PropertyInterpolator:
    public SGReferenced
  {
    public:
      virtual ~PropertyInterpolator();

      /**
       * Resets animation timer to zero and prepares for interpolation to new
       * target value.
       */
      void reset(const SGPropertyNode& target);

      /**
       * Set easing function to be used for interpolation.
       */
      void setEasingFunction(easing_func_t easing);

      /**
       * Calculate an animation step.
       *
       * @param prop    Property being animated
       * @param dt      Current frame duration
       * @return Time not used by the animation (>= 0 if animation has finished,
       *         else time is negative indicating the remaining time until
       *         finished)
       */
      double update(SGPropertyNode& prop, double dt);

      const std::string& getType() const    { return _type; }

    protected:
      friend class PropertyInterpolationMgr;

      std::string               _type;
      easing_func_t             _easing;
      PropertyInterpolatorRef   _next;
      double                    _duration,
                                _cur_t;

      PropertyInterpolator();

      virtual void setTarget(const SGPropertyNode& target) = 0;
      virtual void init(const SGPropertyNode& prop) = 0;
      virtual void write(SGPropertyNode& prop, double t) = 0;
  };

  class NumericInterpolator:
    public PropertyInterpolator
  {
    protected:
      double _end,
             _diff;

      virtual void setTarget(const SGPropertyNode& target);
      virtual void init(const SGPropertyNode& prop);
      virtual void write(SGPropertyNode& prop, double t);
  };

} // namespace simgear


#endif /* SG_PROPERTY_INTERPOLATOR_HXX_ */
