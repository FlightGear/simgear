// inlines.h -- various inline template definitions
//
// Written by Norman Vine, started June 2000.
//
// Copyright (C) 2000  Norman Vine  - nhv@cape.com
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


#ifndef _SG_INLINES_H
#define _SG_INLINES_H


template <class T>
inline const int SG_SIGN(const T x) {
    return x < T(0) ? -1 : 1;
}

template <class T>
inline const T SG_MIN2(const T a, const T b) {
    return a < b ? a : b;
}

// return the minimum of three values
template <class T>
inline const T SG_MIN3( const T a, const T b, const T c) {
    return (a < b ? SG_MIN2 (a, c) : SG_MIN2 (b, c));
}

template <class T>
inline const T SG_MAX2(const T a, const T b) {
    return  a > b ? a : b;
}

// return the maximum of three values
template <class T>
inline const T SG_MAX3 (const T a, const T b, const T c) {
    return (a > b ? SG_MAX2 (a, c) : SG_MAX2 (b, c));
}

// 
template <class T>
inline void SG_SWAP( T &a, T &b) {
    T c = a;  a = b;  b = c;
}

#endif // _SG_INLINES_H
