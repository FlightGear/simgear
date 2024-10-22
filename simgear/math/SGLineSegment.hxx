// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2006 Mathias Froehlich <mathias.froehlich@web.de>

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
