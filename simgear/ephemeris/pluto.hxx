// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#ifndef _PLUTO_HXX_
#define _PLUTO_HXX_

#include "celestialBody.hxx"

class Pluto : public CelestialBody
{
public:
  Pluto (double mjd);
  Pluto ();
};

#endif // _PLUTO_HXX_
