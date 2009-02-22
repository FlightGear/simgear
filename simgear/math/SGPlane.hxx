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
