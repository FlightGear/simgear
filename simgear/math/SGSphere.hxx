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
  SGSphere() :
    _radius(-1)
  { }
  SGSphere(const SGVec3<T>& center, const T& radius) :
    _center(center),
    _radius(radius)
  { }

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

  const bool empty() const
  { return !valid(); }

  bool valid() const
  { return 0 <= _radius; }

  void clear()
  { _radius = -1; }

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
