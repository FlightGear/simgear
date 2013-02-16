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

#ifndef SGBox_H
#define SGBox_H

template<typename T>
class SGBox {
public:
  SGBox() :
    _min(SGLimits<T>::max(), SGLimits<T>::max(), SGLimits<T>::max()),
    _max(-SGLimits<T>::max(), -SGLimits<T>::max(), -SGLimits<T>::max())
  { }
  SGBox(const SGVec3<T>& pt) :
    _min(pt),
    _max(pt)
  { }
  SGBox(const SGVec3<T>& min, const SGVec3<T>& max) :
    _min(min),
    _max(max)
  { }
  template<typename S>
  explicit SGBox(const SGBox<S>& box) :
    _min(box.getMin()),
    _max(box.getMax())
  { }

  void setMin(const SGVec3<T>& min)
  { _min = min; }
  const SGVec3<T>& getMin() const
  { return _min; }

  void setMax(const SGVec3<T>& max)
  { _max = max; }
  const SGVec3<T>& getMax() const
  { return _max; }

  SGVec3<T> getCorner(unsigned i) const
  { return SGVec3<T>((i&1) ? _min[0] : _max[0],
                     (i&2) ? _min[1] : _max[1],
                     (i&4) ? _min[2] : _max[2]); }

  template<typename S>
  SGVec3<T> getNearestCorner(const SGVec3<S>& pt) const
  {
    SGVec3<T> center = getCenter();
    return SGVec3<T>((pt[0] <= center[0]) ? _min[0] : _max[0],
                     (pt[1] <= center[1]) ? _min[1] : _max[1],
                     (pt[2] <= center[2]) ? _min[2] : _max[2]);
  }
  template<typename S>
  SGVec3<T> getFarestCorner(const SGVec3<S>& pt) const
  {
    SGVec3<T> center = getCenter();
    return SGVec3<T>((pt[0] > center[0]) ? _min[0] : _max[0],
                     (pt[1] > center[1]) ? _min[1] : _max[1],
                     (pt[2] > center[2]) ? _min[2] : _max[2]);
  }

  // return the closest point to pt still in the box
  template<typename S>
  SGVec3<T> getClosestPoint(const SGVec3<S>& pt) const
  {
    return SGVec3<T>((pt[0] < _min[0]) ? _min[0] : ((_max[0] < pt[0]) ? _max[0] : T(pt[0])),
                     (pt[1] < _min[1]) ? _min[1] : ((_max[1] < pt[1]) ? _max[1] : T(pt[1])),
                     (pt[2] < _min[2]) ? _min[2] : ((_max[2] < pt[2]) ? _max[2] : T(pt[2])));
  }

  // Only works for floating point types
  SGVec3<T> getCenter() const
  { return T(0.5)*(_min + _max); }

  // Only valid for nonempty boxes
  SGVec3<T> getSize() const
  { return _max - _min; }
  SGVec3<T> getHalfSize() const
  { return T(0.5)*getSize(); }

  T getVolume() const
  {
    if (empty())
      return 0;
    return (_max[0] - _min[0])*(_max[1] - _min[1])*(_max[2] - _min[2]);
  }

  bool empty() const
  { return !valid(); }

  bool valid() const
  {
    if (_max[0] < _min[0])
      return false;
    if (_max[1] < _min[1])
      return false;
    if (_max[2] < _min[2])
      return false;
    return true;
  }

  void clear()
  {
    _min[0] = SGLimits<T>::max();
    _min[1] = SGLimits<T>::max();
    _min[2] = SGLimits<T>::max();
    _max[0] = -SGLimits<T>::max();
    _max[1] = -SGLimits<T>::max();
    _max[2] = -SGLimits<T>::max();
  }

  void expandBy(const SGVec3<T>& v)
  { _min = min(_min, v); _max = max(_max, v); }

  void expandBy(const SGBox<T>& b)
  { _min = min(_min, b._min); _max = max(_max, b._max); }

  // Note that this only works if the box is nonmepty
  unsigned getBroadestAxis() const
  {
    SGVec3<T> size = getSize();
    if (size[1] <= size[0] && size[2] <= size[0])
      return 0;
    else if (size[2] <= size[1])
      return 1;
    else
      return 2;
  }

  // Note that this only works if the box is nonmepty
  unsigned getSmallestAxis() const
  {
    SGVec3<T> size = getSize();
    if (size[1] >= size[0] && size[2] >= size[0])
      return 0;
    else if (size[2] >= size[1])
      return 1;
    else
      return 2;
  }

private:
  SGVec3<T> _min;
  SGVec3<T> _max;
};

/// Output to an ostream
template<typename char_type, typename traits_type, typename T>
inline
std::basic_ostream<char_type, traits_type>&
operator<<(std::basic_ostream<char_type, traits_type>& s, const SGBox<T>& box)
{ return s << "min = " << box.getMin() << ", max = " << box.getMax(); }

#endif
