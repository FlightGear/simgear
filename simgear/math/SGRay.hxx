// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2006 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef SGRay_H
#define SGRay_H

template<typename T>
class SGRay {
public:
  SGRay()
  { }
  SGRay(const SGVec3<T>& origin, const SGVec3<T>& dir) :
    _origin(origin), _direction(dir)
  { }
  template<typename S>
  explicit SGRay(const SGRay<S>& ray) :
    _origin(ray.getOrigin()), _direction(ray.getDirection())
  { }

  void set(const SGVec3<T>& origin, const SGVec3<T>& dir)
  { _origin = origin; _direction = dir; }

  void setOrigin(const SGVec3<T>& origin)
  { _origin = origin; }
  const SGVec3<T>& getOrigin() const
  { return _origin; }

  void setDirection(const SGVec3<T>& direction)
  { _direction = direction; }
  const SGVec3<T>& getDirection() const
  { return _direction; }

  SGVec3<T> getNormalizedDirection() const
  { return normalize(getDirection()); }

  SGVec3<T> getClosestPointTo(const SGVec3<T>& point)
  {
      SGVec3<T> u(getNormalizedDirection()),
          v(point - _origin);
      return (dot(u, v) * u) + _origin; 
  }
private:
  SGVec3<T> _origin;
  SGVec3<T> _direction;
};

/// Output to an ostream
template<typename char_type, typename traits_type, typename T>
inline
std::basic_ostream<char_type, traits_type>&
operator<<(std::basic_ostream<char_type, traits_type>& s,
           const SGRay<T>& ray)
{
  return s << "ray: origin = " << ray.getOrigin()
           << ", direction = " << ray.getDirection();
}

#endif
