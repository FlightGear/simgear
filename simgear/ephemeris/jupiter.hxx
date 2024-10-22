// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#ifndef _JUPITER_HXX_
#define _JUPITER_HXX_

#include <simgear/ephemeris/celestialBody.hxx>
#include <simgear/ephemeris/star.hxx>

class Jupiter : public CelestialBody
{
public:
  Jupiter (double mjd);
  Jupiter ();
  void updatePosition(double mjd, Star *ourSun);
};

#endif // _JUPITER_HXX_
