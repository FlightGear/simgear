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
  void setMin(const SGVec3<T>& min)
  { _min = min; }
  const SGVec3<T>& getMin() const
  { return _min; }

  void setMax(const SGVec3<T>& max)
  { _max = max; }
  const SGVec3<T>& getMax() const
  { return _max; }

  // Only works for floating point types
  SGVec3<T> getCenter() const
  { return T(0.5)*(_min + _max); }

  // Only valid for nonempty boxes
  SGVec3<T> getSize() const
  { return _max - _min; }

  T getVolume() const
  {
    if (empty())
      return 0;
    return (_max[0] - _min[0])*(_max[1] - _min[1])*(_max[2] - _min[2]);
  }

  const bool empty() const
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
    SGVec3d size = getSize();
    if (size[1] <= size[0] && size[2] <= size[0])
      return 0;
    else if (size[2] <= size[1])
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
