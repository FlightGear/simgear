/* -*-c++-*-
 *
 * Copyright (C) 2007 Tim Moore
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef VECTORARRAYADAPTERHXX
#define VECTORARRAYADAPTERHXX 1

// #define SG_CHECK_VECTOR_ACCESS 1
namespace simgear
{
template <typename Vector>
class VectorArrayAdapter {
public:
    /*
     * Adapter that provides 2D array access to the elements of a
     * std::vector in row major order.
     * @param v the vector
     * @param rowStride distance from an element to the corresponding
     * 	element in the next row.
     * @param baseOffset offset of the first element of the array from the
     * 	beginning of the vector.
     * @param rowOffset offset of the first element in a row from the
     * 	actual beginning of the row.
     */
    VectorArrayAdapter(Vector& v, int rowStride, int baseOffset = 0,
        int rowOffset = 0):
        _v(v),  _rowStride(rowStride), _baseOffset(baseOffset),
        _rowOffset(rowOffset)
    {
    }
#ifdef SG_CHECK_VECTOR_ACCESS
    typename Vector::value_type& operator() (int i, int j)
    {
        return _v.at(_baseOffset + i * _rowStride + _rowOffset + j);
    }
    const typename Vector::value_type& operator() (int i, int j) const
    {
        return _v.at(_baseOffset + i * _rowStride + _rowOffset + j);
    }
#else
    typename Vector::value_type& operator() (int i, int j)
    {
        return _v[_baseOffset + i * _rowStride + _rowOffset + j];
    }
    const typename Vector::value_type& operator() (int i, int j) const
    {
        return _v[_baseOffset + i * _rowStride + _rowOffset + j];
    }
#endif
private:
    Vector& _v;
    const int _rowStride;
    const int _baseOffset;
    const int _rowOffset;
};
}
#endif
