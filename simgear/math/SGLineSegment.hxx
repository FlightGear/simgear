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

#ifndef SGLineSegment_H
#define SGLineSegment_H

template<typename T>
class SGLineSegment {
public:
  SGLineSegment()
  { }
  SGLineSegment(const SGVec3<T>& start, const SGVec3<T>& end) :
    _start(start),
    _direction(end - start)
  { }
  template<typename S>
  explicit SGLineSegment(const SGLineSegment<S>& lineSegment) :
    _start(lineSegment.getStart()),
    _direction(lineSegment.getDirection())
  { }

  void set(const SGVec3<T>& start, const SGVec3<T>& end)
  { _start = start; _direction = end - start; }

  const SGVec3<T>& getStart() const
  { return _start; }
  SGVec3<T> getEnd() const
  { return _start + _direction; }
  const SGVec3<T>& getDirection() const
  { return _direction; }
  SGVec3<T> getNormalizedDirection() const
  { return normalize(getDirection()); }

  SGVec3<T> getCenter() const
  { return _start + T(0.5)*_direction; }

  SGLineSegment<T> transform(const SGMatrix<T>& matrix) const
  {
    SGLineSegment<T> lineSegment;
    lineSegment._start = matrix.xformPt(_start);
    lineSegment._direction = matrix.xformVec(_direction);
    return lineSegment;
  }

private:
  SGVec3<T> _start;
  SGVec3<T> _direction;
};

/// Output to an ostream
template<typename char_type, typename traits_type, typename T>
inline
std::basic_ostream<char_type, traits_type>&
operator<<(std::basic_ostream<char_type, traits_type>& s,
           const SGLineSegment<T>& lineSegment)
{
  return s << "line segment: start = " << lineSegment.getStart()
           << ", end = " << lineSegment.getEnd();
}

#endif
