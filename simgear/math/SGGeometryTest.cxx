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

#include "SGGeometry.hxx"
#include "sg_random.hxx"

template<typename T>
SGVec3<T> rndVec3(void)
{
  return SGVec3<T>(sg_random(), sg_random(), sg_random());
}

template<typename T>
bool
TriangleClosestPointIntersectionTest(void)
{
  unsigned nTests = 100000;
  unsigned failedCount = 0;
  for (unsigned i = 0; i < nTests; ++i) {
      // test triangle
      SGTriangle<T> triangle(rndVec3<T>(), rndVec3<T>(), rndVec3<T>());
      T triangleEps = 100*SGLimits<T>::epsilon();

      // generate random point in the triangle
      T u = 4*sg_random() - 2;
      T v = 4*sg_random() - 2;
      if (1 < u + v) {
        v = T(0.5)*(v + 1 - u);
        u = 1 - v;
      }
      u = SGMisc<T>::clip(u, 0, 1);
      v = SGMisc<T>::clip(v, 0, 1);

      SGVec3<T> testClosest;
      testClosest = triangle.getBaseVertex();
      testClosest += u*triangle.getEdge(0) + v*triangle.getEdge(1);

      SGVec3<T> n = triangle.getNormal();
      SGVec3<T> e0 = triangle.getEdge(0);
      SGVec3<T> e1 = triangle.getEdge(1);

      // The normals to the edges
      SGVec3<T> a0 = cross(e0, n);
      SGVec3<T> a1 = cross(e1 - e0, n);
      SGVec3<T> a2 = cross(n, e1);

      SGVec3<T> testPoint = testClosest;
      // Ok, if we are at some edge, go perpandicular to the triangle away
      if (u == 0) {
        if (v == 0) {
          testPoint += sg_random()*a0 + sg_random()*a2;
        } else if (v == 1) {
          testPoint += sg_random()*a1 + sg_random()*a2;
        } else {
          testPoint += sg_random()*a2;
        }
      } else if (u == 1) {
        testPoint += sg_random()*a0 + sg_random()*a1;
      } else {
        if (v == 0) {
          testPoint += sg_random()*a0;
        } else if (u + v == 1) {
          testPoint += sg_random()*a1;
        }
      }
      testPoint += (2*sg_random() - 1)*n;

      // Test the closest point function
      SGVec3<T> closest = closestPoint(triangle, testPoint);
      if (!equivalent(closest, testClosest, triangleEps)) {
        std::cout << "Failed closest point test #" << i
                  << ": not equivalent!\nu = "
                  << u << ", v = " << v
                  << "\ntestPoint = " << testPoint
                  << ", testClosest = " << testClosest
                  << ", closest = " << closest << "\n"
                  << triangle << std::endl;
        ++failedCount;
      }

      // Test the triangle sphere intersection
      SGSphere<T> sphere(testPoint, sg_random());
      bool exactIntersection = intersects(sphere, testClosest);
//       bool exactIntersection = intersects(sphere, closest);
      bool sphereTriangleIntersection = intersects(sphere, triangle);
    
      if (sphereTriangleIntersection != exactIntersection) {
        std::cout << "Failed triangle sphere intersection test #" << i
                  << ": not equivalent!\nu = "
                  << u << ", v = " << v
                  << "\ntestPoint = " << testPoint
                  << ", testClosest = " << testClosest
                  << ", closest = " << closest << "\n"
                  << triangle << std::endl;
        ++failedCount;
      }
  }

  if (nTests < 100*failedCount) {
    std::cout << "Failed box line intersection tests: " << failedCount
              << " tests out of " << nTests
              << " went wrong. Abort!" << std::endl;
    return false;
  }

  return true;
}

template<typename T>
bool
TriangleLineIntersectionTest(void)
{
  unsigned nTests = 100000;
  unsigned failedCount = 0;
  for (unsigned i = 0; i < nTests; ++i) {
    SGVec3<T> v0 = rndVec3<T>();
    SGVec3<T> v1 = rndVec3<T>();
    SGVec3<T> v2 = rndVec3<T>();
    
    SGTriangle<T> tri(v0, v1, v2);

    T triangleEps = 100*SGLimits<T>::epsilon();

    // generate random coeficients
    T u = 4*sg_random() - 2;
    T v = 4*sg_random() - 2;
    T t = 4*sg_random() - 2;

    SGVec3<T> isectpt = v0 + u*(v1 - v0) + v*(v2 - v0);

    SGLineSegment<T> lineSegment;
    SGVec3<T> dir = rndVec3<T>();

    SGVec3<T> isectres;
    lineSegment.set(isectpt - t*dir, isectpt + (1 - t)*dir);
    
    if (intersects(isectres, tri, lineSegment)) {
      if (0 <= u && 0 <= v && u+v <= 1 && 0 <= t && t <= 1) {
        if (!equivalent(isectres, isectpt, triangleEps)) {
          std::cout << "Failed line segment intersection test #" << i
                    << ": not equivalent!\nu = "
                    << u << ", v = " << v << ", t = " << t
                    << "\n" << tri << "\n" << lineSegment << std::endl;
          ++failedCount;
        }
      } else {
        std::cout << "Failed line segment intersection test #" << i
                  << ": false positive!\nu = "
                  << u << ", v = " << v << ", t = " << t
                  << "\n" << tri << "\n" << lineSegment << std::endl;
        ++failedCount;
      }
    } else {
      if (0 <= u && 0 <= v && u+v <= 1 && 0 <= t && t <= 1) {
        std::cout << "Failed line segment intersection test #" << i
                  << ": false negative!\nu = "
                  << u << ", v = " << v << ", t = " << t
                  << "\n" << tri << "\n" << lineSegment << std::endl;
        ++failedCount;
      }
    }

    SGRay<T> ray;
    ray.set(isectpt - t*dir, dir);
    if (intersects(isectres, tri, ray)) {
      if (0 <= u && 0 <= v && u+v <= 1 && 0 <= t) {
        if (!equivalent(isectres, isectpt, triangleEps)) {
          std::cout << "Failed ray intersection test #" << i
                    << ": not equivalent!\nu = "
                    << u << ", v = " << v << ", t = " << t
                    << "\n" << tri << "\n" << ray << std::endl;
          ++failedCount;
        }
      } else {
        std::cout << "Failed ray intersection test #" << i
                  << ": false positive!\nu = "
                  << u << ", v = " << v << ", t = " << t
                  << "\n" << tri << "\n" << ray << std::endl;
        ++failedCount;
      }
    } else {
      if (0 <= u && 0 <= v && u+v <= 1 && 0 <= t) {
        std::cout << "Failed ray intersection test #" << i
                  << ": false negative !\nu = "
                  << u << ", v = " << v << ", t = " << t
                  << "\n" << tri << "\n" << ray << std::endl;
        ++failedCount;
      }
    }
  }

  if (nTests < 100*failedCount) {
    std::cout << "Failed ray intersection tests: " << failedCount
              << " tests out of " << nTests
              << " went wrong. Abort!" << std::endl;
    return false;
  }

  /// Some crude handmade test
  SGVec3<T> v0 = SGVec3<T>(0, 0, 0);
  SGVec3<T> v1 = SGVec3<T>(1, 0, 0);
  SGVec3<T> v2 = SGVec3<T>(0, 1, 0);

  SGTriangle<T> tri(v0, v1, v2);

  SGRay<T> ray;
  ray.set(SGVec3<T>(0, 0, 1), SGVec3<T>(0.1, 0.1, -1));
  if (!intersects(tri, ray)) {
    std::cout << "Failed test #1!" << std::endl;
    return false;
  }

  ray.set(SGVec3<T>(0, 0, 1), SGVec3<T>(0, 0, -1));
  if (!intersects(tri, ray)) {
    std::cout << "Failed test #2!" << std::endl;
    return false;
  }

  SGLineSegment<T> lineSegment;
  lineSegment.set(SGVec3<T>(0, 0, 1), SGVec3<T>(0.1, 0.1, -1));
  if (!intersects(tri, lineSegment)) {
    std::cout << "Failed test #3!" << std::endl;
    return false;
  }

  lineSegment.set(SGVec3<T>(0, 0, 1), SGVec3<T>(0, 0, -1));
  if (!intersects(tri, lineSegment)) {
    std::cout << "Failed test #4!" << std::endl;
    return false;
  }

  lineSegment.set(SGVec3<T>(0, 0, 1), SGVec3<T>(0, 1, -1));
  if (!intersects(tri, lineSegment)) {
    std::cout << "Failed test #5!" << std::endl;
    return false;
  }

  lineSegment.set(SGVec3<T>(0, 0, 1), SGVec3<T>(1, 0, -1));
  if (!intersects(tri, lineSegment)) {
    std::cout << "Failed test #6!" << std::endl;
    return false;
  }

  lineSegment.set(SGVec3<T>(0, 0, 1), SGVec3<T>(1, 1, -1));
  if (!intersects(tri, lineSegment)) {
    std::cout << "Failed test #7!" << std::endl;
    return false;
  }

  // is exactly in the plane
  // FIXME: cannot detect that yet ??
//   lineSegment.set(SGVec3<T>(0, 0, 0), SGVec3<T>(1, 0, 0));
//   if (!intersects(tri, lineSegment)) {
//     std::cout << "Failed test #8!" << std::endl;
//     return false;
//   }

  // is exactly in the plane
  // FIXME: cannot detect that yet ??
//   lineSegment.set(SGVec3<T>(-1, 0, 0), SGVec3<T>(1, 0, 0));
//   if (!intersects(tri, lineSegment)) {
//     std::cout << "Failed test #9!" << std::endl;
//     return false;
//   }

  // is exactly paralell to the plane
  // FIXME: cannot detect that yet ??
//   lineSegment.set(SGVec3<T>(-1, 1, 0), SGVec3<T>(1, 1, 0));
//   if (intersects(tri, lineSegment)) {
//     std::cout << "Failed test #10!" << std::endl;
//     return false;
//   }

  // should fail since the line segment poins slightly beyond the triangle
  lineSegment.set(SGVec3<T>(0, 0, 1), SGVec3<T>(1, 1, -0.9));
  if (intersects(tri, lineSegment)) {
    std::cout << "Failed test #11!" << std::endl;
    return false;
  }

  lineSegment.set(SGVec3<T>(0, 0, 1), SGVec3<T>(0, -0.1, -1));
  if (intersects(tri, lineSegment)) {
    std::cout << "Failed test #12!" << std::endl;
    return false;
  }

  lineSegment.set(SGVec3<T>(0, 0, 1), SGVec3<T>(-0.1, -0.1, -1));
  if (intersects(tri, lineSegment)) {
    std::cout << "Failed test #13!" << std::endl;
    return false;
  }

  lineSegment.set(SGVec3<T>(0, 0, 1), SGVec3<T>(-0.1, 0, -1));
  if (intersects(tri, lineSegment)) {
    std::cout << "Failed test #14!" << std::endl;
    return false;
  }

  return true;
}

template<typename T>
bool
SphereLineIntersectionTest(void)
{
  unsigned nTests = 100000;
  unsigned failedCount = 0;
  for (unsigned i = 0; i < nTests; ++i) {
    SGVec3<T> center = rndVec3<T>();
    T radius = 2*sg_random();
    SGSphere<T> sphere(center, radius);

    SGVec3<T> offset = normalize(rndVec3<T>());
    T t = 4*sg_random();

    // This one is the point we use to judge if the test should fail or not
    SGVec3<T> base = center + t*offset;

    SGVec3<T> per = perpendicular(offset);
    SGVec3<T> start = base + 4*sg_random()*per;
    SGVec3<T> end = base - 4*sg_random()*per;

    SGLineSegment<T> lineSegment;
    lineSegment.set(start, end);
    if (intersects(sphere, lineSegment)) {
      if (radius < t) {
        std::cout << "Failed sphere line intersection test #" << i
                  << ": false positive!\nt = " << t << "\n"
                  << sphere << "\n" << lineSegment << std::endl;
        ++failedCount;
      }
    } else {
      if (t <= radius) {
        std::cout << "Failed sphere line intersection test #" << i
                  << ": false negative!\nt = " << t << "\n"
                  << sphere << "\n" << lineSegment << std::endl;
        ++failedCount;
      }
    }

    SGRay<T> ray;
    ray.set(start, end - start);
    if (intersects(sphere, ray)) {
      if (radius < t) {
        std::cout << "Failed sphere line intersection test #" << i
                  << ": false positive!\nt = " << t << "\n"
                  << sphere << "\n" << ray << std::endl;
        ++failedCount;
      }
    } else {
      if (t <= radius) {
        std::cout << "Failed sphere line intersection test #" << i
                  << ": false negative!\nt = " << t << "\n"
                  << sphere << "\n" << ray << std::endl;
        ++failedCount;
      }
    }
  }

  if (nTests < 100*failedCount) {
    std::cout << "Failed sphere line intersection tests: " << failedCount
              << " tests out of " << nTests
              << " went wrong. Abort!" << std::endl;
    return false;
  }

  failedCount = 0;
  for (unsigned i = 0; i < nTests; ++i) {
    SGVec3<T> center = rndVec3<T>();
    T radius = 2*sg_random();
    SGSphere<T> sphere(center, radius);

    SGVec3<T> offset = normalize(rndVec3<T>());
    T t = 4*sg_random();

    // This one is the point we use to judge if the test should fail or not
    SGVec3<T> base = center + t*offset;

    SGVec3<T> start = base;
    SGVec3<T> end = base + 2*sg_random()*offset;

    SGLineSegment<T> lineSegment;
    lineSegment.set(start, end);
    if (intersects(sphere, lineSegment)) {
      if (radius < t) {
        std::cout << "Failed sphere line intersection test #" << i
                  << ": false positive!\nt = " << t << "\n"
                  << sphere << "\n" << lineSegment << std::endl;
        ++failedCount;
      }
    } else {
      if (t <= radius) {
        std::cout << "Failed sphere line intersection test #" << i
                  << ": false negative!\nt = " << t << "\n"
                  << sphere << "\n" << lineSegment << std::endl;
        ++failedCount;
      }
    }

    SGRay<T> ray;
    ray.set(start, end - start);
    if (intersects(sphere, ray)) {
      if (radius < t) {
        std::cout << "Failed sphere line intersection test #" << i
                  << ": false positive!\nt = " << t << "\n"
                  << sphere << "\n" << ray << std::endl;
        ++failedCount;
      }
    } else {
      if (t <= radius) {
        std::cout << "Failed sphere line intersection test #" << i
                  << ": false negative!\nt = " << t << "\n"
                  << sphere << "\n" << ray << std::endl;
        ++failedCount;
      }
    }
  }

  if (nTests < 100*failedCount) {
    std::cout << "Failed sphere line intersection tests: " << failedCount
              << " tests out of " << nTests
              << " went wrong. Abort!" << std::endl;
    return false;
  }

  return true;
}

template<typename T>
bool
BoxLineIntersectionTest(void)
{
  // ok, bad test case coverage, but better than nothing ...

  unsigned nTests = 100000;
  unsigned failedCount = 0;
  for (unsigned i = 0; i < nTests; ++i) {
    SGBox<T> box;
    box.expandBy(rndVec3<T>());
    box.expandBy(rndVec3<T>());

    SGVec3<T> center = box.getCenter();
    
    // This one is the point we use to judge if the test should fail or not
    SGVec3<T> base = rndVec3<T>();
    SGVec3<T> dir = base - center;

    SGLineSegment<T> lineSegment;
    lineSegment.set(base, base + dir);
    if (intersects(box, lineSegment)) {
      if (!intersects(box, base)) {
        std::cout << "Failed box line intersection test #" << i
                  << ": false positive!\n"
                  << box << "\n" << lineSegment << std::endl;
        ++failedCount;
      }
    } else {
      if (intersects(box, base)) {
        std::cout << "Failed box line intersection test #" << i
                  << ": false negative!\n"
                  << box << "\n" << lineSegment << std::endl;
        ++failedCount;
      }
    }

    SGRay<T> ray;
    ray.set(base, dir);
    if (intersects(box, ray)) {
      if (!intersects(box, base)) {
        std::cout << "Failed box line intersection test #" << i
                  << ": false positive!\n"
                  << box << "\n" << ray << std::endl;
        ++failedCount;
      }
    } else {
      if (intersects(box, base)) {
        std::cout << "Failed box line intersection test #" << i
                  << ": false negative!\n"
                  << box << "\n" << ray << std::endl;
        ++failedCount;
      }
    }
  }

  if (nTests < 100*failedCount) {
    std::cout << "Failed box line intersection tests: " << failedCount
              << " tests out of " << nTests
              << " went wrong. Abort!" << std::endl;
    return false;
  }

  return true;
}

int
main(void)
{
  std::cout << "Testing Geometry intersection routines.\n"
            << "Some of these tests can fail due to roundoff problems...\n"
            << "Dont worry if only a few of them fail..." << std::endl;

  sg_srandom(17);

  if (!TriangleClosestPointIntersectionTest<float>())
    return EXIT_FAILURE;
  if (!TriangleClosestPointIntersectionTest<double>())
    return EXIT_FAILURE;

  if (!TriangleLineIntersectionTest<float>())
    return EXIT_FAILURE;
  if (!TriangleLineIntersectionTest<double>())
    return EXIT_FAILURE;

  if (!SphereLineIntersectionTest<float>())
    return EXIT_FAILURE;
  if (!SphereLineIntersectionTest<double>())
    return EXIT_FAILURE;
  
  if (!BoxLineIntersectionTest<float>())
    return EXIT_FAILURE;
  if (!BoxLineIntersectionTest<double>())
    return EXIT_FAILURE;
  
  std::cout << "Successfully passed all tests!" << std::endl;
  return EXIT_SUCCESS;
}
