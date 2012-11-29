// colours.h 
//
// -- This header file contains color related functions
//
// Copyright (C) 2003  FlightGear Flight Simulator
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


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

