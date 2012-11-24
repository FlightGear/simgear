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

#ifndef SGSphere_H
#define SGSphere_H


template<typename T>
class SGSphere {
public:

#ifdef __GNUC__
// Avoid "_center not initialized" warnings.
#   pragma GCC diagnostic ignored "-Wuninitialized"
#endif

  SGSphere() :
     /*
      * Do not initialize _center to save unneeded initialization time.
      * Fix 'may be used uninitialized' warnings locally instead
      */
//  _center(0.0, 0.0, 0.0),
    _radius(-1)
  { }
  SGSphere(const SGVec3<T>& center, const T& radius) :
    _center(center),
    _radius(radius)
  { }
  template<typename S>
  explicit SGSphere(const SGSphere<S>& sphere) :
    _center(sphere.getCenter()),
    _radius(sphere.getRadius())
  { }

#ifdef __GNUC__
  // Restore warning settings.
#   pragma GCC diagnostic warning "-Wuninitialized"
#endif

  const SGVec3<T>& getCenter() const
  { return _center; }
  void setCenter(const SGVec3<T>& center)
  { _center = center; }

  const T& getRadius() const
  { return _radius; }
  void setRadius(const T& radius)
  { _radius = radius; }
  T getRadius2() const
  { return _radius*_radius; }

  bool empty() const
  { return !valid(); }

  bool valid() const
  { return 0 <= _radius; }

  void clear()
  { _radius = -1; }

  /// Return true if this is inside sphere
  bool inside(const SGSphere<T>& sphere) const
  {
    if (empty())
      return false;
    if (sphere.empty())
      return false;

    T dist = sphere.getRadius() - getRadius();
    return distSqr(getCenter(), sphere.getCenter()) <= dist*dist;
  }

  void expandBy(const SGVec3<T>& v)
  {
    if (empty()) {
      _center = v;
      _radius = 0;
      return;
    }

    T dist2 = distSqr(_center, v);
    if (dist2 <= getRadius2())
      return;

    T dist = sqrt(dist2);
    T newRadius = T(0.5)*(_radius + dist);
    _center += ((newRadius - _radius)/dist)*(v - _center);
    _radius = newRadius;
  }

  void expandBy(const SGSphere<T>& s)
  {
    if (s.empty())
      return;

    if (empty()) {
      _center = s.getCenter();
      _radius = s.getRadius();
      return;
    }

    T dist = length(_center - s.getCenter());
    if (dist <= SGLimits<T>::min()) {
      _radius = SGMisc<T>::max(_radius, s._radius);
      return;
    }

    // already included
    if (dist + s.getRadius() <= _radius)
      return;

    // new one includes all
    if (dist + _radius <= s.getRadius()) {
      _center = s.getCenter();
      _radius = s.getRadius();
      return;
    }

    T newRadius = T(0.5)*(_radius + dist + s.getRadius());
    T ratio = (newRadius - _radius) / dist;
    _radius = newRadius;

    _center[0] += ratio*(s._center[0] - _center[0]);
    _center[1] += ratio*(s._center[1] - _center[1]);
    _center[2] += ratio*(s._center[2] - _center[2]);
  }

  void expandBy(const SGBox<T>& box)
  {
    if (box.empty())
      return;

    if (empty()) {
      _center = box.getCenter();
      _radius = T(0.5)*length(box.getSize());
      return;
    }

    SGVec3<T> boxCenter = box.getCenter();
    SGVec3<T> corner;
    for (unsigned i = 0; i < 3; ++i) {
      if (_center[i] < boxCenter[i])
        corner[i] = box.getMax()[i];
      else
        corner[i] = box.getMin()[i];
    }
    expandBy(corner);
  }

private:
  SGVec3<T> _center;
  T _radius;
};

/// Output to an ostream
template<typename char_type, typename traits_type, typename T>
inline
std::basic_ostream<char_type, traits_type>&
operator<<(std::basic_ostream<char_type, traits_type>& s,
           const SGSphere<T>& sphere)
{
  return s << "center = " << sphere.getCenter()
           << ", radius = " << sphere.getRadius();
}

#endif
