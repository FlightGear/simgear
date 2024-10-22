// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2006 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef SGPlane_H
#define SGPlane_H

template<typename T>
class SGPlane {
public:
  SGPlane()
  { }
  SGPlane(const SGVec3<T>& normal, const T& dist) :
    _normal(normal), _dist(dist)
  { }
  SGPlane(const SGVec3<T>& normal, const SGVec3<T>& point) :
    _normal(normal), _dist(-dot(normal, point))
  { }
  SGPlane(const SGVec3<T> vertices[3]) :
    _normal(normalize(cross(vertices[1] - vertices[0],
                            vertices[2] - vertices[0]))),
    _dist(-dot(_normal, vertices[0]))
  { }
  SGPlane(const SGVec3<T>& v0, const SGVec3<T>& v1, const SGVec3<T>& v2) :
    _normal(normalize(cross(v1 - v0, v2 - v0))),
    _dist(-dot(_normal, v0))
  { }
  template<typename S>
  explicit SGPlane(const SGPlane<S>& plane) :
    _normal(plane.getNormal()), _dist(plane.getDist())
  { }

  void setNormal(const SGVec3<T>& normal)
  { _normal = normal; }
  const SGVec3<T>& getNormal() const
  { return _normal; }

  void setDist(const T& dist)
  { _dist = dist; }
  const T& getDist() const
  { return _dist; }

  /// Return a point on the plane 
  SGVec3<T> getPointOnPlane() const
  { return -_dist*_normal; }

  /// That is the distance where we measure positive in direction of the normal
  T getPositiveDist() const
  { return -_dist; }
  /// That is the distance where we measure positive in the oposite direction
  /// of the normal.
  const T& getNegativeDist() const
  { return _dist; }

private:
  // That ordering is important because of one constructor
  SGVec3<T> _normal;
  T _dist;
};

#endif
