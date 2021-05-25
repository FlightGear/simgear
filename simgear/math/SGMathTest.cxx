// Copyright (C) 2006  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/misc/test_macros.hxx>

#include <cstdlib>
#include <iostream>

#include "SGMath.hxx"
#include "SGRect.hxx"
#include "sg_random.hxx"

int lineno = 0;


template<typename T>
bool
Vec3Test(void)
{
  SGVec3<T> v1, v2, v3;

  // Check if the equivalent function works
  v1 = SGVec3<T>(1, 2, 3);
  v2 = SGVec3<T>(3, 2, 1);
  if (equivalent(v1, v2))
    { lineno = __LINE__; return false; }

  // Check the unary minus operator
  v3 = SGVec3<T>(-1, -2, -3);
  if (!equivalent(-v1, v3))
    { lineno = __LINE__; return false; }

  // Check the unary plus operator
  v3 = SGVec3<T>(1, 2, 3);
  if (!equivalent(+v1, v3))
    { lineno = __LINE__; return false; }

  // Check the addition operator
  v3 = SGVec3<T>(4, 4, 4);
  if (!equivalent(v1 + v2, v3))
    { lineno = __LINE__; return false; }

  // Check the subtraction operator
  v3 = SGVec3<T>(-2, 0, 2);
  if (!equivalent(v1 - v2, v3))
    { lineno = __LINE__; return false; }

  // Check the scaler multiplication operator
  v3 = SGVec3<T>(2, 4, 6);
  if (!equivalent(2*v1, v3))
    { lineno = __LINE__; return false; }

  // Check the dot product
  if (fabs(dot(v1, v2) - 10) > 10*SGLimits<T>::epsilon())
    { lineno = __LINE__; return false; }

  // Check the cross product
  v3 = SGVec3<T>(-4, 8, -4);
  if (!equivalent(cross(v1, v2), v3))
    { lineno = __LINE__; return false; }

  // Check the euclidean length
  if (fabs(14 - length(v1)*length(v1)) > 14*SGLimits<T>::epsilon())
    { lineno = __LINE__; return false; }

  return true;
}

template<typename T>
bool
isSameRotation(const SGQuat<T>& q1, const SGQuat<T>& q2)
{
  const SGVec3<T> e1(1, 0, 0);
  const SGVec3<T> e2(0, 1, 0);
  const SGVec3<T> e3(0, 0, 1);
  if (!equivalent(q1.transform(e1), q2.transform(e1)))
    { lineno = __LINE__; return false; }
  if (!equivalent(q1.transform(e2), q2.transform(e2)))
    { lineno = __LINE__; return false; }
  if (!equivalent(q1.transform(e3), q2.transform(e3)))
    { lineno = __LINE__; return false; }
  return true;
}

template<typename T>
bool
QuatTest(void)
{
  const SGVec3<T> e1(1, 0, 0);
  const SGVec3<T> e2(0, 1, 0);
  const SGVec3<T> e3(0, 0, 1);
  SGVec3<T> v1, v2;
  SGQuat<T> q1, q2, q3, q4;
  // Check a rotation around the x axis
  q1 = SGQuat<T>::fromAngleAxis(SGMisc<T>::pi(), e1);
  v1 = SGVec3<T>(1, 2, 3);
  v2 = SGVec3<T>(1, -2, -3);
  if (!equivalent(q1.transform(v1), v2))
    { lineno = __LINE__; return false; }

  // Check a rotation around the x axis
  q1 = SGQuat<T>::fromAngleAxis(0.5*SGMisc<T>::pi(), e1);
  v2 = SGVec3<T>(1, 3, -2);
  if (!equivalent(q1.transform(v1), v2))
    { lineno = __LINE__; return false; }

  // Check a rotation around the y axis
  q1 = SGQuat<T>::fromAngleAxis(SGMisc<T>::pi(), e2);
  v2 = SGVec3<T>(-1, 2, -3);
  if (!equivalent(q1.transform(v1), v2))
    { lineno = __LINE__; return false; }

  // Check a rotation around the y axis
  q1 = SGQuat<T>::fromAngleAxis(0.5*SGMisc<T>::pi(), e2);
  v2 = SGVec3<T>(-3, 2, 1);
  if (!equivalent(q1.transform(v1), v2))
    { lineno = __LINE__; return false; }

  // Check a rotation around the z axis
  q1 = SGQuat<T>::fromAngleAxis(SGMisc<T>::pi(), e3);
  v2 = SGVec3<T>(-1, -2, 3);
  if (!equivalent(q1.transform(v1), v2))
    { lineno = __LINE__; return false; }

  // Check a rotation around the z axis
  q1 = SGQuat<T>::fromAngleAxis(0.5*SGMisc<T>::pi(), e3);
  v2 = SGVec3<T>(2, -1, 3);
  if (!equivalent(q1.transform(v1), v2))
    { lineno = __LINE__; return false; }

  // Now check some successive transforms
  // We can reuse the prevously tested stuff
  q1 = SGQuat<T>::fromAngleAxis(0.5*SGMisc<T>::pi(), e1);
  q2 = SGQuat<T>::fromAngleAxis(0.5*SGMisc<T>::pi(), e2);
  q3 = q1*q2;
  v2 = q2.transform(q1.transform(v1));
  if (!equivalent(q3.transform(v1), v2))
    { lineno = __LINE__; return false; }

  /// Test from Euler angles
  float x = 0.2*SGMisc<T>::pi();
  float y = 0.3*SGMisc<T>::pi();
  float z = 0.4*SGMisc<T>::pi();
  q1 = SGQuat<T>::fromAngleAxis(z, e3);
  q2 = SGQuat<T>::fromAngleAxis(y, e2);
  q3 = SGQuat<T>::fromAngleAxis(x, e1);
  v2 = q3.transform(q2.transform(q1.transform(v1)));
  q4 = SGQuat<T>::fromEulerRad(z, y, x);
  if (!equivalent(q4.transform(v1), v2))
    { lineno = __LINE__; return false; }

  /// Test angle axis forward and back transform
  q1 = SGQuat<T>::fromAngleAxis(0.2*SGMisc<T>::pi(), e1);
  q2 = SGQuat<T>::fromAngleAxis(0.7*SGMisc<T>::pi(), e2);
  q3 = q1*q2;
  SGVec3<T> angleAxis;
  q1.getAngleAxis(angleAxis);
  q4 = SGQuat<T>::fromAngleAxis(angleAxis);
  if (!isSameRotation(q1, q4))
    { lineno = __LINE__; return false; }
  q2.getAngleAxis(angleAxis);
  q4 = SGQuat<T>::fromAngleAxis(angleAxis);
  if (!isSameRotation(q2, q4))
    { lineno = __LINE__; return false; }
  q3.getAngleAxis(angleAxis);
  q4 = SGQuat<T>::fromAngleAxis(angleAxis);
  if (!isSameRotation(q3, q4))
    { lineno = __LINE__; return false; }

  /// Test angle axis forward and back transform
  q1 = SGQuat<T>::fromAngleAxis(0.2*SGMisc<T>::pi(), e1);
  q2 = SGQuat<T>::fromAngleAxis(1.7*SGMisc<T>::pi(), e2);
  q3 = q1*q2;
  SGVec3<T> positiveAngleAxis = q1.getPositiveRealImag();
  q4 = SGQuat<T>::fromPositiveRealImag(positiveAngleAxis);
  if (!isSameRotation(q1, q4))
    { lineno = __LINE__; return false; }
  positiveAngleAxis = q2.getPositiveRealImag();
  q4 = SGQuat<T>::fromPositiveRealImag(positiveAngleAxis);
  if (!isSameRotation(q2, q4))
    { lineno = __LINE__; return false; }
  positiveAngleAxis = q3.getPositiveRealImag();
  q4 = SGQuat<T>::fromPositiveRealImag(positiveAngleAxis);
  if (!isSameRotation(q3, q4))
    { lineno = __LINE__; return false; }

  return true;
}

template<typename T>
bool
QuatDerivativeTest(void)
{
  for (unsigned i = 0; i < 100; ++i) {
    // Generate the test case:
    // Give a lower bound to the distance, so avoid testing cancelation
    T dt = T(0.01) + sg_random();
    // Start with orientation o0, angular velocity av and a random stepsize
    SGQuat<T> o0 = SGQuat<T>::fromEulerDeg(T(360)*sg_random(), T(360)*sg_random(), T(360)*sg_random());
    SGVec3<T> av(sg_random(), sg_random(), sg_random());
    // Do one euler step and renormalize
    SGQuat<T> o1 = normalize(o0 + dt*o0.derivative(av));

    // Check if we can restore the angular velocity
    SGVec3<T> av2 = SGQuat<T>::forwardDifferenceVelocity(o0, o1, dt);
    if (!equivalent(av, av2))
      { lineno = __LINE__; return false; }

    // Test with the equivalent orientation
    o1 = -o1;
    av2 = SGQuat<T>::forwardDifferenceVelocity(o0, o1, dt);
    if (!equivalent(av, av2))
      { lineno = __LINE__; return false; }
  }
  return true;
}

template<typename T>
bool
MatrixTest(void)
{
  // Create some test matrix
  SGVec3<T> v0(2, 7, 17);
  SGQuat<T> q0 = SGQuat<T>::fromAngleAxis(SGMisc<T>::pi(), normalize(v0));
  SGMatrix<T> m0 = SGMatrix<T>::unit();
  m0.postMultTranslate(v0);
  m0.postMultRotate(q0);

  // Check the three forms of the inverse for that kind of special matrix
  SGMatrix<T> m1 = SGMatrix<T>::unit();
  m1.preMultTranslate(-v0);
  m1.preMultRotate(inverse(q0));

  SGMatrix<T> m2, m3;
  invert(m2, m0);
  m3 = transNeg(m0);
  if (!equivalent(m1, m2))
    { lineno = __LINE__; return false; }
  if (!equivalent(m2, m3))
    { lineno = __LINE__; return false; }

  // Check matrix multiplication and inversion
  if (!equivalent(m0*m1, SGMatrix<T>::unit()))
    { lineno = __LINE__; return false; }
  if (!equivalent(m1*m0, SGMatrix<T>::unit()))
    { lineno = __LINE__; return false; }
  if (!equivalent(m0*m2, SGMatrix<T>::unit()))
    { lineno = __LINE__; return false; }
  if (!equivalent(m2*m0, SGMatrix<T>::unit()))
    { lineno = __LINE__; return false; }
  if (!equivalent(m0*m3, SGMatrix<T>::unit()))
    { lineno = __LINE__; return false; }
  if (!equivalent(m3*m0, SGMatrix<T>::unit()))
    { lineno = __LINE__; return false; }

  return true;
}

template<typename T>
void doRectTest()
{
  SGRect<T> rect(10, 15, 20, 25);

  SG_CHECK_EQUAL(rect.x(), 10)
  SG_CHECK_EQUAL(rect.y(), 15)
  SG_CHECK_EQUAL(rect.width(), 20)
  SG_CHECK_EQUAL(rect.height(), 25)

  SG_CHECK_EQUAL(rect.pos(), SGVec2<T>(10, 15))
  SG_CHECK_EQUAL(rect.size(), SGVec2<T>(20, 25))

  SG_CHECK_EQUAL(rect.l(), 10)
  SG_CHECK_EQUAL(rect.t(), 15)
  SG_CHECK_EQUAL(rect.r(), 30)
  SG_CHECK_EQUAL(rect.b(), 40)

  SG_VERIFY(rect == rect)
  SG_VERIFY(rect == SGRect<T>(10, 15, 20, 25))
  SG_VERIFY(rect != SGRect<T>(11, 15, 20, 25))

  SG_VERIFY(rect.contains(10, 15))
  SG_VERIFY(!rect.contains(9, 15))
  SG_VERIFY(rect.contains(9, 15, 1))
}

bool
GeodesyTest(void)
{
  // We know that the values are on the order of 1
  double epsDeg = 10*360*SGLimits<double>::epsilon();
  // For the altitude values we need to tolerate relative errors in the order
  // of the radius
  double epsM = 10*6e6*SGLimits<double>::epsilon();

  SGVec3<double> cart0, cart1;
  SGGeod geod0, geod1;
  SGGeoc geoc0;

  // create some geodetic position
  geod0 = SGGeod::fromDegM(30, 20, 17);

  // Test the conversion routines to cartesian coordinates
  cart0 = SGVec3<double>::fromGeod(geod0);
  geod1 = SGGeod::fromCart(cart0);
  if (epsDeg < fabs(geod0.getLongitudeDeg() - geod1.getLongitudeDeg()) ||
      epsDeg < fabs(geod0.getLatitudeDeg() - geod1.getLatitudeDeg()) ||
      epsM < fabs(geod0.getElevationM() - geod1.getElevationM()))
    { lineno = __LINE__; return false; }

  // Test the conversion routines to radial coordinates
  geoc0 = SGGeoc::fromCart(cart0);
  cart1 = SGVec3<double>::fromGeoc(geoc0);
  if (!equivalent(cart0, cart1))
    { lineno = __LINE__; return false; }

  // test course / advance routines
  // uses examples from Williams aviation formulary
  SGGeoc lax = SGGeoc::fromRadM(-2.066470, 0.592539, 10.0);
  SGGeoc jfk = SGGeoc::fromRadM(-1.287762, 0.709186, 10.0);

  double distNm = SGGeodesy::distanceRad(lax, jfk) * SG_RAD_TO_NM;
  std::cout << "distance is " << distNm << std::endl;
  if (0.5 < fabs(distNm - 2144)) // 2144 nm
	{ lineno = __LINE__; return false; }

  double crsDeg = SGGeodesy::courseRad(lax, jfk) * SG_RADIANS_TO_DEGREES;
  std::cout << "course is " << crsDeg << std::endl;
  if (0.5 < fabs(crsDeg - 66)) // 66 degrees
	{ lineno = __LINE__; return false; }

  SGGeoc adv;
  SGGeodesy::advanceRadM(lax, crsDeg * SG_DEGREES_TO_RADIANS, 100 * SG_NM_TO_METER, adv);
  std::cout << "lon:" << adv.getLongitudeRad() << ", lat:" << adv.getLatitudeRad() << std::endl;

  if (0.01 < fabs(adv.getLongitudeRad() - (-2.034206)) ||
	  0.01 < fabs(adv.getLatitudeRad() - 0.604180))
	{ lineno = __LINE__; return false; }

  return true;
}

int
main(void)
{
  sg_srandom(17);

  // Do vector tests
  if (!Vec3Test<float>())
    { fprintf(stderr, "Error at line: %i called from line: %i\n", lineno, __LINE__); return EXIT_FAILURE; }
  if (!Vec3Test<double>())
    { fprintf(stderr, "Error at line: %i called from line: %i\n", lineno, __LINE__); return EXIT_FAILURE; }

  // Do quaternion tests
  if (!QuatTest<float>())
    { fprintf(stderr, "Error at line: %i called from line: %i\n", lineno, __LINE__); return EXIT_FAILURE; }
  if (!QuatTest<double>())
    { fprintf(stderr, "Error at line: %i called from line: %i\n", lineno, __LINE__); return EXIT_FAILURE; }
  if (!QuatDerivativeTest<float>())
    { fprintf(stderr, "Error at line: %i called from line: %i\n", lineno, __LINE__); return EXIT_FAILURE; }
  if (!QuatDerivativeTest<double>())
    { fprintf(stderr, "Error at line: %i called from line: %i\n", lineno, __LINE__); return EXIT_FAILURE; }

  // Do matrix tests
  if (!MatrixTest<float>())
    { fprintf(stderr, "Error at line: %i called from line: %i\n", lineno, __LINE__); return EXIT_FAILURE; }
  if (!MatrixTest<double>())
    { fprintf(stderr, "Error at line: %i called from line: %i\n", lineno, __LINE__); return EXIT_FAILURE; }

  // Do rect tests
  doRectTest<int>();
  doRectTest<double>();

  // Check geodetic/geocentric/cartesian conversions
  if (!GeodesyTest())
    { fprintf(stderr, "Error at line: %i called from line: %i\n", lineno, __LINE__); return EXIT_FAILURE; }

  std::cout << "Successfully passed all tests!" << std::endl;
  return EXIT_SUCCESS;
}
