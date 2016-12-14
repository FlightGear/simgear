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

template<typename T, int N> class simd4x4_t;

namespace simd4x4
{

template<typename T, int N>
inline void zeros(simd4x4_t<T,N>& r) {
    std::memset(r, 0, sizeof(T[N][N]));
}

template<typename T, int N>
inline void unit(simd4x4_t<T,N>& r) {
    zeros(r);
    for (int i=0; i<N; i++) {
        r.ptr()[i][i] = T(1);
    }
}

template<typename T>
inline simd4x4_t<T,4> rotation_matrix(T angle, const simd4_t<T,3>& axis)
{
    T s = std::sin(angle), c = std::cos(angle), t = T(1)-c;
    simd4_t<T,3> at = axis*t, as = axis*s;
    simd4x4_t<T,4> m;

    simd4x4::unit(m);
    for (int i=0; i<3; i++) {
        simd4_t<T,4> r = axis.ptr()[i]*at;
        for (int j=0; j<4; j++) {
            m.m4x4()[0][j] = r.v4()[j];
        }
    }

    m.ptr()[0][0] += c;
    m.ptr()[0][1] += as.ptr()[2];
    m.ptr()[0][2] -= as.ptr()[1];

    m.ptr()[1][0] -= as.ptr()[2];
    m.ptr()[1][1] += c;
    m.ptr()[1][2] += as.ptr()[0];

    m.ptr()[2][0] += as.ptr()[1];
    m.ptr()[2][1] -= as.ptr()[0];
    m.ptr()[2][2] += c;

    return m;
}

template<typename T, int N>
inline void rotate(simd4x4_t<T,N>& mtx, T angle, const simd4_t<T,3>& axis) {
    if (std::abs(angle) > std::numeric_limits<T>::epsilon()) {
        mtx *= rotation_matrix(angle, axis);
    }
}

template<typename T, int N>
inline simd4x4_t<T,N> transpose(simd4x4_t<T,N> mtx) {
    simd4x4_t<T,N> m;
    for (int i=0; i<N; i++) {
        for(int j=0; j<N; j++) {
            m.ptr()[j][i] = mtx.ptr()[i][j];
        }
    }
    return m;
}

} /* namespace simd4x4 */

template<typename T, int N>
class simd4x4_t
{
private:
    union {
        T _m4x4[4][4];
        T mtx[N][N];
        T array[N*N];
    };

public:
    simd4x4_t(void) {}
    simd4x4_t(const T m[N*N]) {
        std::memcpy(array, m, sizeof(T[N*N]));
    }
    simd4x4_t(const T m[N][N]) {
        std::memcpy(array, m, sizeof(T[N*N]));
    }
    simd4x4_t(const simd4x4_t<T,N>& m) {
        std::memcpy(array, m, sizeof(T[N*N]));
    }
    ~simd4x4_t(void) {}

    inline T (&m4x4(void))[4][4] {
        return _m4x4;
    }

    inline const T (&m4x4(void) const)[4][4] {
        return _m4x4;
    }

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
    inline simd4x4_t<T,N>& operator=(const T m[N][N]) {
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

    inline simd4x4_t<T,N> operator*(T s) {
        simd4x4_t<T,N> r(*this);
        r *= s;
        return r;
    }
    inline simd4x4_t<T,N> operator*(simd4x4_t<T,N> m) {
        m *= *this; return m;
    }

    inline simd4x4_t<T,N> operator/(T s) {
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

    inline simd4x4_t<T,N>& operator/=(T s) {
        return operator*=(1/T(s));
    }
};

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

    union ALIGN16 {
        __m128 simd4x4[4];
        __mtx4f_t mtx;
        float array[4*4];
    } ALIGN16C;

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

    simd4x4_t<float,4>& operator*=(const simd4x4_t<float,4>& m2) {
        simd4x4_t<float,4> m1 = *this;
        simd4_t<float,4> row, col;

        for (int i=0; i<4; i++) {
            simd4_t<float,4> col(m2.ptr()[i][0]);
            row.v4() = m1.m4x4()[0] * col.v4();
            for (int j=1; j<4; j++) {
                simd4_t<float,4> col(m2.ptr()[i][j]);
                row.v4() += m1.m4x4()[j] * col.v4();
            }
            simd4x4[i] = row.v4();
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

namespace simd4x4
{

template<>
inline simd4x4_t<float,4> rotation_matrix<float>(float angle, const simd4_t<float,3>& axis)
{
    float s = std::sin(angle), c = std::cos(angle), t = 1.0-c;
    simd4_t<float,4> axt, at = axis*t, as = axis*s;
    simd4x4_t<float,4> m;

    simd4x4::unit(m);
    axt = axis.ptr()[0]*at;
    m.m4x4()[0] = axt.v4();

    axt = axis.ptr()[1]*at;
    m.ptr()[0][0] += c;
    m.ptr()[0][1] += as.ptr()[2];
    m.ptr()[0][2] -= as.ptr()[1];

    m.m4x4()[1] = axt.v4();

    axt = axis.ptr()[2]*at;
    m.ptr()[1][0] -= as.ptr()[2];
    m.ptr()[1][1] += c;
    m.ptr()[1][2] += as.ptr()[0];

    m.m4x4()[2] = axt.v4();

    m.ptr()[2][0] += as.ptr()[1];
    m.ptr()[2][1] -= as.ptr()[0];
    m.ptr()[2][2] += c;

    return m;
}

template<>
inline simd4x4_t<float,4> transpose<float>(simd4x4_t<float,4> m) {
    _MM_TRANSPOSE4_PS(m.m4x4()[0], m.m4x4()[1], m.m4x4()[2], m.m4x4()[3]);
    return m;
}

} /* namespace simd4x */

# endif


# ifdef __SSE2__
#  include <emmintrin.h>

template<>
class simd4x4_t<double,4>
{
private:
    typedef double  __mtx4d_t[4][4];

    union ALIGN16 {
        __m128d simd4x4[4][2];
        __mtx4d_t mtx;
        double array[4*4];
    } ALIGN16C;

public:
    simd4x4_t(void) {}
    explicit simd4x4_t(const double m[4*4]) {
        const double *p = m;
        for (int i=0; i<4; i++) {
            simd4_t<double,4> vec4(p);
            simd4x4[i][0] = vec4.v4()[0]; p += 4;
            simd4x4[i][1] = vec4.v4()[1];
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

    inline simd4x4_t<double,4>& operator=(const double m[4*4]) {
        const double *p = m;
        for (int i=0; i<4; i++) {
            simd4_t<double,4> vec4(p);
            simd4x4[i][0] = vec4.v4()[0]; p += 4;
            simd4x4[i][1] = vec4.v4()[1];
        }
        return *this;
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

    simd4x4_t<double,4>& operator*=(const simd4x4_t<double,4>& m2) {
        simd4x4_t<double,4> m1 = *this;
        simd4_t<double,4> row, col;

        for (int i=0; i<4; i++ ) {
            simd4_t<double,4> col = m1.m4x4()[0];
            row = col * m2.ptr()[i][0];
            for (int j=1; j<4; j++) {
                col = m1.m4x4()[j];
                row += col * m2.ptr()[i][j];
            }
            simd4x4[i][0] = row.v4()[0];
            simd4x4[i][1] = row.v4()[1];
        }
        return *this;
    }

};


template<>
inline simd4_t<double,4> operator*(const simd4x4_t<double,4>& m, const simd4_t<double,4>& vi)
{
    simd4_t<double,4> mv(m);
    mv *= vi.ptr()[0];
    for (int i=1; i<4; i+=2) {
        simd4_t<double,4> row = m.m4x4()[i];
        row *= vi.ptr()[i];
        mv.v4()[0] += row.v4()[0];
        mv.v4()[1] += row.v4()[1];
    }
    return mv;
}

namespace simd4x4
{

template<>
inline simd4x4_t<double,4> rotation_matrix<double>(double angle, const simd4_t<double,3>& axis)
{
    double s = std::sin(angle), c = std::cos(angle), t = 1.0-c;
    simd4_t<double,4> axt, at = axis*t, as = axis*s;
    simd4x4_t<double,4> m;

    simd4x4::unit(m);
    axt = axis.ptr()[0]*at;
    m.m4x4()[0][0] = axt.v4()[0];
    m.m4x4()[0][1] = axt.v4()[1];

    axt = axis.ptr()[1]*at;
    m.ptr()[0][0] += c;
    m.ptr()[0][1] += as.ptr()[2];
    m.ptr()[0][2] -= as.ptr()[1];

    m.m4x4()[1][0] = axt.v4()[0];
    m.m4x4()[1][1] = axt.v4()[1];

    axt = axis.ptr()[2]*at;
    m.ptr()[1][0] -= as.ptr()[2];
    m.ptr()[1][1] += c;
    m.ptr()[1][2] += as.ptr()[0];

    m.m4x4()[2][0] = axt.v4()[0];
    m.m4x4()[2][1] = axt.v4()[1];

    m.ptr()[2][0] += as.ptr()[1];
    m.ptr()[2][1] -= as.ptr()[0];
    m.ptr()[2][2] += c;

    return m;
}

template<>
inline simd4x4_t<double,4> transpose<double>(simd4x4_t<double,4> m) {
    simd4x4_t<double,4> mtx;
    __m128d T0 = _mm_unpacklo_pd(m.m4x4()[0][0], m.m4x4()[0][1]);
    __m128d T1 = _mm_unpacklo_pd(m.m4x4()[1][0], m.m4x4()[1][1]);
    __m128d T2 = _mm_unpackhi_pd(m.m4x4()[0][0], m.m4x4()[0][1]);
    __m128d T3 = _mm_unpackhi_pd(m.m4x4()[1][0], m.m4x4()[1][1]);
    mtx.m4x4()[0][0] = _mm_unpacklo_pd(T0, T1);
    mtx.m4x4()[1][0] = _mm_unpacklo_pd(T2, T3);
    mtx.m4x4()[2][0] = _mm_unpackhi_pd(T0, T1);
    mtx.m4x4()[3][0] = _mm_unpackhi_pd(T2, T3);

    T0 = _mm_unpacklo_pd(m.m4x4()[2][0], m.m4x4()[2][1]);
    T1 = _mm_unpacklo_pd(m.m4x4()[3][0], m.m4x4()[3][1]);
    T2 = _mm_unpackhi_pd(m.m4x4()[2][0], m.m4x4()[2][1]);
    T3 = _mm_unpackhi_pd(m.m4x4()[3][0], m.m4x4()[3][1]);
    mtx.m4x4()[0][1] = _mm_unpacklo_pd(T0, T1);
    mtx.m4x4()[1][1] = _mm_unpacklo_pd(T2, T3);
    mtx.m4x4()[2][1] = _mm_unpackhi_pd(T0, T1);
    mtx.m4x4()[3][1] = _mm_unpackhi_pd(T2, T3);
    return mtx;
}


} /* namespace simd4x4 */

# endif


# ifdef __SSE2__
#  include <xmmintrin.h>

template<>
class simd4x4_t<int,4>
{
private:
    typedef int  __mtx4i_t[4][4];

    union ALIGN16 {
        __m128i simd4x4[4];
        __mtx4i_t mtx;
        int array[4*4];
    } ALIGN16C;

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

    simd4x4_t<int,4>& operator*=(const simd4x4_t<int,4>& m2) {
        simd4x4_t<int,4> m1 = *this;
        simd4_t<int,4> row, col;

        for (int i=0; i<4; i++) {
            simd4_t<int,4> col(m2.ptr()[i][0]);
            row.v4() = m1.m4x4()[0] * col.v4();
            for (int j=1; j<4; j++) {
                simd4_t<int,4> col(m2.ptr()[i][j]);
                row.v4() += m1.m4x4()[j] * col.v4();
            }
            simd4x4[i] = row.v4();
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

namespace simd4x4
{

template<>
inline simd4x4_t<int,4> transpose<int>(simd4x4_t<int,4> m) {
    __m128i T0 = _mm_unpacklo_epi32(m.m4x4()[0], m.m4x4()[1]);
    __m128i T1 = _mm_unpacklo_epi32(m.m4x4()[2], m.m4x4()[3]);
    __m128i T2 = _mm_unpackhi_epi32(m.m4x4()[0], m.m4x4()[1]);
    __m128i T3 = _mm_unpackhi_epi32(m.m4x4()[2], m.m4x4()[3]);

    m.m4x4()[0] = _mm_unpacklo_epi64(T0, T1);
    m.m4x4()[1] = _mm_unpackhi_epi64(T0, T1);
    m.m4x4()[2] = _mm_unpacklo_epi64(T2, T3);
    m.m4x4()[3] = _mm_unpackhi_epi64(T2, T3);

    return m;
}

} /* namespace simd4x */
# endif

#endif /* __SIMD4X4_H__ */

