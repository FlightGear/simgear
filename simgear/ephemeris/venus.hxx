// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#ifndef _VENUS_HXX_
#define _VENUS_HXX_

#include <simgear/ephemeris/celestialBody.hxx>
#include <simgear/ephemeris/star.hxx>

class Venus : public CelestialBody
{
public:
  Venus (double mjd);
  Venus ();
  void updatePosition(double mjd, Star *ourSun);
};

#endif // _VENUS_HXX_
