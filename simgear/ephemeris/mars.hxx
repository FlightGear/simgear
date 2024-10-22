// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#ifndef _MARS_HXX_
#define _MARS_HXX_

#include <simgear/ephemeris/celestialBody.hxx>
#include <simgear/ephemeris/star.hxx>

class Mars : public CelestialBody
{
public:
  Mars ( double mjd );
  Mars ();
  void updatePosition(double mjd, Star *ourSun);
};

#endif // _MARS_HXX_
