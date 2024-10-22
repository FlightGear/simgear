// SPDX-License-Identifier: LGPL-2.1-or-later

/**
 * @file
 * @brief This header file contains color-related functions
 */

#ifndef _SG_COLORS_HXX
#define _SG_COLORS_HXX 1

#include <math.h>

#if defined (sgi)
const float system_gamma = 2.0/1.7;

#else	// others
const float system_gamma = 2.5;
#endif

// simple architecture independant gamma correction function.
inline void gamma_correct_rgb(float *color,
                              float reff = 2.5, float system = system_gamma)
{
    if (reff == system)
       return;

    float tmp = reff/system;
    color[0] = pow(color[0], tmp);
    color[1] = pow(color[1], tmp);
    color[2] = pow(color[2], tmp);
}

inline void gamma_correct_c(float *color,
                            float reff = 2.5, float system = system_gamma)
{
   if (reff == system)
      return;

   *color = pow(*color, reff/system);
}

inline void gamma_restore_rgb(float *color,
                              float reff = 2.5, float system = system_gamma)
{
    if (reff == system)
       return;

    float tmp = system/reff;
    color[0] = pow(color[0], tmp);
    color[1] = pow(color[1], tmp);
    color[2] = pow(color[2], tmp);
}

inline void gamma_restore_c(float *color,
                            float reff = 2.5, float system = system_gamma)
{
   if (reff == system)
      return;

   *color = pow(*color, system/reff);
}


#endif // _SG_COLORS_HXX

