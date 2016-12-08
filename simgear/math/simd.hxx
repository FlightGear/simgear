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
#include <cmath>


template<typename T, int N> class simd4_t;

namespace simd4
{

template<typename T, int N>
inline simd4_t<T,N> abs(simd4_t<T,N> v) {
    for (int i=0; i<N; i++) {
        v[i] = std::abs(v[i]);
    }
    return v;
}

template<typename T, int N>
inline T magnitude2(simd4_t<T,N> v) {
    T mag2 = 0;
    v = v*v;
    for (int i=0; i<N; i++) {
        mag2 += v[i];
    }
    return mag2;
}

template<typename T, int N>
inline T magnitude(const simd4_t<T,N>& v) {
    return std::sqrt(magnitude2(v));
}

template<typename T, int N>
inline T normalize(simd4_t<T,N>& v) {
    T mag = simd4::magnitude(v);
    if (mag) {
        v /= mag;
    } else {
        v = T(0);
    }
    return mag;
}

template<typename T, int N>
inline T dot(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    T dp = 0;
    v1 *= v2;
    for (int i=0; i<N; i++) {
       dp += v1[i];
    }
    return dp;
}

} /* namespace simd4 */


template<typename T, int N>
class simd4_t
{
private:
    union {
       T _v4[4];
       T vec[N];
    };

public:
    simd4_t(void) {}
    simd4_t(T s) {
        for (int i=0; i<N; i++) vec[i] = s;
        for (int i=N; i<4; i++) _v4[i] = 0;
    }
    explicit simd4_t(const T v[N]) {
        std::memcpy(vec, v, sizeof(T[N]));
    }
    template<int M>
    simd4_t(const simd4_t<T,M>& v) {
        if (M<4) std::memset(_v4+M, 0, sizeof(T[4-M]));
        std::memcpy(vec, v.ptr(), sizeof(T[M]));
    }
    ~simd4_t(void) {}

    inline T (&v4(void))[4] {
        return vec;
    }

    inline const T (&v4(void) const)[4] {
        return vec;
    }

    inline const T (&ptr(void) const)[N] {
        return vec;
    }

    inline T (&ptr(void))[N] {
        return vec;
    }

    inline operator const T*(void) const {
        return vec;
    }

    inline operator T*(void) {
        return vec;
    }

    inline simd4_t<T,N>& operator=(T s) {
        for (int i=0; i<N; i++) vec[i] = s;
        for (int i=N; i<4; i++) _v4[i] = 0;
        return *this;
    }
    inline simd4_t<T,N>& operator=(const T v[N]) {
        if (N<4) std::memset(_v4+N, 0, sizeof(T[4-N]));
        std::memcpy(vec, v, sizeof(T[N]));
        return *this;
    }
    template<int M>
    inline simd4_t<T,N>& operator=(const simd4_t<T,M>& v) {
        if (M<4) std::memset(_v4+M, 0, sizeof(T[4-M]));
        std::memcpy(vec, v.ptr(), sizeof(T[M]));
        return *this;
    }

    inline simd4_t<T,N> operator+(T s) {
        simd4_t<T,N> r(*this);
        r += s;
        return r;
    }
    inline simd4_t<T,N> operator+(const T v[N]) {
        simd4_t<T,N> r(v);
        r += *this;
        return r;
    }
    inline simd4_t<T,N> operator+(simd4_t<T,N> v)
    {
        v += *this;
        return v;
    }

    inline simd4_t<T,N> operator-(void) {
        simd4_t<T,N> r(0);
        r -= *this;
        return r;
    }
    inline simd4_t<T,N> operator-(T s) {
        simd4_t<T,N> r(*this);
        r -= s;
        return r;
    }
    inline simd4_t<T,N> operator-(const simd4_t<T,N>& v) {
        simd4_t<T,N> r(*this);
        r -= v;
        return r;
    }
    inline simd4_t<T,N> operator*(T s) {
        simd4_t<T,N> r(s);
        r *= *this;
        return r;
    }
    inline simd4_t<T,N> operator*(const T v[N]) {
        simd4_t<T,N> r(v);
        r *= *this;
        return r;
    }
    inline simd4_t<T,N> operator*(simd4_t<T,N> v) {
        v *= *this; return v;
    }

    inline simd4_t<T,N> operator/(T s) {
        simd4_t<T,N> r(1/s);
        r *= this;
        return r;
    }
    inline simd4_t<T,N> operator/(const T v[N]) {
        simd4_t<T,N> r(*this);
        r /= v;
        return r;
    }
    inline simd4_t<T,N> operator/(const simd4_t<T,N>& v) {
        simd4_t<T,N> r(*this);
        r /= v; return v;
    }

    inline simd4_t<T,N>& operator+=(T s) {
        for (int i=0; i<N; i++) {
            vec[i] += s;
        }
        return *this;
    }
    inline simd4_t<T,N>& operator+=(const T v[N]) {
        simd4_t<T,N> r(v);
        *this += r.v4();
        return *this;
    }
    template<int M>
    inline simd4_t<T,N>& operator+=(const simd4_t<T,M>& v) {
        for (int i=0; i<M; i++) {
           vec[i] += v[i];
        }
        return *this;
    }

    inline simd4_t<T,N>& operator-=(T s) {
        for (int i=0; i<N; i++) {
           vec[i] -= s;
        }
        return *this;
    }
 
    inline simd4_t<T,N>& operator-=(const T v[N]) {
        simd4_t<T,N> r(v);
        *this -= r.v4();
        return *this;
    }
    inline simd4_t<T,N>& operator-=(const simd4_t<T,N>& v) {
        for (int i=0; i<N; i++) {
            vec[i] -= v[i];
        }
        return *this;
    }
    inline simd4_t<T,N>& operator*=(T s) {
        for (int i=0; i<N; i++) {
           vec[i] *= s;
        }
        return *this;
    }
    inline simd4_t<T,N>& operator*=(const T v[N]) {
        simd4_t<T,N> r(v);
        *this *= r.v4();
        return *this;
    }
    inline simd4_t<T,N>& operator*=(const simd4_t<T,N>& v) {
        for (int i=0; i<N; i++) {
           vec[i] *= v[i];
        }
        return *this;
    }

    inline simd4_t<T,N>& operator/=(T s) {
        return operator*=(1/s);
    }
    inline simd4_t<T,N>& operator/=(const T v[N]) {
        simd4_t<T,N> r(v);
        *this /= r.v4();
        return *this;
    }
    inline simd4_t<T,N>& operator/=(const simd4_t<T,N>& v) {
        for (int i=0; i<N; i++) {
           vec[i] /= v[i];
        }
        return *this;
    }
};

template<typename T, int N>
inline simd4_t<T,N> operator+(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    v1 += v2;
    return v1;
}

template<typename T, int N>
inline simd4_t<T,N> operator-(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    v1 -= v2;
    return v1;
}

template<typename T, int N>
inline simd4_t<T,N> operator*(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    v1 *= v2;
    return v1;
}

template<typename T, int N>
inline simd4_t<T,N> operator*(T f, simd4_t<T,N> v) {
    v *= f;
    return v;
}

template<typename T, int N>
inline simd4_t<T,N> operator*(simd4_t<T,N> v, T f) {
    v *= f;
    return v;
}


# ifdef __MMX__
#  include <mmintrin.h>
# endif

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
        float _v4[4];
    };

public:
    simd4_t(void) {}
    simd4_t(float f) {
        simd4 = _mm_set1_ps(f);
        for (int i=N; i<4; i++) _v4[i] = 0.0f;
    }
    explicit simd4_t(const __vec4f_t v) {
        simd4 = _mm_loadu_ps(v);
        for (int i=N; i<4; i++) _v4[i] = 0.0f;
    }
    template<int M>
    simd4_t(const simd4_t<float,M>& v) {
        simd4 = v.v4();
        for (int i=M; i<4; i++) _v4[i] = 0.0f;
    }
    simd4_t(const __m128& v) {
        simd4 = v;
    }

    inline __m128 (&v4(void)) {
        return simd4;
    }

    inline const __m128 (&v4(void) const) {
        return simd4;
    }

    inline const float (&ptr(void) const)[N] {
        return vec;
    }

    inline float (&ptr(void))[N] {
        return vec;
    }

    inline operator const float*(void) const {
        return vec;
    }

    inline operator float*(void) {
        return vec;
    }

    inline simd4_t<float,N>& operator=(float f) {
        simd4 = _mm_set1_ps(f);
        for (int i=N; i<4; i++) _v4[i] = 0.0f;
        return *this;
    }
    inline simd4_t<float,N>& operator=(const __vec4f_t v) {
        simd4 = _mm_loadu_ps(v);
        for (int i=N; i<4; i++) _v4[i] = 0.0f;
        return *this;
    }
    template<int M>
    inline simd4_t<float,N>& operator=(const simd4_t<float,M>& v) {
        simd4 = v.v4();
        for (int i=M; i<4; i++) _v4[i] = 0.0f;
        return *this;
    }

    inline simd4_t<float,N>& operator+=(float f) {
        simd4_t<float,N> r(f);
        simd4 += r.v4();
        return *this;
    }
    inline simd4_t<float,N>& operator+=(const simd4_t<float,N>& v) {
        simd4 += v.v4();
        return *this;
    }

    inline simd4_t<float,N>& operator-=(float f) {
        simd4_t<float,N> r(f);
        simd4 -= r.v4();
        return *this;
    }
    inline simd4_t<float,N>& operator-=(const simd4_t<float,N>& v) {
        simd4 -= v.v4();
        return *this;
    }

    inline simd4_t<float,N>& operator*=(float f) {
        simd4_t<float,N> r(f);
        simd4 *= r.v4();
        return *this;
    }
    inline simd4_t<float,N>& operator*=(const simd4_t<float,N>& v) {
        simd4 *= v.v4();
        return *this;
    }

    inline simd4_t<float,N>& operator/=(float f) {
        simd4_t<float,N> r(1.0f/f);
        simd4 *= r.v4();
        return *this;
    }
    inline simd4_t<float,N>& operator/=(const simd4_t<float,N>& v) {
        simd4 /= v.v4();
        return *this;
    }
};

namespace simd4
{
template<int N>
inline simd4_t<float,N>abs(simd4_t<float,N> v) {
    static const __m128 sign_mask = _mm_set1_ps(-0.f); // -0.f = 1 << 31
    v.v4() = _mm_andnot_ps(sign_mask, v.v4());
    return v;
  }
};
# endif


# ifdef __SSE2__
#  include <emmintrin.h>

template<int N>
class simd4_t<double,N>
{
private:
   typedef double  __vec4d_t[N];

    union {
        __m128d simd4[2];
        __vec4d_t vec;
        double _v4[4];
    };

public:
    simd4_t(void) {}
    simd4_t(double d) {
        simd4[0] = simd4[1] = _mm_set1_pd(d);
        for (int i=N; i<4; i++) _v4[i] = 0.0;
    }
    explicit simd4_t(const __vec4d_t v) {
        simd4[0] = _mm_loadu_pd(v);
        simd4[1] = _mm_loadu_pd(v+2);
        for (int i=N; i<4; i++) _v4[i] = 0.0;
    }
    template<int M>
    simd4_t(const simd4_t<double,M>& v) {
        simd4[0] = v.v4()[0];
        simd4[1] = v.v4()[1];
        for (int i=M; i<4; i++) _v4[i] = 0.0;
    }
    simd4_t(const __m128d v[2]) {
        simd4[0] = v[0];
        simd4[1] = v[1];
    }

    inline __m128d (&v4(void))[2] {
        return simd4;
    }

    inline const __m128d (&v4(void) const)[2] {
        return simd4;
    }

    inline const double (&ptr(void) const)[N] {
        return vec;
    }

    inline double (&ptr(void))[N] {
        return vec;
    }

    inline operator const double*(void) const {
        return vec;
    }

    inline operator double*(void) {
        return vec;
    }

    inline simd4_t<double,N>& operator=(double d) {
        simd4[0] = simd4[1] = _mm_set1_pd(d);
        for (int i=N; i<4; i++) _v4[i] = 0.0;
        return *this;
    }
    inline simd4_t<double,N>& operator=(const __vec4d_t v) {
        simd4[0] = _mm_loadu_pd(v);
        simd4[1] = _mm_loadu_pd(v+2);
        for (int i=N; i<4; i++) _v4[i] = 0.0;
        return *this;
    }
    template<int M>
    inline simd4_t<double,N>& operator=(const simd4_t<double,M>& v) {
        simd4[0] = v.v4()[0];
        simd4[1] = v.v4()[1];
        for (int i=M; i<4; i++) _v4[i] = 0.0;
        return *this;
    }
    inline simd4_t<double,N>& operator=(const __m128d v[2]) {
        simd4[0] = v[0];
        simd4[1] = v[1];
        return *this;
    }

    inline simd4_t<double,N>& operator+=(double d) {
        simd4_t<double,N> r(d);
        simd4[0] += r.v4()[0];
        simd4[1] += r.v4()[1];
        return *this;
    }
    inline simd4_t<double,N>& operator+=(const simd4_t<double,N>& v) {
        simd4[0] += v.v4()[0];
        simd4[1] += v.v4()[1];
        return *this;
    }

    inline simd4_t<double,N>& operator-=(double d) {
        simd4_t<double,N> r(d);
        simd4[0] -= r.v4()[0];
        simd4[1] -= r.v4()[1];
        return *this;
    }
    inline simd4_t<double,N>& operator-=(const simd4_t<double,N>& v) {
        simd4[0] -= v.v4()[0];
        simd4[1] -= v.v4()[1];
        return *this;
    }

    inline simd4_t<double,N>& operator*=(double d) {
        simd4_t<double,N> r(d);
        simd4[0] *= r.v4()[0];
        simd4[1] *= r.v4()[1];
        return *this;
    }
    inline simd4_t<double,N>& operator*=(const simd4_t<double,N>& v) {
        simd4[0] *= v.v4()[0];
        simd4[1] *= v.v4()[1];
        return *this;
    }

    inline simd4_t<double,N>& operator/=(double d) {
        simd4_t<double,N> r(1.0/d);
        simd4[0] *= r.v4()[0];
        simd4[1] *= r.v4()[1];
        return *this;
    }
    inline simd4_t<double,N>& operator/=(const simd4_t<double,N>& v) {
        simd4[0] /= v.v4()[0];
        simd4[1] /= v.v4()[1];
        return *this;
    }
};

namespace simd4
{
template<int N>
inline simd4_t<double,N>abs(simd4_t<double,N> v) {
    static const __m128d sign_mask = _mm_set1_pd(-0.); // -0. = 1 << 63
    v.v4()[0] = _mm_andnot_pd(sign_mask, v.v4()[0]);
    v.v4()[1] = _mm_andnot_pd(sign_mask, v.v4()[1]);
    return v;
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
        int _v4[4];
    };

public:
    simd4_t(void) {}
    simd4_t(int i) {
        simd4 = _mm_set1_epi32(i);
        for (int i=N; i<4; i++) _v4[i] = 0;
    }
    explicit simd4_t(const __vec4i_t v) {
        simd4 = _mm_loadu_si128((__m128i*)v);
        for (int i=N; i<4; i++) _v4[i] = 0;
    }
    template<int M>
    simd4_t(const simd4_t<int,M>& v) {
        simd4 = v.v4();
        for (int i=M; i<4; i++) _v4[i] = 0;
    }
    simd4_t(const __m128i& v) {
        simd4 = v;
    }

    inline __m128i (&v4(void)) {
        return simd4;
    }

    inline const __m128i (&v4(void) const) {
        return simd4;
    }

    inline const int (&ptr(void) const)[N] {
        return vec;
    }

    inline int (&ptr(void))[N] {
        return vec;
    }

    inline operator const int*(void) const {
        return vec;
    }

    inline operator int*(void) {
        return vec;
    }

    inline simd4_t<int,N>& operator=(int i) {
        simd4 = _mm_set1_epi32(i);
        for (int i=N; i<4; i++) _v4[i] = 0;
        return *this;
    }
    inline simd4_t<int,N>& operator=(const __vec4i_t v) {
        simd4 = _mm_loadu_si128((__m128i*)v);
        for (int i=N; i<4; i++) _v4[i] = 0;
        return *this;
    }
    template<int M>
    inline simd4_t<int,N>& operator=(const simd4_t<int,M>& v) {
        simd4 = v.v4();
        for (int i=M; i<4; i++) _v4[i] = 0;
        return *this;
    }

    inline simd4_t<int,N>& operator+=(int i) {
        simd4_t<int,N> r(i);
        simd4 += r.v4();
        return *this;
    }
    inline simd4_t<int,N>& operator+=(const simd4_t<int,N>& v) {
        simd4 += v.v4();
        return *this;
    }

    inline simd4_t<int,N>& operator-=(int i) {
        simd4_t<int,N> r(i);
        simd4 -= r.v4();
        return *this;
    }
    inline simd4_t<int,N>& operator-=(const simd4_t<int,N>& v) {
        simd4 -= v.v4();
        return *this;
    }

    inline simd4_t<int,N>& operator*=(int i) {
        simd4_t<int,N> r(i);
        simd4 *= r.v4();
        return *this;
    }
    inline simd4_t<int,N>& operator*=(const simd4_t<int,N>& v) {
        simd4 *= v.v4();
        return *this;
    }

    inline simd4_t<int,N>& operator/=(int i) {
        simd4_t<int,N> r(i);
        simd4 /= r.v4();
        return *this;
    }
    inline simd4_t<int,N>& operator/=(const simd4_t<int,N>& v) {
        simd4 /= v.v4();
        return *this;
    }
};
# endif


#endif /* __SIMD_H__ */

