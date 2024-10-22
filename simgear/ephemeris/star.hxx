// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#ifndef _STAR_HXX_
#define _STAR_HXX_


#include <simgear/ephemeris/celestialBody.hxx>


class Star : public CelestialBody
{

private:

    double lonEcl;     // the sun's true longitude
    double xs, ys;     // the sun's rectangular geocentric coordinates
    double ye, ze;     // the sun's rectangularequatorial rectangular geocentric coordinates
    double distance;   // the sun's distance to the earth

public:

    Star (double mjd);
    Star ();
    virtual ~Star();
    void updatePosition(double mjd);
    double getM() const;
    double getw() const;
    double getxs() const;
    double getys() const;
    double getye() const;
    double getze() const;
    double getDistance() const;
    double getlonEcl() const;
};


inline double Star::getM() const
{
  return M;
}

inline double Star::getw() const
{
  return w;
}

inline double Star::getxs() const
{
  return xs;
}

inline double Star::getys() const
{
  return ys;
}

inline double Star::getye() const
{
   return ye;
}

inline double Star::getze() const
{
   return ze;
}

inline double Star::getDistance() const
{
  return distance;
}

inline double Star::getlonEcl() const
{
  return lonEcl;
}

#endif // _STAR_HXX_














