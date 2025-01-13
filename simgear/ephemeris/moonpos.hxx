// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#ifndef _MOONPOS_HXX_
#define _MOONPOS_HXX_


#include <simgear/constants.h>

#include <simgear/ephemeris/celestialBody.hxx>
#include <simgear/ephemeris/star.hxx>


class MoonPos : public CelestialBody {
private:
    double xg, yg;     // the moon's rectangular geocentric coordinates
    double ye, ze;     // the moon's rectangular equatorial coordinates
    double distance;   // the moon's distance to the earth
    double distance_in_a;  // the moon's distance to the earth in unit of its semi-mayor axis a
    double phase;      // the moon's phase (angle between the Sun and the Moon as seen from the Earth) from -pi to pi in radians
    double phase_angle; // the moon's phase angle (angle between the Sun and the Earth as seen from the Moon) from -pi to pi in radians

public:
    MoonPos(double mjd);
    MoonPos();
    virtual ~MoonPos();
    void updatePositionTopo(double mjd, double lst, double lat, Star *ourSun);
    void updatePosition(double mjd, Star *ourSun);
    double getM() const;
    double getw() const;
    double getxg() const;
    double getyg() const;
    double getye() const;
    double getze() const;
    double getDistance() const;
    double getDistanceInMayorAxis() const;
    double getPhase() const;
    double getPhaseAngle() const;
};

inline double MoonPos::getM() const
{
  return M;
}

inline double MoonPos::getw() const
{
  return w;
}

inline double MoonPos::getxg() const
{
  return xg;
}

inline double MoonPos::getyg() const
{
  return yg;
}

inline double MoonPos::getye() const
{
   return ye;
}

inline double MoonPos::getze() const
{
   return ze;
}

inline double MoonPos::getDistance() const
{
  return distance;
}

inline double MoonPos::getDistanceInMayorAxis() const
{
  return distance_in_a;
}

inline double MoonPos::getPhase() const
{
  return phase;
}

inline double MoonPos::getPhaseAngle() const
{
  return phase_angle;
}

#endif // _MOONPOS_HXX_
