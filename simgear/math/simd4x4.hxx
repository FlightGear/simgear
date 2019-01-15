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

#ifdef HAVE_CONFIG_H
# include <simgear/simgear_config.h>
#endif

#if defined(__GNUC__) && defined(__ARM_NEON__)
# include <simgear/math/simd4x4_neon.hxx>
#endif

#include <simgear/math/simd.hxx>

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
    for (int i=0; i<N; ++i) {
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
    for (int i=0; i<3; ++i) {
        simd4_t<T,3> r = axis.ptr()[i]*at;
        for (int j=0; j<3; ++j) {
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
    for (int i=0; i<N; ++i) {
        for(int j=0; j<N; ++j) {
            m.ptr()[j][i] = mtx.ptr()[i][j];
        }
    }
    return m;
}

template<typename T>
inline void translate(simd4x4_t<T,4>& m, const simd4_t<T,3>& dist) {
    for (int i=0; i<3; ++i) {
        m.ptr()[3][i] -= dist[i];
    }
}

template<typename T, typename S>
inline void pre_translate(simd4x4_t<T,4>& m, const simd4_t<S,3>& dist)
{
    simd4_t<T,4> row3(m.ptr()[0][3],m.ptr()[1][3],m.ptr()[2][3],m.ptr()[3][3]);
    for (int i=0; i<3; ++i) {
        simd4_t<T,4> trow3 = T(dist[i])*row3;
        for (int j=0; j<4; ++j) {
            m.ptr()[j][i] += trow3[j];
        }
    }
}

template<typename T, typename S>
inline void post_translate(simd4x4_t<T,4>& m, const simd4_t<S,3>& dist)
{
    simd4_t<T,3> col3(m.ptr()[3]);
    for (int i=0; i<3; ++i) {
        simd4_t<T,3> trow3 = T(dist[i]);
        trow3 *= m.ptr()[i];
        col3 += trow3;
    }
    for (int i=0; i<3; ++i) {
        m.ptr()[3][i] = col3[i];
    }
}


template<typename T> // point transform
inline simd4_t<T,3> transform(const simd4x4_t<T,4>& mtx, const simd4_t<T,3>& pt)
{
    simd4_t<T,3> tpt(mtx.ptr()[3][0],mtx.ptr()[3][1],mtx.ptr()[3][2]);
    for (int i=0; i<3; ++i) {
        simd4_t<T,3> ptd(mtx.ptr()[i][0],mtx.ptr()[i][1],mtx.ptr()[i][2]);
        ptd *= pt[i];
        tpt += ptd;
    }
    return tpt;
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
    simd4x4_t(T m00, T m01, T m02, T m03,
              T m10, T m11, T m12, T m13,
              T m20, T m21, T m22, T m23,
              T m30, T m31, T m32, T m33)
    {
        array[0] = m00; array[1] = m10;
        array[2] = m20; array[3] = m30;
        array[4] = m01; array[5] = m11;
        array[6] = m21; array[7] = m31;
        array[8] = m02; array[9] = m12;
        array[10] = m22; array[11] = m32;
        array[12] = m03; array[13] = m13;
        array[14] = m23; array[15] = m33;
    }
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

    inline void set(int i, const simd4_t<T,N>& v) {
        std::memcpy(mtx[i], v.v4(), sizeof(T[N]));
    }

    inline simd4x4_t<T,N>& operator+=(const simd4x4_t<T,N>& m) {
        for (int i=0; i<N*N; ++i) {
           array[i] += m[i];
        }
        return *this;
    }

    inline simd4x4_t<T,N>& operator-=(const simd4x4_t<T,N>& m) {
        for (int i=0; i<N*N; ++i) {
           array[i] -= m[i];
        }
        return *this;
    }

    inline simd4x4_t<T,N>& operator*=(T s) {
        for (int i=0; i<N*N; ++i) {
           array[i] *= s;
        }
        return *this;
    }
    simd4x4_t<T,N>& operator*=(const simd4x4_t<T,N>& m1) {
        simd4x4_t<T,N> m2 = *this;
        simd4_t<T,N> row;
        for (int j=0; j<N; ++j) {
            for (int r=0; r<N; r++) {
                row[r] = m2.ptr()[r][0];
            }
            row *= m1.ptr()[0][j];
            for (int r=0; r<N; r++) {
                mtx[r][j] = row[r];
            }
            for (int i=1; i<N; ++i) {
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


template<typename T, int N, int M>
inline simd4_t<T,M> operator*(const simd4x4_t<T,N>& m, const simd4_t<T,M>& vi)
{
    simd4_t<T,M> mv;
    simd4_t<T,M> row(m.ptr()[0]);
    mv = vi.ptr()[0] * row;
    for (int j=1; j<M; ++j) {
        simd4_t<T,M> row(m.ptr()[j]);
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


#ifdef ENABLE_SIMD_CODE

# ifdef __SSE__
template<>
class alignas(16) simd4x4_t<float,4>
{
private:
    typedef float  __mtx4f_t[4][4];

    union alignas(16) {
        __m128 simd4x4[4];
        alignas(16) __mtx4f_t mtx;
        alignas(16) float array[4*4];
    };

public:
    simd4x4_t(void) {}
    simd4x4_t(float m00, float m01, float m02, float m03,
              float m10, float m11, float m12, float m13,
              float m20, float m21, float m22, float m23,
              float m30, float m31, float m32, float m33)
    {
        simd4x4[0] = _mm_set_ps(m30,m20,m10,m00);
        simd4x4[1] = _mm_set_ps(m31,m21,m11,m01);
        simd4x4[2] = _mm_set_ps(m32,m22,m12,m02);
        simd4x4[3] = _mm_set_ps(m33,m23,m13,m03);
    }
    simd4x4_t(const float m[4*4]) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = _mm_loadu_ps((const float*)&m[4*i]);
        }
    }

    simd4x4_t(const __mtx4f_t m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = _mm_loadu_ps(m[i]);
        }
    }
    simd4x4_t(const simd4x4_t<float,4>& m) {
        for (int i=0; i<4; ++i) {
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

    template<int M>
    inline void set(int i, const simd4_t<float,M>& v) {
        simd4x4[i] = v.v4();
    }

    inline simd4x4_t<float,4>& operator+=(const simd4x4_t<float,4>& m) {
        for (int i=0; i<4; ++i) {
           simd4x4[i] = _mm_add_ps(simd4x4[i], m.m4x4()[i]);
        }
        return *this;
    }

    inline simd4x4_t<float,4>& operator-=(const simd4x4_t<float,4>& m) {
        for (int i=0; i<4; ++i) {
           simd4x4[i] = _mm_sub_ps(simd4x4[i], m.m4x4()[i]);
        }
        return *this;
    }

    inline simd4x4_t<float,4>& operator*=(float f) {
        __m128 f4 = _mm_set1_ps(f);
        for (int i=0; i<4; ++i) {
           simd4x4[i] = _mm_mul_ps(simd4x4[i], f4);
        }
        return *this;
    }

    simd4x4_t<float,4>& operator*=(const simd4x4_t<float,4>& m2) {
        simd4x4_t<float,4> m1 = *this;
        __m128 row, col;
        for (int i=0; i<4; ++i) {
            col = _mm_set1_ps(m2.ptr()[i][0]);
            row = _mm_mul_ps(m1.m4x4()[0], col);
            for (int j=1; j<4; ++j) {
                col = _mm_set1_ps(m2.ptr()[i][j]);
                row = _mm_add_ps(row, _mm_mul_ps(m1.m4x4()[j], col));
            }
            simd4x4[i] = row;
        }
        return *this;
    }
};

template<int M>
inline simd4_t<float,M> operator*(const simd4x4_t<float,4>& m, const simd4_t<float,M>& vi)
{
    __m128 mv = _mm_mul_ps(m.m4x4()[0], _mm_set1_ps(vi.ptr()[0]));
    for (int i=1; i<M; ++i) {
        __m128 row = _mm_mul_ps(m.m4x4()[i], _mm_set1_ps(vi.ptr()[i]));
        mv = _mm_add_ps(mv, row);
    }
    return mv;
}

namespace simd4x4
{

template<>
inline simd4x4_t<float,4> rotation_matrix<float>(float angle, const simd4_t<float,3>& axis)
{
    float s = std::sin(angle), c = std::cos(angle), t = 1.0-c;
    simd4_t<float,3> axt, at = axis*t, as = axis*s;
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

inline void translate(simd4x4_t<float,4>& m, const simd4_t<float,3>& dist) {
    m.m4x4()[3] = _mm_sub_ps(m.m4x4()[3], dist.v4());
}

template<typename S>
inline void pre_translate(simd4x4_t<float,4>& m, const simd4_t<S,3>& dist)
{
    simd4x4_t<float,4> mt = simd4x4::transpose(m);
    __m128 row3 = mt.m4x4()[3];
    for (int i=0; i<3; ++i) {
        __m128 t = _mm_set1_ps(float(dist[i]));
        mt.m4x4()[i] = _mm_add_ps(mt.m4x4()[i], _mm_mul_ps(t, row3));
    }
    m = simd4x4::transpose(mt);
}

template<typename S>
inline void post_translate(simd4x4_t<float,4>& m, const simd4_t<S,3>& dist)
{
    __m128 col3 = m.m4x4()[3];
    for (int i=0; i<3; ++i) {
        __m128 t = _mm_set1_ps(float(dist[i]));
        col3 = _mm_add_ps(col3, _mm_mul_ps(t, m.m4x4()[i]));
    }
    m.m4x4()[3] = col3;
}

template<>
inline simd4_t<float,3> transform<float>(const simd4x4_t<float,4>& m, const simd4_t<float,3>& pt) {
    __m128 tpt = m.m4x4()[3];
    for (int i=0; i<3; ++i) {
        __m128 ptd = _mm_set1_ps(pt[i]);
        tpt = _mm_add_ps(tpt, _mm_mul_ps(ptd, m.m4x4()[i]));
    }
    return tpt;
}

} /* namespace simd4x */

# endif


# ifdef __AVX_unsupported__
template<>
class alignas(32) simd4x4_t<double,4>
{
private:
    typedef double  __mtx4d_t[4][4];

    union alignas(32) {
        __m256d simd4x4[4];
        alignas(32) __mtx4d_t mtx;
        alignas(32) double array[4*4];
    };

public:
    simd4x4_t(void) {}
    simd4x4_t(double m00, double m01, double m02, double m03,
              double m10, double m11, double m12, double m13,
              double m20, double m21, double m22, double m23,
              double m30, double m31, double m32, double m33)
    {
        simd4x4[0] = _mm256_set_pd(m30,m20,m10,m00);
        simd4x4[1] = _mm256_set_pd(m31,m21,m11,m01);
        simd4x4[2] = _mm256_set_pd(m32,m22,m12,m02);
        simd4x4[3] = _mm256_set_pd(m33,m23,m13,m03);
    }
    simd4x4_t(const double m[4*4]) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = _mm256_loadu_pd((const double*)&m[4*i]);
        }
    }

    simd4x4_t(const __mtx4d_t m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = _mm256_loadu_pd(m[i]);
        }
    }
    simd4x4_t(const simd4x4_t<double,4>& m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = m.m4x4()[i];
        }
    }
    ~simd4x4_t(void) {}

    inline __m256d (&m4x4(void))[4] {
        return simd4x4;
    }

    inline const __m256d (&m4x4(void) const)[4] {
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

    template<int M>
    inline void set(int i, const simd4_t<double,M>& v) {
        simd4x4[i] = v.v4();
    }

    inline simd4x4_t<double,4>& operator+=(const simd4x4_t<double,4>& m) {
        for (int i=0; i<4; ++i) {
           simd4x4[i] = _mm256_add_pd(simd4x4[i], m.m4x4()[i]);
        }
        return *this;
    }

    inline simd4x4_t<double,4>& operator-=(const simd4x4_t<double,4>& m) {
        for (int i=0; i<4; ++i) {
           simd4x4[i] = _mm256_sub_pd(simd4x4[i], m.m4x4()[i]);
        }
        return *this;
    }

    inline simd4x4_t<double,4>& operator*=(double f) {
        __m256d f4 = _mm256_set1_pd(f);
        for (int i=0; i<4; ++i) {
           simd4x4[i] = _mm256_mul_pd(simd4x4[i], f4);
        }
        return *this;
    }

    simd4x4_t<double,4>& operator*=(const simd4x4_t<double,4>& m2) {
        simd4x4_t<double,4> m1 = *this;
        __m256d row, col;
        for (int i=0; i<4; ++i) {
            col = _mm256_set1_pd(m2.ptr()[i][0]);
            row = _mm256_mul_pd(m1.m4x4()[0], col);
            for (int j=1; j<4; ++j) {
                col = _mm256_set1_pd(m2.ptr()[i][j]);
                row = _mm256_add_pd(row, _mm256_mul_pd(m1.m4x4()[j], col));
            }
            simd4x4[i] = row;
        }
        return *this;
    }
};

template<int M>
inline simd4_t<double,M> operator*(const simd4x4_t<double,4>& m, const simd4_t<double,M>& vi)
{
    __m256d mv = _mm256_mul_pd(m.m4x4()[0], _mm256_set1_pd(vi.ptr()[0]));
    for (int i=1; i<M; ++i) {
        __m256d row = _mm256_mul_pd(m.m4x4()[i], _mm256_set1_pd(vi.ptr()[i]));
        mv = _mm256_add_pd(mv, row);
    }
    return mv;
}

namespace simd4x4
{
template<>
inline simd4x4_t<double,4> rotation_matrix<double>(double angle, const simd4_t<double,3>& axis)
{
    double s = std::sin(angle), c = std::cos(angle), t = 1.0-c;
    simd4_t<double,3> axt, at = axis*t, as = axis*s;
    simd4x4_t<double,4> m;

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
inline simd4x4_t<double,4> transpose<double>(simd4x4_t<double,4> m) {
// http://stackoverflow.com/questions/36167517/m256d-transpose4-equivalent
    __m256d tmp0 = _mm256_shuffle_pd(m.m4x4()[0], m.m4x4()[1], 0x0);
    __m256d tmp2 = _mm256_shuffle_pd(m.m4x4()[0], m.m4x4()[1], 0xF);
    __m256d tmp1 = _mm256_shuffle_pd(m.m4x4()[2], m.m4x4()[3], 0x0);
    __m256d tmp3 = _mm256_shuffle_pd(m.m4x4()[2], m.m4x4()[3], 0xF);

    m.m4x4()[0] = _mm256_permute2f128_pd(tmp0, tmp1, 0x20);
    m.m4x4()[1] = _mm256_permute2f128_pd(tmp2, tmp3, 0x20);
    m.m4x4()[2] = _mm256_permute2f128_pd(tmp0, tmp1, 0x31);
    m.m4x4()[3] = _mm256_permute2f128_pd(tmp2, tmp3, 0x31);

    return m;
}

inline void translate(simd4x4_t<double,4>& m, const simd4_t<double,3>& dist) {
    m.m4x4()[3] = _mm256_sub_pd(m.m4x4()[3], dist.v4());
}

template<typename S>
inline void pre_translate(simd4x4_t<double,4>& m, const simd4_t<S,3>& dist)
{
    simd4_t<double,4> row3(m.ptr()[0][3],m.ptr()[1][3],m.ptr()[2][3],m.ptr()[3][3]);
    for (int i=0; i<3; ++i) {
        for (int j=0; j<4; ++j) {
            m.ptr()[j][i] += row3[j]*double(dist[i]);
        }
    }
#if 0
    // this is slower
    simd4x4_t<double,4> mt = simd4x4::transpose(m);
    __m256d row3 = mt.m4x4()[3];
    for (int i=0; i<3; ++i) {
        __m256d _mm256_set1_pd(float(dist[i]));
        mt.m4x4()[i] = _mm256_add_pd(mt.m4x4()[i], _mm256_mul_pd(t, row3));
    }
    m = simd4x4::transpose(mt);
#endif
}

template<typename S>
inline void post_translate(simd4x4_t<double,4>& m, const simd4_t<S,3>& dist) {
    __m256d col3 = m.m4x4()[3];
    for (int i=0; i<3; ++i) {
        __m256d t = _mm256_set1_pd(double(dist[i]));
        col3 = _mm256_add_pd(col3, _mm256_mul_pd(t, m.m4x4()[i]));
    }
    m.m4x4()[3] = col3;
}

template<>
inline simd4_t<double,3> transform<double>(const simd4x4_t<double,4>& m, const simd4_t<double,3>& pt) {
    simd4_t<double,3> res;
    __m256d tpt = m.m4x4()[3];
    for (int i=0; i<3; ++i) {
        __m256d ptd = _mm256_set1_pd(pt[i]);
        tpt = _mm256_add_pd(tpt, _mm256_mul_pd(ptd, m.m4x4()[i]));
    }
    res = tpt;
    return res;
}

} /* namespace simd4x4 */

# elif defined __SSE2__

template<>
class alignas(16) simd4x4_t<double,4>
{
private:
    typedef double  __mtx4d_t[4][4];

    union alignas(16) {
        __m128d simd4x4[4][2];
        alignas(16) __mtx4d_t mtx;
        alignas(16) double array[4*4];
    };

public:
    simd4x4_t(void) {}
    simd4x4_t(double m00, double m01, double m02, double m03,
              double m10, double m11, double m12, double m13,
              double m20, double m21, double m22, double m23,
              double m30, double m31, double m32, double m33) 
    {
        simd4x4[0][0] = _mm_set_pd(m10,m00);
        simd4x4[0][1] = _mm_set_pd(m30,m20);
        simd4x4[1][0] = _mm_set_pd(m11,m01);
        simd4x4[1][1] = _mm_set_pd(m31,m21);
        simd4x4[2][0] = _mm_set_pd(m12,m02);
        simd4x4[2][1] = _mm_set_pd(m32,m22);
        simd4x4[3][0] = _mm_set_pd(m13,m03);
        simd4x4[3][1] = _mm_set_pd(m33,m23);
    }
    simd4x4_t(const double m[4*4]) {
        const double *p = m;
        for (int i=0; i<4; ++i) {
            simd4_t<double,4> vec4(p);
            simd4x4[i][0] = vec4.v4()[0]; p += 4;
            simd4x4[i][1] = vec4.v4()[1];
            simd4x4[i][0] = _mm_loadu_pd((const double*)&m[4*i]);
            simd4x4[i][1] = _mm_loadu_pd((const double*)&m[4*i+2]);
        }
    }

    simd4x4_t(const __mtx4d_t m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i][0] = simd4_t<double,4>(m[i]).v4()[0];
            simd4x4[i][1] = simd4_t<double,4>(m[i]).v4()[1];
            simd4x4[i][0] = _mm_loadu_pd(m[i]);
            simd4x4[i][1] = _mm_loadu_pd(m[i+2]);
        }
    }
    simd4x4_t(const simd4x4_t<double,4>& m) {
        for (int i=0; i<4; ++i) {
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

    template<int M>
    inline void set(int i, const simd4_t<double,M>& v) {
        simd4x4[i][0] = v.v4()[0];
        simd4x4[i][1] = v.v4()[1];
    }

    inline simd4x4_t<double,4>& operator+=(const simd4x4_t<double,4>& m) {
        for (int i=0; i<4; ++i) {
           simd4x4[i][0] = _mm_add_pd(simd4x4[i][0], m.m4x4()[i][0]);
           simd4x4[i][1] = _mm_add_pd(simd4x4[i][1], m.m4x4()[i][1]);
        }
        return *this;
    }

    inline simd4x4_t<double,4>& operator-=(const simd4x4_t<double,4>& m) {
        for (int i=0; i<4; ++i) {
           simd4x4[i][0] = _mm_sub_pd(simd4x4[i][0], m.m4x4()[i][0]);
           simd4x4[i][1] = _mm_sub_pd(simd4x4[i][1], m.m4x4()[i][1]);
        }
        return *this;
    }

    inline simd4x4_t<double,4>& operator*=(double f) {
        __m128d f4 = _mm_set1_pd(f);
        for (int i=0; i<4; ++i) {
           simd4x4[i][0] = _mm_mul_pd(simd4x4[i][0], f4);
           simd4x4[i][1] = _mm_mul_pd(simd4x4[i][1], f4);
        }
        return *this;
    }

    simd4x4_t<double,4>& operator*=(const simd4x4_t<double,4>& m2) {
        simd4x4_t<double,4> m1 = *this;
        simd4_t<double,4> row, col;
        for (int i=0; i<4; ++i ) {
            simd4_t<double,4> col = m1.m4x4()[0];
            row = col * m2.ptr()[i][0];
            for (int j=1; j<4; ++j) {
                col = m1.m4x4()[j];
                row += col * m2.ptr()[i][j];
            }
            simd4x4[i][0] = row.v4()[0];
            simd4x4[i][1] = row.v4()[1];
        }
        return *this;
    }

};

template<int M>
inline simd4_t<double,M> operator*(const simd4x4_t<double,4>& m, const simd4_t<double,M>& vi)
{
    __m128d mv[2];

    mv[0] = _mm_mul_pd(m.m4x4()[0][0], _mm_set1_pd(vi.ptr()[0]));
    mv[1] = _mm_mul_pd(m.m4x4()[0][1], _mm_set1_pd(vi.ptr()[0]));
    for (int i=1; i<M; ++i) {
        __m128d row[2];
        row[0] = _mm_mul_pd(m.m4x4()[i][0], _mm_set1_pd(vi.ptr()[i]));
        row[1] = _mm_mul_pd(m.m4x4()[i][1], _mm_set1_pd(vi.ptr()[i]));
        mv[0] = _mm_add_pd(mv[0], row[0]);
        mv[1] = _mm_add_pd(mv[1], row[1]);
    }
    return mv;
}

namespace simd4x4
{

template<>
inline simd4x4_t<double,4> rotation_matrix<double>(double angle, const simd4_t<double,3>& axis)
{
    double s = std::sin(angle), c = std::cos(angle), t = 1.0-c;
    simd4_t<double,3> axt, at = axis*t, as = axis*s;
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

inline void translate(simd4x4_t<double,4>& m, const simd4_t<double,3>& dist) {
    m.m4x4()[3][0] = _mm_sub_pd(m.m4x4()[3][0], dist.v4()[0]);
    m.m4x4()[3][1] = _mm_sub_pd(m.m4x4()[3][1], dist.v4()[1]);
}

template<typename S>
inline void pre_translate(simd4x4_t<double,4>& m, const simd4_t<S,3>& dist)
{
#if 1
    simd4_t<double,4> row3(m.ptr()[0][3],m.ptr()[1][3],m.ptr()[2][3],m.ptr()[3][3]);
    for (int i=0; i<3; ++i) {
        for (int j=0; j<4; ++j) {
            m.ptr()[j][i] += row3[j]*double(dist[i]);
        }
    }
#endif
#if 0
    // twice as slow
    simd4x4_t<double,4> mt = simd4x4::transpose(m);
    __m128d row3[2];
    row3[0] = mt.m4x4()[3][0];
    row3[1] = mt.m4x4()[3][1];
    for (int i=0; i<3; ++i) {
        __m128d t = _mm_set1_pd(double(dist[i]));
        mt.m4x4()[i][0] = _mm_add_pd(mt.m4x4()[i][0], _mm_mul_pd(t, row3[0]));
        mt.m4x4()[i][1] = _mm_add_pd(mt.m4x4()[i][1], _mm_mul_pd(t, row3[1]));
    }
    m = simd4x4::transpose(mt);
#endif
}

template<typename S>
inline void post_translate(simd4x4_t<double,4>& m, const simd4_t<S,3>& dist) {
    __m128d col3[2];
    col3[0] = m.m4x4()[3][0];
    col3[1] = m.m4x4()[3][1];
    for (int i=0; i<3; ++i) {
        __m128d t = _mm_set1_pd(double(dist[i]));
        col3[0] = _mm_add_pd(col3[0], _mm_mul_pd(t, m.m4x4()[i][0]));
        col3[1] = _mm_add_pd(col3[1], _mm_mul_pd(t, m.m4x4()[i][1]));
    }
    m.m4x4()[3][0] = col3[0];
    m.m4x4()[3][1] = col3[1];
}

template<>
inline simd4_t<double,3> transform<double>(const simd4x4_t<double,4>& m, const simd4_t<double,3>& pt) {
    simd4_t<double,3> tpt;
    tpt.v4()[0] = m.m4x4()[3][0];
    tpt.v4()[1] = m.m4x4()[3][1];
    for (int i=0; i<3; ++i) {
        __m128d ptd = _mm_set1_pd(pt[i]);
        tpt.v4()[0] = _mm_add_pd(tpt.v4()[0], _mm_mul_pd(ptd, m.m4x4()[i][0]));
        tpt.v4()[1] = _mm_add_pd(tpt.v4()[1], _mm_mul_pd(ptd, m.m4x4()[i][1]));
    }
    return tpt;
}

} /* namespace simd4x4 */

# endif


# ifdef __SSE2__
template<>
class alignas(16) simd4x4_t<int,4>
{
private:
    typedef int  __mtx4i_t[4][4];

    union alignas(16) {
        __m128i simd4x4[4];
        alignas(16) __mtx4i_t mtx;
        alignas(16) int array[4*4];
    };

public:
    simd4x4_t(void) {}
    simd4x4_t(int m00, int m01, int m02, int m03,
              int m10, int m11, int m12, int m13,
              int m20, int m21, int m22, int m23,
              int m30, int m31, int m32, int m33)
    {
        simd4x4[0] = _mm_set_epi32(m30,m20,m10,m00);
        simd4x4[1] = _mm_set_epi32(m31,m21,m11,m01);
        simd4x4[2] = _mm_set_epi32(m32,m22,m12,m02);
        simd4x4[3] = _mm_set_epi32(m33,m23,m13,m03);
    }
    simd4x4_t(const int m[4*4]) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = _mm_loadu_si128((const __m128i*)&m[4*i]);
        }
    }
    simd4x4_t(const __mtx4i_t m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = _mm_loadu_si128((const __m128i*)&m[i]);
        }
    }
    simd4x4_t(const simd4x4_t<int,4>& m) {
        for (int i=0; i<4; ++i) {
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

    template<int M>
    inline void set(int i, const simd4_t<int,M>& v) {
        simd4x4[i] = v.v4();
    }

    inline simd4x4_t<int,4>& operator+=(const simd4x4_t<int,4>& m) {
        for (int i=0; i<4; ++i) {
           simd4x4[i] = _mm_add_epi32(simd4x4[i], m.m4x4()[i]);
        }
        return *this;
    }

    inline simd4x4_t<int,4>& operator-=(const simd4x4_t<int,4>& m) {
        for (int i=0; i<4; ++i) {
           simd4x4[i] = _mm_sub_epi32(simd4x4[i], m.m4x4()[i]);
        }
        return *this;
    }

    inline simd4x4_t<int,4>& operator*=(int v) {
        simd4_t<int,4> v4(v);
        for (int i=0; i<4; ++i) {
           simd4x4[i] *= v4.v4();
        }
        return *this;
    }

    simd4x4_t<int,4>& operator*=(const simd4x4_t<int,4>& m2) {
        simd4x4_t<int,4> m1 = *this;
        simd4_t<int,4> row, col;

        for (int i=0; i<4; ++i) {
            simd4_t<int,4> col(m2.ptr()[i][0]);
            row.v4() = m1.m4x4()[0] * col.v4();
            for (int j=1; j<4; ++j) {
                simd4_t<int,4> col(m2.ptr()[i][j]);
                row.v4() += m1.m4x4()[j] * col.v4();
            }
            simd4x4[i] = row.v4();
        }
        return *this;
    }
};

template<int M>
inline simd4_t<int,M> operator*(const simd4x4_t<int,4>& m, const simd4_t<int,M>& vi)
{
    simd4_t<int,M> mv(m.m4x4()[0]);
    mv *= vi.ptr()[0];
    for (int i=1; i<M; ++i) {
        simd4_t<int,4> row(m.m4x4()[i]);
        row *= vi.ptr()[i];
        mv.v4() += row.v4();
    }
    for (int i=M; i<4; ++i) mv[i] = 0;
    return mv;
}

template<>
inline simd4x4_t<int,4> operator*(const simd4x4_t<int,4>& m1, const simd4x4_t<int,4>& m2)
{
    simd4_t<int,4> row, col;
    simd4x4_t<int,4> m;

    for (int i=0; i<4; ++i) {
        simd4_t<int,4> col(m2.ptr()[i][0]);
        row.v4() = m1.m4x4()[0] * col.v4();
        for (int j=1; j<4; ++j) {
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

inline void translate(simd4x4_t<int,4>& m, const simd4_t<int,3>& dist) {
    m.m4x4()[3] = _mm_sub_epi32(m.m4x4()[3], dist.v4());
}

template<typename S>
inline void pre_translate(simd4x4_t<int,4>& m, const simd4_t<S,3>& dist)
{
    simd4x4_t<int,4> mt = simd4x4::transpose(m);
    simd4_t<int,4> row3(mt.ptr()[3]);
    for (int i=0; i<3; ++i) {
        simd4_t<int,4> trow3 = int(dist[i]);
        trow3 *= row3.v4();
        mt.m4x4()[i] = _mm_add_epi32(mt.m4x4()[i], trow3.v4());
    }
    m = simd4x4::transpose(mt);
}

template<typename S>
inline void post_translate(simd4x4_t<int,4>& m, const simd4_t<S,3>& dist)
{
    __m128i col3 = m.m4x4()[3];
    for (int i=0; i<3; ++i) {
        simd4_t<int,4> trow3 = int(dist[i]);
        trow3 *= m.m4x4()[i];
        col3 = _mm_add_epi32(col3, trow3.v4());
    }
    m.m4x4()[3] = col3;
}

template<>
inline simd4_t<int,3> transform<int>(const simd4x4_t<int,4>& m, const simd4_t<int,3>& pt) {
    simd4_t<int,3> tpt = m.m4x4()[3];
    for (int i=0; i<3; ++i) {
        simd4_t<int,3> ptd = m.m4x4()[i];
        ptd *= pt[i];
        tpt.v4() = _mm_add_epi32(tpt.v4(), ptd.v4());
    }
    return tpt;
}


} /* namespace simd4x */
# endif

#endif /* ENABLE_SIMD_CODE */

#endif /* __SIMD4X4_H__ */

