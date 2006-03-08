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

#include <cstdlib>
#include <iostream>

#include <plib/sg.h>

#include "SGMath.hxx"

template<typename T>
bool
Vec3Test(void)
{
  SGVec3<T> v1, v2, v3;

  // Check if the equivalent function works
  v1 = SGVec3<T>(1, 2, 3);
  v2 = SGVec3<T>(3, 2, 1);
  if (equivalent(v1, v2))
    return false;

  // Check the unary minus operator
  v3 = SGVec3<T>(-1, -2, -3);
  if (!equivalent(-v1, v3))
    return false;

  // Check the unary plus operator
  v3 = SGVec3<T>(1, 2, 3);
  if (!equivalent(+v1, v3))
    return false;

  // Check the addition operator
  v3 = SGVec3<T>(4, 4, 4);
  if (!equivalent(v1 + v2, v3))
    return false;

  // Check the subtraction operator
  v3 = SGVec3<T>(-2, 0, 2);
  if (!equivalent(v1 - v2, v3))
    return false;

  // Check the scaler multiplication operator
  v3 = SGVec3<T>(2, 4, 6);
  if (!equivalent(2*v1, v3))
    return false;

  // Check the dot product
  if (fabs(dot(v1, v2) - 10) > 10*SGLimits<T>::epsilon())
    return false;

  // Check the cross product
  v3 = SGVec3<T>(-4, 8, -4);
  if (!equivalent(cross(v1, v2), v3))
    return false;

  // Check the euclidean length
  if (fabs(14 - length(v1)*length(v1)) > 14*SGLimits<T>::epsilon())
    return false;

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
    return false;
  
  // Check a rotation around the x axis
  q1 = SGQuat<T>::fromAngleAxis(0.5*SGMisc<T>::pi(), e1);
  v2 = SGVec3<T>(1, 3, -2);
  if (!equivalent(q1.transform(v1), v2))
    return false;

  // Check a rotation around the y axis
  q1 = SGQuat<T>::fromAngleAxis(SGMisc<T>::pi(), e2);
  v2 = SGVec3<T>(-1, 2, -3);
  if (!equivalent(q1.transform(v1), v2))
    return false;
  
  // Check a rotation around the y axis
  q1 = SGQuat<T>::fromAngleAxis(0.5*SGMisc<T>::pi(), e2);
  v2 = SGVec3<T>(-3, 2, 1);
  if (!equivalent(q1.transform(v1), v2))
    return false;

  // Check a rotation around the z axis
  q1 = SGQuat<T>::fromAngleAxis(SGMisc<T>::pi(), e3);
  v2 = SGVec3<T>(-1, -2, 3);
  if (!equivalent(q1.transform(v1), v2))
    return false;

  // Check a rotation around the z axis
  q1 = SGQuat<T>::fromAngleAxis(0.5*SGMisc<T>::pi(), e3);
  v2 = SGVec3<T>(2, -1, 3);
  if (!equivalent(q1.transform(v1), v2))
    return false;

  // Now check some successive transforms
  // We can reuse the prevously tested stuff
  q1 = SGQuat<T>::fromAngleAxis(0.5*SGMisc<T>::pi(), e1);
  q2 = SGQuat<T>::fromAngleAxis(0.5*SGMisc<T>::pi(), e2);
  q3 = q1*q2;
  v2 = q2.transform(q1.transform(v1));
  if (!equivalent(q3.transform(v1), v2))
    return false;

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
    return false;

  /// Test angle axis forward and back transform
  q1 = SGQuat<T>::fromAngleAxis(0.2*SGMisc<T>::pi(), e1);
  q2 = SGQuat<T>::fromAngleAxis(0.7*SGMisc<T>::pi(), e2);
  q3 = q1*q2;
  SGVec3<T> angleAxis;
  q1.getAngleAxis(angleAxis);
  q4 = SGQuat<T>::fromAngleAxis(angleAxis);
  if (!equivalent(q1, q4))
    return false;
  q2.getAngleAxis(angleAxis);
  q4 = SGQuat<T>::fromAngleAxis(angleAxis);
  if (!equivalent(q2, q4))
    return false;
  q3.getAngleAxis(angleAxis);
  q4 = SGQuat<T>::fromAngleAxis(angleAxis);
  if (!equivalent(q3, q4))
    return false;

  return true;
}

template<typename T>
bool
MatrixTest(void)
{
  // Create some test matrix
  SGVec3<T> v0(2, 7, 17);
  SGQuat<T> q0 = SGQuat<T>::fromAngleAxis(SGMisc<T>::pi(), normalize(v0));
  SGMatrix<T> m0(q0, v0);

  // Check the tqo forms of the inverse for that kind of special matrix
  SGMatrix<T> m1, m2;
  invert(m1, m0);
  m2 = transNeg(m0);
  if (!equivalent(m1, m2))
    return false;

  // Check matrix multiplication and inversion
  if (!equivalent(m0*m1, SGMatrix<T>::unit()))
    return false;
  if (!equivalent(m1*m0, SGMatrix<T>::unit()))
    return false;
  if (!equivalent(m0*m2, SGMatrix<T>::unit()))
    return false;
  if (!equivalent(m2*m0, SGMatrix<T>::unit()))
    return false;
  
  return true;
}

bool
GeodesyTest(void)
{
  // We know that the values are on the order of 1
  double epsDeg = 10*SGLimits<double>::epsilon();
  // For the altitude values we need to tolerate relative errors in the order
  // of the radius
  double epsM = 1e6*SGLimits<double>::epsilon();

  SGVec3<double> cart0, cart1;
  SGGeod geod0, geod1;
  SGGeoc geoc0;

  // create some geodetic position
  geod0 = SGGeod::fromDegM(30, 20, 17);

  // Test the conversion routines to cartesian coordinates
  cart0 = geod0;
  geod1 = cart0;
  if (epsDeg < fabs(geod0.getLongitudeDeg() - geod1.getLongitudeDeg()) ||
      epsDeg < fabs(geod0.getLatitudeDeg() - geod1.getLatitudeDeg()) ||
      epsM < fabs(geod0.getElevationM() - geod1.getElevationM()))
    return false;

  // Test the conversion routines to radial coordinates
  geoc0 = cart0;
  cart1 = geoc0;
  if (!equivalent(cart0, cart1))
    return false;

  return true;
}


bool
sgInterfaceTest(void)
{
  SGVec3f v3f = SGVec3f::e2();
  SGVec4f v4f = SGVec4f::e2();
  SGQuatf qf = SGQuatf::fromEulerRad(1.2, 1.3, -0.4);
  SGMatrixf mf(qf, v3f);

  // Copy to and from plibs types check if result is equal,
  // test for exact equality
  SGVec3f tv3f;
  sgVec3 sv3f;
  sgCopyVec3(sv3f, v3f.sg());
  sgCopyVec3(tv3f.sg(), sv3f);
  if (tv3f != v3f)
    return false;
  
  // Copy to and from plibs types check if result is equal,
  // test for exact equality
  SGVec4f tv4f;
  sgVec4 sv4f;
  sgCopyVec4(sv4f, v4f.sg());
  sgCopyVec4(tv4f.sg(), sv4f);
  if (tv4f != v4f)
    return false;

  // Copy to and from plibs types check if result is equal,
  // test for exact equality
  SGQuatf tqf;
  sgQuat sqf;
  sgCopyQuat(sqf, qf.sg());
  sgCopyQuat(tqf.sg(), sqf);
  if (tqf != qf)
    return false;

  // Copy to and from plibs types check if result is equal,
  // test for exact equality
  SGMatrixf tmf;
  sgMat4 smf;
  sgCopyMat4(smf, mf.sg());
  sgCopyMat4(tmf.sg(), smf);
  if (tmf != mf)
    return false;

  return true;
}

bool
sgdInterfaceTest(void)
{
  SGVec3d v3d = SGVec3d::e2();
  SGVec4d v4d = SGVec4d::e2();
  SGQuatd qd = SGQuatd::fromEulerRad(1.2, 1.3, -0.4);
  SGMatrixd md(qd, v3d);

  // Copy to and from plibs types check if result is equal,
  // test for exact equality
  SGVec3d tv3d;
  sgdVec3 sv3d;
  sgdCopyVec3(sv3d, v3d.sg());
  sgdCopyVec3(tv3d.sg(), sv3d);
  if (tv3d != v3d)
    return false;
  
  // Copy to and from plibs types check if result is equal,
  // test for exact equality
  SGVec4d tv4d;
  sgdVec4 sv4d;
  sgdCopyVec4(sv4d, v4d.sg());
  sgdCopyVec4(tv4d.sg(), sv4d);
  if (tv4d != v4d)
    return false;

  // Copy to and from plibs types check if result is equal,
  // test for exact equality
  SGQuatd tqd;
  sgdQuat sqd;
  sgdCopyQuat(sqd, qd.sg());
  sgdCopyQuat(tqd.sg(), sqd);
  if (tqd != qd)
    return false;

  // Copy to and from plibs types check if result is equal,
  // test for exact equality
  SGMatrixd tmd;
  sgdMat4 smd;
  sgdCopyMat4(smd, md.sg());
  sgdCopyMat4(tmd.sg(), smd);
  if (tmd != md)
    return false;

  return true;
}

int
main(void)
{
  // Do vector tests
  if (!Vec3Test<float>())
    return EXIT_FAILURE;
  if (!Vec3Test<double>())
    return EXIT_FAILURE;

  // Do quaternion tests
  if (!QuatTest<float>())
    return EXIT_FAILURE;
  if (!QuatTest<double>())
    return EXIT_FAILURE;

  // Do matrix tests
  if (!MatrixTest<float>())
    return EXIT_FAILURE;
  if (!MatrixTest<double>())
    return EXIT_FAILURE;

  // Check geodetic/geocentric/cartesian conversions
//   if (!GeodesyTest())
//     return EXIT_FAILURE;

  // Check interaction with sg*/sgd*
  if (!sgInterfaceTest())
    return EXIT_FAILURE;
  if (!sgdInterfaceTest())
    return EXIT_FAILURE;
  
  std::cout << "Successfully passed all tests!" << std::endl;
  return EXIT_SUCCESS;
}
