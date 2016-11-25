// Copyright (C) 2016 Erik Hofman - erik@ehofman.com
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

#ifndef __SIMD4X4_H__
#define __SIMD4X4_H__	1

#include <cstring>

#include "simd.hxx"

template<typename T, int N>
class simd4x4_t
{
private:
    union {
        T m4x4[4][4];
        T mtx[N][N];
        T array[N*N];
    };

public:
    simd4x4_t(void) {}
    simd4x4_t(const T m[N*N]) {
        std::memcpy(array, m, sizeof(T[N*N]));
    }
    simd4x4_t(const simd4x4_t<T,N>& m) {
        std::memcpy(array, m, sizeof(T[N*N]));
    }
    ~simd4x4_t(void) {}

    inline const T (&ptr(void) const)[N][N] {
        return mtx;
    }

    inline T (&ptr(void))[N][N] {
        return mtx;
    }

    inline operator const T*(void) const {
        return array;
    }

    inline operator T*(void) {
        return array;
    }

    inline simd4x4_t<T,N>& operator=(const T m[N*N]) {
        std::memcpy(array, m, sizeof(T[N*N]));
        return *this;
    }
    inline simd4x4_t<T,N>& operator=(const simd4x4_t<T,N>& m) {
        std::memcpy(array, m, sizeof(T[N*N]));
        return *this;
    }

    inline simd4x4_t<T,N> operator+(simd4x4_t<T,N> m) {
        m += *this; return m;
    }

    inline simd4x4_t<T,N> operator-(simd4x4_t<T,N> m) {
        m -= *this; return m;
    }

    template<typename S>
    inline simd4x4_t<T,N> operator*(S s) {
        simd4x4_t<T,N> r(*this);
        r *= s;
        return r;
    }
    inline simd4x4_t<T,N> operator*(simd4x4_t<T,N> m) {
        m *= *this; return m;
    }

    template<typename S>
    inline simd4x4_t<T,N> operator/(S s) {
        simd4x4_t<T,N> r(*this);
        r *= (1/T(s));
        return r;
    }

    inline simd4x4_t<T,N>& operator+=(const simd4x4_t<T,N>& m) {
        for (int i=0; i<N*N; i++) {
           array[i] += m[i];
        }
        return *this;
    }

    inline simd4x4_t<T,N>& operator-=(const simd4x4_t<T,N>& m) {
        for (int i=0; i<N*N; i++) {
           array[i] -= m[i];
        }
        return *this;
    }

    inline simd4x4_t<T,N>& operator*=(T s) {
        for (int i=0; i<N*N; i++) {
           array[i] *= s;
        }
        return *this;
    }
    simd4x4_t<T,N>& operator*=(const simd4x4_t<T,N>& m1) {
        simd4x4_t<T,N> m2 = *this;
        simd4_t<T,N> row;
        for (int j=0; j<N; j++) {
            for (int r=0; r<N; r++) {
                row[r] = m2.ptr()[r][0];
            }
            row *= m1.ptr()[0][j];
            for (int r=0; r<N; r++) {
                mtx[r][j] = row[r];
            }
            for (int i=1; i<N; i++) {
                for (int r=0; r<N; r++) {
                    row[r] = m2.ptr()[r][i];
                }
                row *= m1.ptr()[i][j];
                for (int r=0; r<N; r++) {
                   mtx[r][j] += row[r];
                }
            }
        }
        return *this;
    }

    template<typename S>
    inline simd4x4_t<T,N>& operator/=(S s) {
        return operator*=(1/T(s));
    }
};

namespace simd4x4
{
template<typename T, int N>
void zeros(simd4x4_t<T,N>& r) {
    std::memset(r, 0, sizeof(T[N][N]));
  }

template<typename T, int N>
void unit(simd4x4_t<T,N>& r) {
     zeros(r);
     for (int i=0; i<N; i++) {
         r.ptr()[i][i] = 1;
     }
  }
}

template<typename T, int N>
inline simd4x4_t<T,N> operator-(simd4x4_t<T,N> m) {
    simd4x4_t<T,N> r; 
    simd4x4::zeros(r);
    r -= m; 
    return r;
}


template<typename T, int N>
inline simd4_t<T,N> operator*(const simd4x4_t<T,N>& m, const simd4_t<T,N>& vi)
{
    simd4_t<T,N> mv;
    simd4_t<T,N> row(m);
    mv = vi.ptr()[0] * row;
    for (int j=1; j<N; j++) {
        simd4_t<T,N> row(m[j*N]);
        mv += vi.ptr()[j] * row;
    }
    return mv;
}

template<typename T, int N>
inline simd4x4_t<T,N> operator*(const simd4x4_t<T,N>& m1, const simd4x4_t<T,N>& m2)
{
    simd4x4_t<T,N> m = m1;
    m *= m2;
    return m;
}


# ifdef __SSE__
#  include <xmmintrin.h>

template<>
class simd4x4_t<float,4>
{
private:
    typedef float  __mtx4f_t[4][4];

    union {
        __m128 simd4x4[4];
        __mtx4f_t mtx;
        float array[4*4];
    };

public:
    simd4x4_t(void) {}
    simd4x4_t(const float m[4*4]) {
        for (int i=0; i<4; i++) {
            simd4x4[i] = simd4_t<float,4>((const float*)&m[4*i]).v4();
        }
    }

    explicit simd4x4_t(const __mtx4f_t m) {
        for (int i=0; i<4; i++) {
            simd4x4[i] = simd4_t<float,4>(m[i]).v4();
        }
    }
    simd4x4_t(const simd4x4_t<float,4>& m) {
        for (int i=0; i<4; i++) {
            simd4x4[i] = m.m4x4()[i];
        }
    }
    ~simd4x4_t(void) {}

    inline __m128 (&m4x4(void))[4] {
        return simd4x4;
    }

    inline const __m128 (&m4x4(void) const)[4] {
        return simd4x4;
    }
    
    inline const float (&ptr(void) const)[4][4] {
        return mtx;
    }

    inline float (&ptr(void))[4][4] {
        return mtx;
    }

    inline operator const float*(void) const {
        return array;
    }

    inline operator float*(void) {
        return array;
    }

    inline simd4x4_t<float,4>& operator=(const __mtx4f_t m) {
        for (int i=0; i<4; i++) {
            simd4x4[i] = simd4_t<float,4>(m[i]).v4();
        }
        return *this;
    }
    inline simd4x4_t<float,4>& operator=(const simd4x4_t<float,4>& m) {
        for (int i=0; i<4; i++) {
            simd4x4[i] = m.m4x4()[i];
        }
        return *this;
    }

    inline simd4x4_t<float,4>& operator+=(const simd4x4_t<float,4>& m) {
        for (int i=0; i<4; i++) {
           simd4x4[i] += m.m4x4()[i];
        }
        return *this;
    }

    inline simd4x4_t<float,4>& operator-=(const simd4x4_t<float,4>& m) {
        for (int i=0; i<4; i++) {
           simd4x4[i] -= m.m4x4()[i];
        }
        return *this;
    }

    inline simd4x4_t<float,4>& operator*=(float f) {
        simd4_t<float,4> f4(f);
        for (int i=0; i<4; i++) {
           simd4x4[i] *= f4.v4();
        }
        return *this;
    }
};

template<>
inline simd4_t<float,4> operator*(const simd4x4_t<float,4>& m, const simd4_t<float,4>& vi)
{
    simd4_t<float,4> mv(m);
    mv *= vi.ptr()[0];
    for (int i=1; i<4; i++) {
        simd4_t<float,4> row(m.m4x4()[i]);
        row *= vi.ptr()[i];
        mv.v4() += row.v4();
    }
    return mv;
}

template<>
inline simd4x4_t<float,4> operator*(const simd4x4_t<float,4>& m1, const simd4x4_t<float,4>& m2)
{
    simd4_t<float,4> row, col;
    simd4x4_t<float,4> m;

    for (int i=0; i<4; i++) {
        simd4_t<float,4> col(m2.ptr()[i][0]);
        row.v4() = m1.m4x4()[0] * col.v4();
        for (int j=1; j<4; j++) {
            simd4_t<float,4> col(m2.ptr()[i][j]);
            row.v4() += m1.m4x4()[j] * col.v4();
        }
        m.m4x4()[i] = row.v4();
    }

    return m;
}
# endif


# ifdef __SSE2__
#  include <emmintrin.h>

template<>
class simd4x4_t<double,4>
{
private:
    typedef double  __mtx4d_t[4][4];

    union {
        __m128d simd4x4[4][2];
        __mtx4d_t mtx;
        double array[4*4];
    };

public:
    simd4x4_t(void) {}
    explicit simd4x4_t(const double m[4*4]) {
        for (int i=0; i<4; i++) {
            simd4x4[i][0] = simd4_t<double,4>((const double*)&m[4*i]).v4()[0];
            simd4x4[i][1] = simd4_t<double,4>((const double*)&m[4*i+2]).v4()[1];
        }
    }

    explicit simd4x4_t(const __mtx4d_t m) {
        for (int i=0; i<4; i++) {
            simd4x4[i][0] = simd4_t<double,4>(m[i]).v4()[0];
            simd4x4[i][1] = simd4_t<double,4>(m[i]).v4()[1];
        }
    }
    simd4x4_t(const simd4x4_t<double,4>& m) {
        for (int i=0; i<4; i++) {
            simd4x4[i][0] = m.m4x4()[i][0];
            simd4x4[i][1] = m.m4x4()[i][1];
        }
    }
    ~simd4x4_t(void) {}

    inline __m128d (&m4x4(void))[4][2] {
        return simd4x4;
    }

    inline const __m128d (&m4x4(void) const)[4][2] {
        return simd4x4;
    }

    inline const double (&ptr(void) const)[4][4] {
        return mtx;
    }

    inline double (&ptr(void))[4][4] {
        return mtx;
    }

    inline operator const double*(void) const {
        return array;
    }

    inline operator double*(void) {
        return array;
    }

    inline simd4x4_t<double,4>& operator=(const __mtx4d_t m) {
        for (int i=0; i<4; i++) {
            simd4x4[i][0] = simd4_t<double,4>(m[i]).v4()[0];
            simd4x4[i][1] = simd4_t<double,4>(m[i]).v4()[1];
        }
        return *this;
    }
    inline simd4x4_t<double,4>& operator=(const simd4x4_t<double,4>& m) {
        for (int i=0; i<4; i++) {
            simd4x4[i][0] = m.m4x4()[i][0];
            simd4x4[i][1] = m.m4x4()[i][1];
        }
        return *this;
    }

    inline simd4x4_t<double,4>& operator+=(const simd4x4_t<double,4>& m) {
        for (int i=0; i<4; i++) {
           simd4x4[i][0] += m.m4x4()[i][0];
           simd4x4[i][1] += m.m4x4()[i][1];
        }
        return *this;
    }

    inline simd4x4_t<double,4>& operator-=(const simd4x4_t<double,4>& m) {
        for (int i=0; i<4; i++) {
           simd4x4[i][0] -= m.m4x4()[i][0];
           simd4x4[i][1] -= m.m4x4()[i][1];
        }
        return *this;
    }

    inline simd4x4_t<double,4>& operator*=(double f) {
        simd4_t<double,4> f4(f);
        for (int i=0; i<4; i++) {
           simd4x4[i][0] *= f4.v4()[0];
           simd4x4[i][1] *= f4.v4()[0];
        }
        return *this;
    }
};


#if 0
template<>
inline simd4_t<double,4> operator*(const simd4x4_t<double,4>& m, const simd4_t<double,4>& vi)
{
    simd4_t<double,4> mv(m.m4x4()[0]);
    mv.v4()[0] *= vi.ptr()[0];
    mv.v4()[1] *= vi.ptr()[2];
    for (int i=1; i<4; i+=2) {
        simd4_t<double,4> row = m.m4x4()[i];
        row.v4()[0] *= vi.ptr()[i];
        row.v4()[1] *= vi.ptr()[i+2];
        mv += row;
    }
    return mv;
}
#endif

template<>
inline simd4x4_t<double,4> operator*(const simd4x4_t<double,4>& m1, const simd4x4_t<double,4>& m2)
{
    simd4_t<double,4> row, col;
    simd4x4_t<double,4> m;

    for (int i=0; i<4; i++ ) {
        simd4_t<double,4> col = m1.m4x4()[0];
        row = col * m2.ptr()[i][0];
        for (int j=1; j<4; j++) {
            col = m1.m4x4()[j];
            row += col * m2.ptr()[i][j];
        }
        m.m4x4()[i][0] = row.v4()[0];
        m.m4x4()[i][1] = row.v4()[1];
    }

    return m;
}
# endif


# ifdef __SSE2__
#  include <xmmintrin.h>

template<>
class simd4x4_t<int,4>
{
private:
    typedef int  __mtx4i_t[4][4];

    union {
        __m128i simd4x4[4];
        __mtx4i_t mtx;
        int array[4*4];
    };

public:
    simd4x4_t(void) {}
    simd4x4_t(const int m[4*4]) {
        for (int i=0; i<4; i++) {
            simd4x4[i] = simd4_t<int,4>((const int*)&m[4*i]).v4();
        }
    }

    explicit simd4x4_t(const __mtx4i_t m) {
        for (int i=0; i<4; i++) {
            simd4x4[i] = simd4_t<int,4>(m[i]).v4();
        }
    }
    simd4x4_t(const simd4x4_t<int,4>& m) {
        for (int i=0; i<4; i++) {
            simd4x4[i] = m.m4x4()[i];
        }
    }
    ~simd4x4_t(void) {}

    inline __m128i (&m4x4(void))[4] {
        return simd4x4;
    }

    inline const __m128i (&m4x4(void) const)[4] {
        return simd4x4;
    }
    
    inline const int (&ptr(void) const)[4][4] {
        return mtx;
    }

    inline int (&ptr(void))[4][4] {
        return mtx;
    }

    inline operator const int*(void) const {
        return array;
    }

    inline operator int*(void) {
        return array;
    }

    inline simd4x4_t<int,4>& operator=(const __mtx4i_t m) {
        for (int i=0; i<4; i++) {
            simd4x4[i] = simd4_t<int,4>(m[i]).v4();
        }
        return *this;
    }
    inline simd4x4_t<int,4>& operator=(const simd4x4_t<int,4>& m) {
        for (int i=0; i<4; i++) {
            simd4x4[i] = m.m4x4()[i];
        }
        return *this;
    }

    inline simd4x4_t<int,4>& operator+=(const simd4x4_t<int,4>& m) {
        for (int i=0; i<4; i++) {
           simd4x4[i] += m.m4x4()[i];
        }
        return *this;
    }

    inline simd4x4_t<int,4>& operator-=(const simd4x4_t<int,4>& m) {
        for (int i=0; i<4; i++) {
           simd4x4[i] -= m.m4x4()[i];
        }
        return *this;
    }

    inline simd4x4_t<int,4>& operator*=(int f) {
        simd4_t<int,4> f4(f);
        for (int i=0; i<4; i++) {
           simd4x4[i] *= f4.v4();
        }
        return *this;
    }
};

template<>
inline simd4_t<int,4> operator*(const simd4x4_t<int,4>& m, const simd4_t<int,4>& vi)
{
    simd4_t<int,4> mv(m);
    mv *= vi.ptr()[0];
    for (int i=1; i<4; i++) {
        simd4_t<int,4> row(m.m4x4()[i]);
        row *= vi.ptr()[i];
        mv.v4() += row.v4();
    }
    return mv;
}

template<>
inline simd4x4_t<int,4> operator*(const simd4x4_t<int,4>& m1, const simd4x4_t<int,4>& m2)
{
    simd4_t<int,4> row, col;
    simd4x4_t<int,4> m;

    for (int i=0; i<4; i++) {
        simd4_t<int,4> col(m2.ptr()[i][0]);
        row.v4() = m1.m4x4()[0] * col.v4();
        for (int j=1; j<4; j++) {
            simd4_t<int,4> col(m2.ptr()[i][j]);
            row.v4() += m1.m4x4()[j] * col.v4();
        }
        m.m4x4()[i] = row.v4();
    }

    return m;
}
# endif

#endif /* __SIMD4X4_H__ */

