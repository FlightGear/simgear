// Copyright (C) 2006-2009  Mathias Froehlich - Mathias.Froehlich@web.de
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

#ifndef SGIntersect_HXX
#define SGIntersect_HXX

#include <algorithm>

template<typename T>
inline bool
intersects(const SGSphere<T>& s1, const SGSphere<T>& s2)
{
  if (s1.empty())
    return false;
  if (s2.empty())
    return false;

  T dist = s1.getRadius() + s2.getRadius();
  return distSqr(s1.getCenter(), s2.getCenter()) <= dist*dist;
}


template<typename T1, typename T2>
inline bool
intersects(const SGBox<T1>& box, const SGSphere<T2>& sphere)
{
  if (sphere.empty())
    return false;
  if (box.empty())
    return false;

  SGVec3<T1> closest = box.getClosestPoint(sphere.getCenter());
  return distSqr(closest, SGVec3<T1>(sphere.getCenter())) <= sphere.getRadius2();
}
// make it symmetric
template<typename T1, typename T2>
inline bool
intersects(const SGSphere<T1>& sphere, const SGBox<T2>& box)
{ return intersects(box, sphere); }


template<typename T1, typename T2>
inline bool
intersects(const SGVec3<T1>& v, const SGBox<T2>& box)
{
  if (v[0] < box.getMin()[0])
    return false;
  if (box.getMax()[0] < v[0])
    return false;
  if (v[1] < box.getMin()[1])
    return false;
  if (box.getMax()[1] < v[1])
    return false;
  if (v[2] < box.getMin()[2])
    return false;
  if (box.getMax()[2] < v[2])
    return false;
  return true;
}
template<typename T1, typename T2>
inline bool
intersects(const SGBox<T1>& box, const SGVec3<T2>& v)
{ return intersects(v, box); }


template<typename T>
inline bool
intersects(const SGRay<T>& ray, const SGPlane<T>& plane)
{
  // We compute the intersection point
  //   x = origin + \alpha*direction
  // from the ray origin and non nomalized direction.
  // For 0 <= \alpha the ray intersects the infinite plane.
  // The intersection point x can also be written
  //   x = n*dist + y
  // where n is the planes normal, dist is the distance of the plane from
  // the origin in normal direction and y is ana aproriate vector
  // perpendicular to n.
  // Equate the x values and take the scalar product with the plane normal n.
  //   dot(n, origin) + \alpha*dot(n, direction) = dist
  // We can now compute alpha from the above equation.
  //   \alpha = (dist - dot(n, origin))/dot(n, direction)

  // The negative numerator for the \alpha expression
  T num = plane.getPositiveDist();
  num -= dot(plane.getNormal(), ray.getOrigin());
  
  // If the numerator is zero, we have the rays origin included in the plane
  if (fabs(num) <= SGLimits<T>::min())
    return true;

  // The denominator for the \alpha expression
  T den = dot(plane.getNormal(), ray.getDirection());

  // If we get here, we already know that the rays origin is not included
  // in the plane. Thus if we have a zero denominator we have
  // a ray paralell to the plane. That is no intersection.
  if (fabs(den) <= SGLimits<T>::min())
    return false;

  // We would now compute \alpha = num/den and compare with 0 and 1.
  // But to avoid that expensive division, check equation multiplied by
  // the denominator.
  T alphaDen = copysign(1, den)*num;
  if (alphaDen < 0)
    return false;

  return true;
}
// make it symmetric
template<typename T>
inline bool
intersects(const SGPlane<T>& plane, const SGRay<T>& ray)
{ return intersects(ray, plane); }

template<typename T>
inline bool
intersects(SGVec3<T>& dst, const SGRay<T>& ray, const SGPlane<T>& plane)
{
  // We compute the intersection point
  //   x = origin + \alpha*direction
  // from the ray origin and non nomalized direction.
  // For 0 <= \alpha the ray intersects the infinite plane.
  // The intersection point x can also be written
  //   x = n*dist + y
  // where n is the planes normal, dist is the distance of the plane from
  // the origin in normal direction and y is ana aproriate vector
  // perpendicular to n.
  // Equate the x values and take the scalar product with the plane normal n.
  //   dot(n, origin) + \alpha*dot(n, direction) = dist
  // We can now compute alpha from the above equation.
  //   \alpha = (dist - dot(n, origin))/dot(n, direction)

  // The negative numerator for the \alpha expression
  T num = plane.getPositiveDist();
  num -= dot(plane.getNormal(), ray.getOrigin());
  
  // If the numerator is zero, we have the rays origin included in the plane
  if (fabs(num) <= SGLimits<T>::min()) {
    dst = ray.getOrigin();
    return true;
  }

  // The denominator for the \alpha expression
  T den = dot(plane.getNormal(), ray.getDirection());

  // If we get here, we already know that the rays origin is not included
  // in the plane. Thus if we have a zero denominator we have
  // a ray paralell to the plane. That is no intersection.
  if (fabs(den) <= SGLimits<T>::min())
    return false;

  // We would now compute \alpha = num/den and compare with 0 and 1.
  // But to avoid that expensive division, check equation multiplied by
  // the denominator.
  T alpha = num/den;
  if (alpha < 0)
    return false;

  dst = ray.getOrigin() + alpha*ray.getDirection();
  return true;
}
// make it symmetric
template<typename T>
inline bool
intersects(SGVec3<T>& dst, const SGPlane<T>& plane, const SGRay<T>& ray)
{ return intersects(dst, ray, plane); }

template<typename T>
inline bool
intersects(const SGLineSegment<T>& lineSegment, const SGPlane<T>& plane)
{
  // We compute the intersection point
  //   x = origin + \alpha*direction
  // from the line segments origin and non nomalized direction.
  // For 0 <= \alpha <= 1 the line segment intersects the infinite plane.
  // The intersection point x can also be written
  //   x = n*dist + y
  // where n is the planes normal, dist is the distance of the plane from
  // the origin in normal direction and y is ana aproriate vector
  // perpendicular to n.
  // Equate the x values and take the scalar product with the plane normal n.
  //   dot(n, origin) + \alpha*dot(n, direction) = dist
  // We can now compute alpha from the above equation.
  //   \alpha = (dist - dot(n, origin))/dot(n, direction)

  // The negative numerator for the \alpha expression
  T num = plane.getPositiveDist();
  num -= dot(plane.getNormal(), lineSegment.getOrigin());
  
  // If the numerator is zero, we have the lines origin included in the plane
  if (fabs(num) <= SGLimits<T>::min())
    return true;

  // The denominator for the \alpha expression
  T den = dot(plane.getNormal(), lineSegment.getDirection());

  // If we get here, we already know that the lines origin is not included
  // in the plane. Thus if we have a zero denominator we have
  // a line paralell to the plane. That is no intersection.
  if (fabs(den) <= SGLimits<T>::min())
    return false;

  // We would now compute \alpha = num/den and compare with 0 and 1.
  // But to avoid that expensive division, compare equations
  // multiplied by |den|. Note that copysign is usually a compiler intrinsic
  // that expands in assembler code that not even stalls the cpus pipes.
  T alphaDen = copysign(1, den)*num;
  if (alphaDen < 0)
    return false;
  if (den < alphaDen)
    return false;

  return true;
}
// make it symmetric
template<typename T>
inline bool
intersects(const SGPlane<T>& plane, const SGLineSegment<T>& lineSegment)
{ return intersects(lineSegment, plane); }

template<typename T>
inline bool
intersects(SGVec3<T>& dst, const SGLineSegment<T>& lineSegment, const SGPlane<T>& plane)
{
  // We compute the intersection point
  //   x = origin + \alpha*direction
  // from the line segments origin and non nomalized direction.
  // For 0 <= \alpha <= 1 the line segment intersects the infinite plane.
  // The intersection point x can also be written
  //   x = n*dist + y
  // where n is the planes normal, dist is the distance of the plane from
  // the origin in normal direction and y is an aproriate vector
  // perpendicular to n.
  // Equate the x values and take the scalar product with the plane normal n.
  //   dot(n, origin) + \alpha*dot(n, direction) = dist
  // We can now compute alpha from the above equation.
  //   \alpha = (dist - dot(n, origin))/dot(n, direction)

  // The negative numerator for the \alpha expression
  T num = plane.getPositiveDist();
  num -= dot(plane.getNormal(), lineSegment.getStart());
  
  // If the numerator is zero, we have the lines origin included in the plane
  if (fabs(num) <= SGLimits<T>::min()) {
    dst = lineSegment.getStart();
    return true;
  }

  // The denominator for the \alpha expression
  T den = dot(plane.getNormal(), lineSegment.getDirection());

  // If we get here, we already know that the lines origin is not included
  // in the plane. Thus if we have a zero denominator we have
  // a line paralell to the plane. That is: no intersection.
  if (fabs(den) <= SGLimits<T>::min())
    return false;

  // We would now compute \alpha = num/den and compare with 0 and 1.
  // But to avoid that expensive division, check equation multiplied by
  // the denominator. FIXME: shall we do so? or compute like that?
  T alpha = num/den;
  if (alpha < 0)
    return false;
  if (1 < alpha)
    return false;

  dst = lineSegment.getStart() + alpha*lineSegment.getDirection();
  return true;
}
// make it symmetric
template<typename T>
inline bool
intersects(SGVec3<T>& dst, const SGPlane<T>& plane, const SGLineSegment<T>& lineSegment)
{ return intersects(dst, lineSegment, plane); }


// Distance of a line segment to a point
template<typename T>
inline T
distSqr(const SGLineSegment<T>& lineSeg, const SGVec3<T>& p)
{
  SGVec3<T> ps = p - lineSeg.getStart();

  T psdotdir = dot(ps, lineSeg.getDirection());
  if (psdotdir <= 0)
    return dot(ps, ps);

  SGVec3<T> pe = p - lineSeg.getEnd();
  if (0 <= dot(pe, lineSeg.getDirection()))
    return dot(pe, pe);
 
  return dot(ps, ps) - psdotdir*psdotdir/dot(lineSeg.getDirection(), lineSeg.getDirection());
}
// make it symmetric
template<typename T>
inline T
distSqr(const SGVec3<T>& p, const SGLineSegment<T>& lineSeg)
{ return distSqr(lineSeg, p); }
// with sqrt
template<typename T>
inline T
dist(const SGVec3<T>& p, const SGLineSegment<T>& lineSeg)
{ return sqrt(distSqr(lineSeg, p)); }
template<typename T>
inline T
dist(const SGLineSegment<T>& lineSeg, const SGVec3<T>& p)
{ return sqrt(distSqr(lineSeg, p)); }

template<typename T>
inline bool
intersects(const SGRay<T>& ray, const SGSphere<T>& sphere)
{
  // See Tomas Akeniene - Moeller/Eric Haines: Real Time Rendering,
  // second edition, page 571
  SGVec3<T> l = sphere.getCenter() - ray.getOrigin();
  T s = dot(l, ray.getDirection());
  T l2 = dot(l, l);

  T r2 = sphere.getRadius2();
  if (s < 0 && l2 > r2)
    return false;

  T d2 = dot(ray.getDirection(), ray.getDirection());
  // The original test would read
  //   T m2 = l2 - s*s/d2;
  //   if (m2 > r2)
  //     return false;
  // but to avoid the expensive division, we multiply by d2
  T m2 = d2*l2 - s*s;
  if (m2 > d2*r2)
    return false;

  return true;
}
// make it symmetric
template<typename T>
inline bool
intersects(const SGSphere<T>& sphere, const SGRay<T>& ray)
{ return intersects(ray, sphere); }

template<typename T>
inline bool
intersects(const SGLineSegment<T>& lineSegment, const SGSphere<T>& sphere)
{
  // See Tomas Akeniene - Moeller/Eric Haines: Real Time Rendering,
  // second edition, page 571
  SGVec3<T> l = sphere.getCenter() - lineSegment.getStart();
  T ld = length(lineSegment.getDirection());
  T s = dot(l, lineSegment.getDirection())/ld;
  T l2 = dot(l, l);

  T r2 = sphere.getRadius2();
  if (s < 0 && l2 > r2)
    return false;

  T m2 = l2 - s*s;
  if (m2 > r2)
    return false;

  T q = sqrt(r2 - m2);
  T t = s - q;
  if (ld < t)
    return false;
  
  return true;
}
// make it symmetric
template<typename T>
inline bool
intersects(const SGSphere<T>& sphere, const SGLineSegment<T>& lineSegment)
{ return intersects(lineSegment, sphere); }


template<typename T>
inline bool
// FIXME do not use that default argument later. Just for development now
intersects(SGVec3<T>& x, const SGTriangle<T>& tri, const SGRay<T>& ray, T eps = 0)
{
  // See Tomas Akeniene - Moeller/Eric Haines: Real Time Rendering

  // Method based on the observation that we are looking for a
  // point x that can be expressed in terms of the triangle points
  //  x = v_0 + u*(v_1 - v_0) + v*(v_2 - v_0)
  // with 0 <= u, v and u + v <= 1.
  // OTOH it could be expressed in terms of the ray
  //  x = o + t*d
  // Now we can compute u, v and t.
  SGVec3<T> p = cross(ray.getDirection(), tri.getEdge(1));

  T denom = dot(p, tri.getEdge(0));
  T signDenom = copysign(1, denom);

  SGVec3<T> s = ray.getOrigin() - tri.getBaseVertex();
  SGVec3<T> q = cross(s, tri.getEdge(0));
  // Now t would read
  //   t = 1/denom*dot(q, tri.getEdge(1));
  // To avoid an expensive division we multiply by |denom|
  T tDenom = signDenom*dot(q, tri.getEdge(1));
  if (tDenom < 0)
    return false;
  // For line segment we would test against
  // if (1 < t)
  //   return false;
  // with the original t. The multiplied test would read
  // if (absDenom < tDenom)
  //   return false;
  
  T absDenom = fabs(denom);
  T absDenomEps = absDenom*eps;

  // T u = 1/denom*dot(p, s);
  T u = signDenom*dot(p, s);
  if (u < -absDenomEps)
    return false;
  // T v = 1/denom*dot(q, d);
  // if (v < -eps)
  //   return false;
  T v = signDenom*dot(q, ray.getDirection());
  if (v < -absDenomEps)
    return false;
  
  if (u + v > absDenom + absDenomEps)
    return false;
  
  // return if paralell ??? FIXME what if paralell and in plane?
  // may be we are ok below than anyway??
  if (absDenom <= SGLimits<T>::min())
    return false;

  x = ray.getOrigin();
  // if we have survived here it could only happen with denom == 0
  // that the point is already in plane. Then return the origin ...
  if (SGLimitsd::min() < absDenom)
    x += (tDenom/absDenom)*ray.getDirection();
  
  return true;
}

template<typename T>
inline bool
intersects(const SGTriangle<T>& tri, const SGRay<T>& ray, T eps = 0)
{
  // FIXME: for now just wrap the other method. When that has prooven
  // well optimized, implement that special case
  SGVec3<T> dummy;
  return intersects(dummy, tri, ray, eps);
}

template<typename T>
inline bool
// FIXME do not use that default argument later. Just for development now
intersects(SGVec3<T>& x, const SGTriangle<T>& tri, const SGLineSegment<T>& lineSegment, T eps = 0)
{
  // See Tomas Akeniene - Moeller/Eric Haines: Real Time Rendering

  // Method based on the observation that we are looking for a
  // point x that can be expressed in terms of the triangle points
  //  x = v_0 + u*(v_1 - v_0) + v*(v_2 - v_0)
  // with 0 <= u, v and u + v <= 1.
  // OTOH it could be expressed in terms of the lineSegment
  //  x = o + t*d
  // Now we can compute u, v and t.
  SGVec3<T> p = cross(lineSegment.getDirection(), tri.getEdge(1));

  T denom = dot(p, tri.getEdge(0));
  T signDenom = copysign(1, denom);

  SGVec3<T> s = lineSegment.getStart() - tri.getBaseVertex();
  SGVec3<T> q = cross(s, tri.getEdge(0));
  // Now t would read
  //   t = 1/denom*dot(q, tri.getEdge(1));
  // To avoid an expensive division we multiply by |denom|
  T tDenom = signDenom*dot(q, tri.getEdge(1));
  if (tDenom < 0)
    return false;
  // For line segment we would test against
  // if (1 < t)
  //   return false;
  // with the original t. The multiplied test reads
  T absDenom = fabs(denom);
  if (absDenom < tDenom)
    return false;
  
  // take the CPU accuracy in account
  T absDenomEps = absDenom*eps;

  // T u = 1/denom*dot(p, s);
  T u = signDenom*dot(p, s);
  if (u < -absDenomEps)
    return false;
  // T v = 1/denom*dot(q, d);
  // if (v < -eps)
  //   return false;
  T v = signDenom*dot(q, lineSegment.getDirection());
  if (v < -absDenomEps)
    return false;
  
  if (u + v > absDenom + absDenomEps)
    return false;
  
  // return if paralell ??? FIXME what if paralell and in plane?
  // may be we are ok below than anyway??
  if (absDenom <= SGLimits<T>::min())
    return false;

  x = lineSegment.getStart();
  // if we have survived here it could only happen with denom == 0
  // that the point is already in plane. Then return the origin ...
  if (SGLimitsd::min() < absDenom)
    x += (tDenom/absDenom)*lineSegment.getDirection();
  
  return true;
}

template<typename T>
inline bool
intersects(const SGTriangle<T>& tri, const SGLineSegment<T>& lineSegment, T eps = 0)
{
  // FIXME: for now just wrap the other method. When that has prooven
  // well optimized, implement that special case
  SGVec3<T> dummy;
  return intersects(dummy, tri, lineSegment, eps);
}


template<typename T>
inline SGVec3<T>
closestPoint(const SGTriangle<T>& tri, const SGVec3<T>& p)
{
  // This method minimizes the distance function Q(u, v) = || p - x ||
  // where x is a point in the trialgle given by the vertices v_i
  //  x = v_0 + u*(v_1 - v_0) + v*(v_2 - v_0)
  // The theoretical analysis is somehow too long for a comment.
  // May be it is sufficient to see that this code passes all the tests.

  SGVec3<T> off = tri.getBaseVertex() - p;
  T a = dot(tri.getEdge(0), tri.getEdge(0));
  T b = dot(tri.getEdge(0), tri.getEdge(1));
  T c = dot(tri.getEdge(1), tri.getEdge(1));
  T d = dot(tri.getEdge(0), off);
  T e = dot(tri.getEdge(1), off);
  
  T det = a*c - b*b;

  T u = b*e - c*d;
  T v = b*d - a*e;

/*
  // Regions
  // \2|
  //  \|
  //   |\ 
  // 3 |0\ 1
  //----------
  // 4 | 5 \ 6
*/

  if (u + v <= det) {
    if (u < 0) {
      if (v < 0) {
        // region 4
        if (d < 0) {
          if (a <= -d) {
            // u = 1;
            // v = 0;
            return tri.getBaseVertex() + tri.getEdge(0);
          } else {
            u = -d/a;
            // v = 0;
            return tri.getBaseVertex() + u*tri.getEdge(0);
          }
        } else {
          if (0 < e) {
            // u = 0;
            // v = 0;
            return tri.getBaseVertex();
          } else if (c <= -e) {
            // u = 0;
            // v = 1;
            return tri.getBaseVertex() + tri.getEdge(1);
          } else {
            // u = 0;
            v = -e/c;
            return tri.getBaseVertex() + v*tri.getEdge(1);
          }
        }
      } else {
        // region 3
        if (0 <= e) {
          // u = 0;
          // v = 0;
          return tri.getBaseVertex();
        } else if (c <= -e) {
          // u = 0;
          // v = 1;
          return tri.getBaseVertex() + tri.getEdge(1);
        } else {
          // u = 0;
          v = -e/c;
          return tri.getBaseVertex() + v*tri.getEdge(1);
        }
      }
    } else if (v < 0) {
      // region 5
      if (0 <= d) {
        // u = 0;
        // v = 0;
        return tri.getBaseVertex();
      } else if (a <= -d) {
        // u = 1;
        // v = 0;
        return tri.getBaseVertex() + tri.getEdge(0);
      } else {
        u = -d/a;
        // v = 0;
        return tri.getBaseVertex() + u*tri.getEdge(0);
      }
    } else {
      // region 0
      if (det <= SGLimits<T>::min()) {
        u = 0;
        v = 0;
        return tri.getBaseVertex();
      } else {
        T invDet = 1/det;
        u *= invDet;
        v *= invDet;
        return tri.getBaseVertex() + u*tri.getEdge(0) + v*tri.getEdge(1);
      }
    }
  } else {
    if (u < 0) {
      // region 2
      T tmp0 = b + d;
      T tmp1 = c + e;
      if (tmp0 < tmp1) {
        T numer = tmp1 - tmp0;
        T denom = a - 2*b + c;
        if (denom <= numer) {
          // u = 1;
          // v = 0;
          return tri.getBaseVertex() + tri.getEdge(0);
        } else {
          u = numer/denom;
          v = 1 - u;
          return tri.getBaseVertex() + u*tri.getEdge(0) + v*tri.getEdge(1);
        }
      } else {
        if (tmp1 <= 0) {
          // u = 0;
          // v = 1;
          return tri.getBaseVertex() + tri.getEdge(1);
        } else if (0 <= e) {
          // u = 0;
          // v = 0;
          return tri.getBaseVertex();
        } else {
          // u = 0;
          v = -e/c;
          return tri.getBaseVertex() + v*tri.getEdge(1);
        }
      }
    } else if (v < 0) {
      // region 6
      T tmp0 = b + e;
      T tmp1 = a + d;
      if (tmp0 < tmp1) {
        T numer = tmp1 - tmp0;
        T denom = a - 2*b + c;
        if (denom <= numer) {
          // u = 0;
          // v = 1;
          return tri.getBaseVertex() + tri.getEdge(1);
        } else {
          v = numer/denom;
          u = 1 - v;
          return tri.getBaseVertex() + u*tri.getEdge(0) + v*tri.getEdge(1);
        }
      } else {
        if (tmp1 < 0) {
          // u = 1;
          // v = 0;
          return tri.getBaseVertex() + tri.getEdge(0);
        } else if (0 <= d) {
          // u = 0;
          // v = 0;
          return tri.getBaseVertex();
        } else {
          u = -d/a;
          // v = 0;
          return tri.getBaseVertex() + u*tri.getEdge(0);
        }
      }
    } else {
      // region 1
      T numer = c + e - b - d;
      if (numer <= 0) {
        // u = 0;
        // v = 1;
        return tri.getVertex(2);
      } else {
        T denom = a - 2*b + c;
        if (denom <= numer) {
          // u = 1;
          // v = 0;
          return tri.getBaseVertex() + tri.getEdge(0);
        } else {
          u = numer/denom;
          v = 1 - u;
          return tri.getBaseVertex() + u*tri.getEdge(0) + v*tri.getEdge(1);
        }
      }
    }
  }
}
template<typename T>
inline SGVec3<T>
closestPoint(const SGVec3<T>& p, const SGTriangle<T>& tri)
{ return closestPoint(tri, p); }

template<typename T, typename T2>
inline bool
intersects(const SGTriangle<T>& tri, const SGSphere<T2>& sphere)
{
  // This method minimizes the distance function Q(u, v) = || p - x ||
  // where x is a point in the trialgle given by the vertices v_i
  //  x = v_0 + u*(v_1 - v_0) + v*(v_2 - v_0)
  // The theoretical analysis is somehow too long for a comment.
  // May be it is sufficient to see that this code passes all the tests.

  SGVec3<T> off = tri.getBaseVertex() - SGVec3<T>(sphere.getCenter());
  T baseDist2 = dot(off, off);
  T a = dot(tri.getEdge(0), tri.getEdge(0));
  T b = dot(tri.getEdge(0), tri.getEdge(1));
  T c = dot(tri.getEdge(1), tri.getEdge(1));
  T d = dot(tri.getEdge(0), off);
  T e = dot(tri.getEdge(1), off);
  
  T det = a*c - b*b;

  T u = b*e - c*d;
  T v = b*d - a*e;

/*
  // Regions
  // \2|
  //  \|
  //   |\ 
  // 3 |0\ 1
  //----------
  // 4 | 5 \ 6
*/

  if (u + v <= det) {
    if (u < 0) {
      if (v < 0) {
        // region 4
        if (d < 0) {
          if (a <= -d) {
            // u = 1;
            // v = 0;
            T sqrDist = a + 2*d + baseDist2;
            return sqrDist <= sphere.getRadius2();
          } else {
            u = -d/a;
            // v = 0;
            T sqrDist = d*u + baseDist2;
            return sqrDist <= sphere.getRadius2();
          }
        } else {
          if (0 < e) {
            // u = 0;
            // v = 0;
            return baseDist2 <= sphere.getRadius2();
          } else if (c <= -e) {
            // u = 0;
            // v = 1;
            T sqrDist = c + 2*e + baseDist2;
            return sqrDist <= sphere.getRadius2();
          } else {
            // u = 0;
            v = -e/c;
            T sqrDist = e*v + baseDist2;
            return sqrDist <= sphere.getRadius2();
          }
        }
      } else {
        // region 3
        if (0 <= e) {
          // u = 0;
          // v = 0;
          return baseDist2 <= sphere.getRadius2();
        } else if (c <= -e) {
          // u = 0;
          // v = 1;
          T sqrDist = c + 2*e + baseDist2;
          return sqrDist <= sphere.getRadius2();
        } else {
          // u = 0;
          v = -e/c;
          T sqrDist = e*v + baseDist2;
          return sqrDist <= sphere.getRadius2();
        }
      }
    } else if (v < 0) {
      // region 5
      if (0 <= d) {
        // u = 0;
        // v = 0;
        return baseDist2 <= sphere.getRadius2();
      } else if (a <= -d) {
        // u = 1;
        // v = 0;
        T sqrDist = a + 2*d + baseDist2;
        return sqrDist <= sphere.getRadius2();
      } else {
        u = -d/a;
        // v = 0;
        T sqrDist = d*u + baseDist2;
        return sqrDist <= sphere.getRadius2();
      }
    } else {
      // region 0
      if (det <= SGLimits<T>::min()) {
        // sqrDist = baseDist2;
        u = 0;
        v = 0;
        return baseDist2 <= sphere.getRadius2();
      } else {
        T invDet = 1/det;
        u *= invDet;
        v *= invDet;
        T sqrDist = u*(a*u + b*v + 2*d) + v*(b*u + c*v + 2*e) + baseDist2;
        return sqrDist <= sphere.getRadius2();
      }
    }
  } else {
    if (u < 0) {
      // region 2
      T tmp0 = b + d;
      T tmp1 = c + e;
      if (tmp0 < tmp1) {
        T numer = tmp1 - tmp0;
        T denom = a - 2*b + c;
        if (denom <= numer) {
          // u = 1;
          // v = 0;
          T sqrDist = a + 2*d + baseDist2;
          return sqrDist <= sphere.getRadius2();
        } else {
          u = numer/denom;
          v = 1 - u;
          T sqrDist = u*(a*u + b*v + 2*d) + v*(b*u + c*v + 2*e) + baseDist2;
          return sqrDist <= sphere.getRadius2();
        }
      } else {
        if (tmp1 <= 0) {
          // u = 0;
          // v = 1;
          T sqrDist = c + 2*e + baseDist2;
          return sqrDist <= sphere.getRadius2();
        } else if (0 <= e) {
          // u = 0;
          // v = 0;
          return baseDist2 <= sphere.getRadius2();
        } else {
          // u = 0;
          v = -e/c;
          T sqrDist = e*v + baseDist2;
          return sqrDist <= sphere.getRadius2();
        }
      }
    } else if (v < 0) {
      // region 6
      T tmp0 = b + e;
      T tmp1 = a + d;
      if (tmp0 < tmp1) {
        T numer = tmp1 - tmp0;
        T denom = a - 2*b + c;
        if (denom <= numer) {
          // u = 0;
          // v = 1;
          T sqrDist = c + 2*e + baseDist2;
          return sqrDist <= sphere.getRadius2();
        } else {
          v = numer/denom;
          u = 1 - v;
          T sqrDist = u*(a*u + b*v + 2*d) + v*(b*u + c*v + 2*e)+baseDist2;
          return sqrDist <= sphere.getRadius2();
        }
      } else {
        if (tmp1 < 0) {
          // u = 1;
          // v = 0;
          T sqrDist = a + 2*d + baseDist2;
          return sqrDist <= sphere.getRadius2();
        } else if (0 <= d) {
          // sqrDist = baseDist2;
          // u = 0;
          // v = 0;
          return baseDist2 <= sphere.getRadius2();
        } else {
          u = -d/a;
          // v = 0;
          T sqrDist = d*u + baseDist2;
          return sqrDist <= sphere.getRadius2();
        }
      }
    } else {
      // region 1
      T numer = c + e - b - d;
      if (numer <= 0) {
        // u = 0;
        // v = 1;
        T sqrDist = c + 2*e + baseDist2;
        return sqrDist <= sphere.getRadius2();
      } else {
        T denom = a - 2*b + c;
        if (denom <= numer) {
          // u = 1;
          // v = 0;
          T sqrDist = a + 2*d + baseDist2;
          return sqrDist <= sphere.getRadius2();
        } else {
          u = numer/denom;
          v = 1 - u;
          T sqrDist = u*(a*u + b*v + 2*d) + v*(b*u + c*v + 2*e) + baseDist2;
          return sqrDist <= sphere.getRadius2();
        }
      }
    }
  }
}
template<typename T1, typename T2>
inline bool
intersects(const SGSphere<T1>& sphere, const SGTriangle<T2>& tri)
{ return intersects(tri, sphere); }


template<typename T>
inline bool
intersects(const SGVec3<T>& v, const SGSphere<T>& sphere)
{
  if (sphere.empty())
    return false;
  return distSqr(v, sphere.getCenter()) <= sphere.getRadius2();
}
template<typename T>
inline bool
intersects(const SGSphere<T>& sphere, const SGVec3<T>& v)
{ return intersects(v, sphere); }


template<typename T>
inline bool
intersects(const SGBox<T>& box, const SGLineSegment<T>& lineSegment)
{
  // See Tomas Akeniene - Moeller/Eric Haines: Real Time Rendering

  SGVec3<T> c = lineSegment.getCenter() - box.getCenter();
  SGVec3<T> w = T(0.5)*lineSegment.getDirection();
  SGVec3<T> v(fabs(w.x()), fabs(w.y()), fabs(w.z()));
  SGVec3<T> h = T(0.5)*box.getSize();

  if (fabs(c[0]) > v[0] + h[0])
    return false;
  if (fabs(c[1]) > v[1] + h[1])
    return false;
  if (fabs(c[2]) > v[2] + h[2])
    return false;

  if (fabs(c[1]*w[2] - c[2]*w[1]) > h[1]*v[2] + h[2]*v[1])
    return false;
  if (fabs(c[0]*w[2] - c[2]*w[0]) > h[0]*v[2] + h[2]*v[0])
    return false;
  if (fabs(c[0]*w[1] - c[1]*w[0]) > h[0]*v[1] + h[1]*v[0])
    return false;

  return true;
}
template<typename T>
inline bool
intersects(const SGLineSegment<T>& lineSegment, const SGBox<T>& box)
{ return intersects(box, lineSegment); }

template<typename T>
inline bool
intersects(const SGBox<T>& box, const SGRay<T>& ray)
{
  // See Tomas Akeniene - Moeller/Eric Haines: Real Time Rendering

  for (unsigned i = 0; i < 3; ++i) {
    T cMin = box.getMin()[i];
    T cMax = box.getMax()[i];

    T cOrigin = ray.getOrigin()[i];

    T cDir = ray.getDirection()[i];
    if (fabs(cDir) <= SGLimits<T>::min()) {
      if (cOrigin < cMin)
        return false;
      if (cMax < cOrigin)
        return false;
    }

    T nearr = - SGLimits<T>::max();
    T farr = SGLimits<T>::max();

    T T1 = (cMin - cOrigin) / cDir;
    T T2 = (cMax - cOrigin) / cDir;
    if (T1 > T2) std::swap (T1, T2);/* since T1 intersection with near plane */
    if (T1 > nearr) nearr = T1; /* want largest Tnear */
    if (T2 < farr) farr = T2; /* want smallest Tfarr */
    if (nearr > farr) // farr box is missed
      return false;
    if (farr < 0) // box is behind ray
      return false;
  }

  return true;
}
// make it symmetric
template<typename T>
inline bool
intersects(const SGRay<T>& ray, const SGBox<T>& box)
{ return intersects(box, ray); }

template<typename T1, typename T2>
inline bool
intersects(const SGBox<T1>& box1, const SGBox<T2>& box2)
{
  if (box2.getMax()[0] < box1.getMin()[0])
    return false;
  if (box1.getMax()[0] < box2.getMin()[0])
    return false;

  if (box2.getMax()[1] < box1.getMin()[1])
    return false;
  if (box1.getMax()[1] < box2.getMin()[1])
    return false;

  if (box2.getMax()[2] < box1.getMin()[2])
    return false;
  if (box1.getMax()[2] < box2.getMin()[2])
    return false;

  return true;
}

#endif
