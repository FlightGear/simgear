// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2006 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef SGTriangle_H
#define SGTriangle_H

template<typename T>
class SGTriangle {
public:
  SGTriangle()
  { }
  SGTriangle(const SGVec3<T>& v0, const SGVec3<T>& v1, const SGVec3<T>& v2)
  { set(v0, v1, v2); }
  SGTriangle(const SGVec3<T> v[3])
  { set(v); }

  void set(const SGVec3<T>& v0, const SGVec3<T>& v1, const SGVec3<T>& v2)
  {
    _v0 = v0;
    _d[0] = v1 - v0;
    _d[1] = v2 - v0;
  }
  void set(const SGVec3<T> v[3])
  {
    _v0 = v[0];
    _d[0] = v[1] - v[0];
    _d[1] = v[2] - v[0];
  }

  SGVec3<T> getCenter() const
  { return _v0 + T(1)/T(3)*(_d[0] + _d[1]); }

  // note that the index is unchecked
  SGVec3<T> getVertex(unsigned i) const
  {
    if (0 < i)
      return _v0 + _d[i-1];
    return _v0;
  }
  /// return the normalized surface normal
  SGVec3<T> getNormal() const
  { return normalize(cross(_d[0], _d[1])); }

  const SGVec3<T>& getBaseVertex() const
  { return _v0; }
  void setBaseVertex(const SGVec3<T>& v)
  { _v0 = v; }
  const SGVec3<T>& getEdge(unsigned i) const
  { return _d[i]; }
  void setEdge(unsigned i, const SGVec3<T>& d)
  { _d[i] = d; }

  // flip the positive side
  void flip()
  {
    SGVec3<T> tmp = _d[0];
    _d[0] = _d[1];
    _d[1] = tmp;
  }

  SGTriangle<T> transform(const SGMatrix<T>& matrix) const
  {
    SGTriangle<T> triangle;
    triangle._v0 = matrix.xformPt(_v0);
    triangle._d[0] = matrix.xformVec(_d[0]);
    triangle._d[1] = matrix.xformVec(_d[1]);
    return triangle;
  }

private:
  /// Store one vertex directly, _d is the offset of the other two
  /// vertices wrt the base vertex
  /// For fast intersection tests this format prooves usefull. For that same
  /// purpose also cache the cross product of the _d[i].
  SGVec3<T> _v0;
  SGVec3<T> _d[2];
};

/// Output to an ostream
template<typename char_type, typename traits_type, typename T>
inline
std::basic_ostream<char_type, traits_type>&
operator<<(std::basic_ostream<char_type, traits_type>& s,
           const SGTriangle<T>& triangle)
{
  return s << "triangle: v0 = " << triangle.getVertex(0)
           << ", v1 = " << triangle.getVertex(1)
           << ", v2 = " << triangle.getVertex(2);
}

#endif
