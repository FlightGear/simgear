// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#ifndef _SATURN_HXX_
#define _SATURN_HXX_

#include <simgear/ephemeris/celestialBody.hxx>
#include <simgear/ephemeris/star.hxx>

class Saturn : public CelestialBody
{
public:
  Saturn (double mjd);
  Saturn ();
  void updatePosition(double mjd, Star *ourSun);
};

#endif // _SATURN_HXX_







