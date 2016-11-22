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
    explicit simd4x4_t(const T m[N][N]) {
        std::memcpy(array, m, sizeof(T[N][N]));
    }
    simd4x4_t(const simd4x4_t<T,N>& m) {
        std::memcpy(array, m, sizeof(T[N][N]));
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

    inline simd4x4_t<T,N>& operator=(const T m[N][N]) {
        std::memcpy(array, m, sizeof(T[N][N]));
        return *this;
    }
    inline simd4x4_t<T,N>& operator=(const simd4x4_t<T,N>& m) {
        std::memcpy(array, m, sizeof(T[N][N]));
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

    template<typename S>
    inline simd4x4_t<T,N>& operator*=(S s) {
        for (int i=0; i<N*N; i++) {
           array[i] *= s;
        }
        return *this;
    }
    inline simd4x4_t<T,N>& operator*=(const simd4x4_t<T,N>& m) {
//TODO
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
     for (int i=0; i<4; i++) {
         r.ptr()[i][i] = 1;
     }
  }

#if 0
template<typename S, int N>
simd4_t<T,N>
_pt4Matrix4_sse(const simd4x4_t<T,N>& m, const simd4_t<S,N>& vi)
{
    simd4_t<T,N> r;
    simd4_t<T,N> x(vi[0]);
    r = x*m[0];
    for (int i=1; i<N; i++ {
       x = vi[i];
       r += x*m[i];
    }
    return r;
}
#endif
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
    explicit simd4x4_t(const __mtx4f_t m) {
        for (int i=0; i<4; i++) {
            simd4x4[i] = _mm_loadu_ps(m[i]);
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
            simd4x4[i] = _mm_loadu_ps(m[i]);
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
        __m128 f4 = _mm_set1_ps(f);
        for (int i=0; i<4; i++) {
           simd4x4[i] *= f4;
        }
        return *this;
    }
};

# endif

#endif /* __SIMD4X4_H__ */

