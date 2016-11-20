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

#ifndef __SIMD_H__
#define __SIMD_H__	1

#include <cstring>


template<typename T, int N>
class simd4_t
{
private:
    union {
       T v4[4];
       T vec[N];
    };

public:
    simd4_t() {}
    template<typename S>
    simd4_t(S s) {
        vec[0] = vec[1] = vec[2] = vec[3] = s;
    }
    explicit simd4_t(const T v[N]) {
        std::memcpy(vec, v, sizeof(T[N]));
    }
    simd4_t(const simd4_t<T,N>& v) {
        std::memcpy(vec, v.vec, sizeof(T[N]));
    }
    ~simd4_t() {}

    T (&ptr(void))[N] {
        return vec;
    }

    const T (&ptr(void) const)[N] {
        return vec;
    }

    inline operator const T*() const {
        return vec;
    }

    inline operator T*() {
        return vec;
    }

    template<typename S>
    inline simd4_t<T,N>& operator=(S s) {
        vec[0] = vec[1] = vec[2] = vec[3] = s;
        return *this;
    }
    inline simd4_t<T,N>& operator=(const T v[N]) {
        std::memcpy(vec, v, sizeof(T[N]));
        return *this;
    }
    inline simd4_t<T,N>& operator=(const simd4_t<T,N>& v) {
        std::memcpy(vec, v.vec, sizeof(T[N]));
        return *this;
    }

    template<typename S>
    inline simd4_t<T,N> operator+(S s)
    {
        simd4_t<T,N> r(*this);
        r += s;
        return r;
    }
    inline simd4_t<T,N> operator+(const T v[N])
    {
        simd4_t<T,N> r(v);
        r += *this;
        return r;
    }
    inline simd4_t<T,N> operator+(const simd4_t<T,N>& v)
    {
        simd4_t<T,N> r(v);
        r += *this;
        return r;
    }

    inline simd4_t<T,N> operator-()
    {
        simd4_t<T,N> r(0);
        r -= *this;
        return r;
    }
    template<typename S>
    inline simd4_t<T,N> operator-(S s)
    {
        simd4_t<T,N> r(*this);
        r -= s;
        return r;
    }
    inline simd4_t<T,N> operator-(simd4_t<T,N>& v)
    {
        simd4_t<T,N> r(*this);
        r -= v;
        return r;
    }

    template<typename S>
    inline simd4_t<T,N> operator*(S s)
    {
        simd4_t<T,N> r(s);
        r *= *this;
        return r;
    }
    inline simd4_t<T,N> operator*(const T v[N])
    {
        simd4_t<T,N> r(v);
        r *= *this;
        return r;
    }
    inline simd4_t<T,N> operator*(simd4_t<T,N>& v)
    {
        simd4_t<T,N> r(v);
        r *= *this;
        return r;
    }

    template<typename S>
    inline simd4_t<T,N> operator/(S s)
    {
        simd4_t<T,N> r(1/T(s));
        r *= this;
        return r;
    }
    inline simd4_t<T,N> operator/(const T v[N])
    {
        simd4_t<T,N> r(*this);
        r /= v;
        return r;
    }
    inline simd4_t<T,N> operator/(simd4_t<T,N>& v)
    {
        simd4_t<T,N> r(*this);
        r /= v;
        return r;
    }

    template<typename S>
    inline simd4_t<T,N>& operator+=(S s)
    {
        for (int i=0; i<N; i++) {
            vec[i] += s;
        }
        return *this;
    }
    inline simd4_t<T,N>& operator+=(const T v[N])
    {
        simd4_t<T,N> r(v);
        *this += r;
        return *this;
    }
    inline simd4_t<T,N>& operator+=(const simd4_t<T,N>& v)
    {
        for (int i=0; i<N; i++) {
           vec[i] += v[i];
        }
        return *this;
    }

    template<typename S>
    inline simd4_t<T,N>& operator-=(S s)
    {
        for (int i=0; i<N; i++) {
           vec[i] -= s;
        }
        return *this;
    }
 
    inline simd4_t<T,N>& operator-=(const T v[N])
    {
        simd4_t<T,N> r(v);
        *this -= r;
        return *this;
    }
    inline simd4_t<T,N>& operator-=(const simd4_t<T,N>& v)
    {
        for (int i=0; i<N; i++) {
            vec[i] -= v[i];
        }
        return *this;
    }

    template<typename S>
    inline simd4_t<T,N>& operator *=(S s)
    {
        for (int i=0; i<N; i++) {
           vec[i] *= s;
        }
        return *this;
    }
    inline simd4_t<T,N>& operator*=(const T v[N])
    {
        simd4_t<T,N> r(v);
        *this *= r;
        return *this;
    }
    inline simd4_t<T,N>& operator*=(const simd4_t<T,N>& v)
    {
        for (int i=0; i<N; i++) {
           vec[i] *= v[i];
        }
        return *this;
    }

    template<typename S>
    inline simd4_t<T,N>& operator/=(S s)
    {
        s = (1/s);
        for (int i=0; i<N; i++) {
           vec[i] *= s;
        }
        return *this;
    }
    inline simd4_t<T,N>& operator/=(const T v[N])
    {
        simd4_t<T,N> r(v);
        *this /= r;
        return *this;
    }
    inline simd4_t<T,N>& operator/=(const simd4_t<T,N>& v)
    {
        for (int i=0; i<N; i++) {
           vec[i] /= v[i];
        }
        return *this;
    }

};

# ifdef __SSE__
#  include <xmmintrin.h>

template<int N>
class simd4_t<float,N>
{
private:
   typedef float  __vec4f_t[N];

    union {
        __m128 simd4;
        __vec4f_t vec;
    };

public:
    simd4_t() {}
    simd4_t(float f) {
        simd4 = _mm_set1_ps(f);
    }
    explicit simd4_t(const __vec4f_t v) {
        simd4 = _mm_loadu_ps(v);
    }
    simd4_t(const simd4_t<float,N>& v) {
        simd4 = v.simd4;
    }
#if 0
    simd4_t(const simd4_t<double,N>& v) {
        simd4 = _mm_movelh_ps(_mm_cvtpd_ps(v.v4()[0]), _mm_cvtpd_ps(v.v4()[1]));
    }
#endif

    float (&ptr(void))[N] {
        return vec;
    }

    const float (&ptr(void) const)[N] {
        return vec;
    }

    inline operator const float*() const {
        return vec;
    }

    inline operator float*() {
        return vec;
    }

    inline simd4_t<float,N>& operator=(float f) {
        simd4 = _mm_set1_ps(f);
        return *this;
    }
    inline simd4_t<float,N>& operator=(const __vec4f_t v) {
        simd4 = _mm_loadu_ps(v);
        return *this;
    }
    inline simd4_t<float,N>& operator=(const simd4_t<float,N>& v) {
        simd4 = v.simd4;
        return *this;
    }

    inline simd4_t<float,N>& operator+=(float f) {
        simd4 += f;
        return *this;
    }
    inline simd4_t<float,N>& operator+=(const simd4_t<float,N>& v) {
        simd4 += v.simd4;
        return *this;
    }

    inline simd4_t<float,N>& operator-=(float f) {
        simd4 -= f;
        return *this;
    }
    inline simd4_t<float,N>& operator-=(const simd4_t<float,N>& v) {
        simd4 -= v.simd4;
        return *this;
    }

    inline simd4_t<float,N>& operator *=(float f) {
        simd4 *= f;
        return *this;
    }
    inline simd4_t<float,N>& operator*=(const simd4_t<float,N>& v) {
        simd4 *= v.simd4;
        return *this;
    }

    inline simd4_t<float,N>& operator/=(float f) {
        f = (1.0f/f);
        simd4 *= f;
        return *this;
    }
    inline simd4_t<float,N>& operator/=(const simd4_t<float,N>& v) {
        simd4 /= v.simd4;
        return *this;
    }
};
# endif


# ifdef __SSE2
template<int N>
class simd4_t<double,N>
{
private:
   typedef double  __vec4d_t[N];

    union {
        __m128d simd4[2];
        __vec4d_t vec;
    };

public:
    simd4_t() {}
    simd4_t(double d) {
        simd4[0] = simd4[1] = _mm_set1_pd(d);
    }
    explicit simd4_t(const __vec4d_t v) {
        simd4[0] = _mm_loadu_pd(v);
        simd4[1] = _mm_loadu_pd(v+2);
    }
    simd4_t(const simd4_t<double,N>& v) {
        simd4[0] = v.simd4[0];
        simd4[1] = v.simd4[1];
    }

    inline __m128d (&v4(void))[2] {
        return simd4;
    }

    inline const __m128d (&v4(void) const)[2] {
        return simd4;
    }

    inline double (&ptr(void))[N] {
        return vec;
    }

    inline const double (&ptr(void) const)[N] {
        return vec;
    }

    inline operator const double*() const {
        return vec;
    }

    inline operator double*() {
        return vec;
    }

    inline simd4_t<double,N>& operator=(double d) {
        simd4[0] = simd[1] = _mm_set1_pd(d);
        return *this;
    }
    inline simd4_t<double,N>& operator=(const __vec4d_t v) {
        simd4[0] = _mm_loadu_pd(v);
        simd4[1] = _mm_loadu_pd(v+2);
        return *this;
    }
    inline simd4_t<double,N>& operator=(const simd4_t<double,N>& v) {
        simd4[0] = v.simd4[0];
        simd4[1] = v.simd4[1];
        return *this;
    }

    inline simd4_t<double,N>& operator+=(double d) {
        simd4[0] += d;
        simd4[1] += d;
        return *this;
    }
    inline simd4_t<double,N>& operator+=(const simd4_t<double,N>& v) {
        simd4[0] += v.simd4[0];
        simd4[1] += v.simd4[1];
        return *this;
    }

    inline simd4_t<double,N>& operator-=(double d) {
        simd4[0] -= d;
        simd4[1] -= d;
        return *this;
    }
    inline simd4_t<double,N>& operator-=(const simd4_t<double,N>& v) {
        simd4[0] -= v.simd4[0];
        simd4[1] -= v.simd4[1];
        return *this;
    }

    inline simd4_t<double,N>& operator *=(double d) {
        simd4[0] *= d;
        simd4[1] *= d;
        return *this;
    }
    inline simd4_t<double,N>& operator*=(const simd4_t<double,N>& v) {
        simd4[0] *= v.simd4[0];
        simd4[1] *= v.simd4[1];
        return *this;
    }

    inline simd4_t<double,N>& operator/=(double d) {
        d = (1.0/d);
        simd4[0] *= d;
        simd4[1] *= d;
        return *this;
    }
    inline simd4_t<double,N>& operator/=(const simd4_t<double,N>& v) {
        simd4[0] /= v.simd4[0];
        simd4[1] /= v.simd4[1];
        return *this;
    }
};
# endif


# ifdef __SSE2__
#  include <emmintrin.h>

template<int N>
class simd4_t<int,N>
{
private:
   typedef int  __vec4i_t[N];

    union {
        __m128i simd4;
        __vec4i_t vec;
    };

public:
    simd4_t() {}
    simd4_t(int i) {
        simd4 = _mm_set1_epi32(i);
    }
    explicit simd4_t(const __vec4i_t v) {
        simd4 = _mm_loadu_si128((__m128i*)v);
    }
    simd4_t(const simd4_t<int,N>& v) {
        simd4 = v.simd4;
    }

    int (&ptr(void))[4] {
        return vec;
    }

    const int (&ptr(void) const)[4] {
        return vec;
    }

    inline operator const int*() const {
        return vec;
    }

    inline operator int*() {
        return vec;
    }

    inline simd4_t<int,N>& operator=(int i) {
        simd4 = _mm_set1_epi32(i);
        return *this;
    }
    inline simd4_t<int,N>& operator=(const __vec4i_t v) {
        simd4 = _mm_loadu_si128((__m128i*)v);
        return *this;
    }
    inline simd4_t<int,N>& operator=(const simd4_t<int,N>& v) {
        simd4 = v.simd4;
        return *this;
    }

    inline simd4_t<int,N>& operator+=(int i) {
        simd4 += i;
        return *this;
    }
    inline simd4_t<int,N>& operator+=(const simd4_t<int,N>& v) {
        simd4 += v.simd4;
        return *this;
    }

    inline simd4_t<int,N>& operator-=(int i) {
        simd4 -= i;
        return *this;
    }
    inline simd4_t<int,N>& operator-=(const simd4_t<int,N>& v) {
        simd4 -= v.simd4;
        return *this;
    }

    inline simd4_t<int,N>& operator *=(int i) {
        simd4 *= i;
        return *this;
    }
    inline simd4_t<int,N>& operator*=(const simd4_t<int,N>& v) {
        simd4 *= v.simd4;
        return *this;
    }

    inline simd4_t<int,N>& operator/=(int i) {
        i = (1/i);
        simd4 *= i;
        return *this;
    }
    inline simd4_t<int,N>& operator/=(const simd4_t<int,N>& v) {
        simd4 /= v.simd4;
        return *this;
    }
};
# endif


#endif /* __SIMD_H__ */

