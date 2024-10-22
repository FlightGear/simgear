// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#ifndef _NEPTUNE_HXX_
#define _NEPTUNE_HXX_

#include <simgear/ephemeris/celestialBody.hxx>
#include <simgear/ephemeris/star.hxx>

class Neptune : public CelestialBody
{
public:
  Neptune (double mjd);
  Neptune ();
  void updatePosition(double mjd, Star *ourSun);
};

#endif // _NEPTUNE_HXX_
