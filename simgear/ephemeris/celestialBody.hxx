// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#ifndef _CELESTIALBODY_H_
#define _CELESTIALBODY_H_

#ifndef __cplusplus
# error This library requires C++
#endif


#include <simgear/constants.h>

class Star;

class CelestialBody
{
protected:              // make the data protected, in order to give the
                        //  inherited classes direct access to the data
  double NFirst;       	/* longitude of the ascending node first part */
  double NSec;		/* longitude of the ascending node second part */
  double iFirst;       	/* inclination to the ecliptic first part */
  double iSec;		/* inclination to the ecliptic second part */
  double wFirst;       	/* first part of argument of perihelion */
  double wSec;		/* second part of argument of perihelion */
  double aFirst;       	/* semimayor axis first part*/
  double aSec;		/* semimayor axis second part */
  double eFirst;       	/* eccentricity first part */
  double eSec;		/* eccentricity second part */
  double MFirst;       	/* Mean anomaly first part */
  double MSec;		/* Mean anomaly second part */

  double N, i, w, a, e, M; /* the resulting orbital elements, obtained from the former */

  double rightAscension, declination;
  double r, R, s, FV;
  double magnitude;
  double lonEcl, latEcl;

  double sgCalcEccAnom(double M, double e);
  double sgCalcActTime(double mjd);
  void updateOrbElements(double mjd);

public:
  CelestialBody(double Nf, double Ns,
		double If, double Is,
		double wf, double ws,
		double af, double as,
		double ef, double es,
		double Mf, double Ms, double mjd);
  CelestialBody(double Nf, double Ns,
		double If, double Is,
		double wf, double ws,
		double af, double as,
		double ef, double es,
		double Mf, double Ms);
  void getPos(double *ra, double *dec);
  void getPos(double *ra, double *dec, double *magnitude);
  double getRightAscension();
  double getDeclination();
  double getMagnitude();
  double getLon() const;
  double getLat() const; 
  void updatePosition(double mjd, Star *ourSun);
};

inline double CelestialBody::getRightAscension() { return rightAscension; }
inline double CelestialBody::getDeclination() { return declination; }
inline double CelestialBody::getMagnitude() { return magnitude; }

inline double CelestialBody::getLon() const
{
  return lonEcl;
}

inline double CelestialBody::getLat() const
{
  return latEcl;
}

#endif // _CELESTIALBODY_H_

