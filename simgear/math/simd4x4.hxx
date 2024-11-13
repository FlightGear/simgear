// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2016 Erik Hofman <erik@ehofman.com>

#ifndef __SIMD4X4_H__
#define __SIMD4X4_H__	1

#include <cstring>

template <typename T, int N>
class simd4x4_t;

#include <simgear/math/simd.hxx>

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
    T s = std::sin(angle);
    T c = std::cos(angle);
    T t = T(1) - c;

    simd4_t<T,3> at = axis*t;
    simd4_t<T,3> as = axis*s;
    simd4x4_t<T,4> m;

    simd4x4::unit(m);
    simd4_t<T,3> aat = axis.ptr()[0]*at;
    m.ptr()[0][0] = aat.ptr()[0] + c;
    m.ptr()[0][1] = aat.ptr()[1] + as.ptr()[2];
    m.ptr()[0][2] = aat.ptr()[2] - as.ptr()[1];

    aat = axis.ptr()[1]*at;
    m.ptr()[1][0] = aat.ptr()[0] - as.ptr()[2];
    m.ptr()[1][1] = aat.ptr()[1] + c;
    m.ptr()[1][2] = aat.ptr()[2] + as.ptr()[0];

    aat = axis.ptr()[2]*at;
    m.ptr()[2][0] = aat.ptr()[0] + as.ptr()[1];
    m.ptr()[2][1] = aat.ptr()[1] - as.ptr()[0];
    m.ptr()[2][2] = aat.ptr()[2] + c;

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
class simd4x4_t final
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
    ~simd4x4_t(void) {}     // non-virtual intentional

    simd4x4_t& operator=(const simd4x4_t& rhs) {
        std::memcpy(array, rhs.array, sizeof(array));
        return *this;
    }

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

#endif /* __SIMD4X4_H__ */

