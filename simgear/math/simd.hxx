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

# ifdef __SSE__
#  include <xmmintrin.h>
# endif
# ifdef __SSE2__
#  include <emmintrin.h>
# endif


template<typename T>
class simd4_t
{
public:
    ~simd4_t() {}
};

template<>
class simd4_t<float>
{
private:
   typedef float  __vec4f_t[4];

# ifdef __SSE__
    union {
        __m128 simd4;
        __vec4f_t vec;
    };
# else
    __vec4f_t vec;
# endif

public:
    simd4_t() {}
    simd4_t(float f)
    {
# ifdef __SSE__
        simd4 = _mm_set1_ps(f);
# else
        vec[0] = vec[1] = vec[2] = vec[3] = f;
# endif
    }
    explicit simd4_t(const __vec4f_t v)
    {
# ifdef __SSE__
        simd4 = _mm_loadu_ps(v);
# else
        std::memcpy(vec, v, sizeof(float[4]));
# endif
    }
    simd4_t(const simd4_t<float>& v)
    {
# ifdef __SSE__
        simd4 = v.simd4;
# else
        std::memcpy(vec, v.vec, sizeof(float[4]));
# endif
    }

    inline float (&ptr(void))[4] {
        return vec;
    }

    inline const float (&ptr(void) const)[4] {
        return vec;
    }

    inline simd4_t<float>& operator=(float f)
    {
# ifdef __SSE__
        simd4 = _mm_set1_ps(f);
# else
        vec[0] = vec[1] = vec[2] = vec[3] = f;
# endif
        return *this;
    }
    inline simd4_t<float>& operator=(const __vec4f_t v)
    {
# ifdef __SSE__
        simd4 = _mm_loadu_ps(v);
# else
        std::memcpy(vec, v, sizeof(float[4]));
# endif
        return *this;
    }
    inline simd4_t<float>& operator=(const simd4_t<float>& v)
    {
# ifdef __SSE__
        simd4 = v.simd4;
# else
        std::memcpy(vec, v.vec, sizeof(float[4]));
# endif
        return *this;
    }

    inline simd4_t operator+(float f)
    {
        simd4_t r(*this);
        r += f;
        return r;
    }
    inline simd4_t operator+(const __vec4f_t v)
    {
        simd4_t r(*this);
        r += v;
        return r;
    }
    inline simd4_t operator+(const simd4_t<float>& v)
    {
        simd4_t r(*this);
        r += v;
        return r;
    }

    inline simd4_t operator-()
    {
        simd4_t r(0.0f);
        r -= vec;
        return r;
    }
    inline simd4_t operator-(float f)
    {
        simd4_t r(*this);
        r -= f;
        return r;
    }
    inline simd4_t operator-(const __vec4f_t v)
    {
        simd4_t r(*this);
        r -= v;
        return r;
    }
    inline simd4_t operator-(simd4_t<float>& v)
    {
        simd4_t r(*this);
        r -= v;
        return r;
    }

    inline simd4_t operator*(float f)
    {
        simd4_t r(*this);
        r *= f;
        return r;
    }
    inline simd4_t operator*(const __vec4f_t v)
    {
        simd4_t r(*this);
        r *= v;
        return r;
    }
    inline simd4_t operator*(simd4_t<float>& v)
    {
        simd4_t r(*this);
        r *= v;
        return r;
    }

    inline simd4_t operator/(float f)
    {
        simd4_t r(*this);
        r /= f;
        return r;
    }
    inline simd4_t operator/(const __vec4f_t v)
    {
        simd4_t r(*this);
        r /= v;
        return r;
    }
    inline simd4_t operator/(simd4_t<float>& v)
    {
        simd4_t r(*this);
        r /= v;
        return r;
    }

    inline simd4_t<float>& operator+=(float f)
    {
# ifdef __SSE__
        simd4 += f;
# else
        vec[0] += f;
        vec[1] += f;
        vec[2] += f;
        vec[3] += f;
# endif
        return *this;
    }
    inline simd4_t<float>& operator+=(const __vec4f_t v)
    {
        simd4_t r(v);
        *this += r;
        return *this;
    }
    inline simd4_t<float>& operator+=(const simd4_t<float>& v)
    {
# ifdef __SSE__
        simd4 += v.simd4;
# else
        vec[0] += v[0];
        vec[1] += v[1];
        vec[2] += v[2];
        vec[3] += v[3];
# endif
        return *this;
    }

    inline simd4_t<float>& operator-=(float f)
    {
# ifdef __SSE__
        simd4 -= f;
# else
        vec[0] -= f;
        vec[1] -= f;
        vec[2] -= f;
        vec[3] -= f;
# endif
        return *this;
    }
    inline simd4_t<float>& operator-=(const __vec4f_t v)
    {
        simd4_t r(v);
        *this -= r;
        return *this;
    }
    inline simd4_t<float>& operator-=(const simd4_t<float>& v)
    {
# ifdef __SSE__
        simd4 -= v.simd4;
# else
        vec[0] -= v[0];
        vec[1] -= v[1];
        vec[2] -= v[2];
        vec[3] -= v[3];
# endif
        return *this;
    }

    inline simd4_t<float>& operator *=(float f)
    {
# ifdef __SSE__
        simd4 *= f;
# else
        vec[0] *= f;
        vec[1] *= f;
        vec[2] *= f;
        vec[3] *= f;
# endif
        return *this;
    }
    inline simd4_t<float>& operator*=(const __vec4f_t v)
    {
        simd4_t r(v);
        *this *= r;
        return *this;
    }
    inline simd4_t<float>& operator*=(const simd4_t<float>& v)
    {
# ifdef __SSE__
        simd4 *= v.simd4;
# else
        vec[0] *= v[0];
        vec[1] *= v[1];
        vec[2] *= v[2];
        vec[3] *= v[3];
# endif
        return *this;
    }

    inline simd4_t<float>& operator/=(float f)
    {
        f = (1.0f/f);
# ifdef __SSE__
        simd4 *= f;
# else
        vec[0] *= f;
        vec[1] *= f;
        vec[2] *= f;
        vec[3] *= f;
# endif
        return *this;
    }
    inline simd4_t<float>& operator/=(const __vec4f_t v)
    {
        simd4_t r(v);
        *this /= r;
        return *this;
    }
    inline simd4_t<float>& operator/=(const simd4_t<float>& v)
    {
# ifdef __SSE__
        simd4 /= v.simd4;
# else
        vec[0] /= v[0];
        vec[1] /= v[1];
        vec[2] /= v[2];
        vec[3] /= v[3];
# endif
        return *this;
    }

    inline operator const float*() const {
        return vec;
    }

    inline operator float*() {
        return vec;
    }
};


template<>
class simd4_t<double>
{
private:
   typedef double  __vec4d_t[4];

# ifdef __SSE2__
    union {
        __m128d simd4;
        __vec4d_t vec;
    };
# else
    __vec4d_t vec;
# endif

public:
    simd4_t() {}
    simd4_t(double d)
    {
# ifdef __SSE2__
        simd4 = _mm_set1_pd(d);
# else
        vec[0] = vec[1] = vec[2] = vec[3] = d;
# endif
    }
    explicit simd4_t(const __vec4d_t v)
    {
# ifdef __SSE2__
        simd4 = _mm_loadu_pd(v);
# else
        std::memcpy(vec, v, sizeof(double[4]));
# endif
    }
    simd4_t(const simd4_t<double>& v)
    {
# ifdef __SSE2__
        simd4 = v.simd4;
# else
        std::memcpy(vec, v.vec, sizeof(double[4]));
# endif
    }

    inline double (&ptr(void))[4] {
        return vec;
    }

    inline const double (&ptr(void) const)[4] {
        return vec;
    }

    inline simd4_t<double>& operator=(double d)
    {
# ifdef __SSE2__
        simd4 = _mm_set1_pd(d);
# else
        vec[0] = vec[1] = vec[2] = vec[3] = d;
# endif
        return *this;
    }
    inline simd4_t<double>& operator=(const __vec4d_t v)
    {
# ifdef __SSE2__
        simd4 = _mm_loadu_pd(v);
# else
        std::memcpy(vec, v, sizeof(double[4]));
# endif
        return *this;
    }
    inline simd4_t<double>& operator=(const simd4_t<double>& v)
    {
# ifdef __SSE2__
        simd4 = v.simd4;
# else
        std::memcpy(vec, v.vec, sizeof(double[4]));
# endif
        return *this;
    }

    inline simd4_t operator+(double d)
    {
        simd4_t r(*this);
        r += d;
        return r;
    }
    inline simd4_t operator+(const __vec4d_t v)
    {
        simd4_t r(*this);
        r += v;
        return r;
    }
    inline simd4_t operator+(const simd4_t<double>& v)
    {
        simd4_t r(*this);
        r += v;
        return r;
    }

    inline simd4_t operator-()
    {
        simd4_t r(0.0f);
        r -= vec;
        return r;
    }
    inline simd4_t operator-(double d)
    {
        simd4_t r(*this);
        r -= d;
        return r;
    }
    inline simd4_t operator-(const __vec4d_t v)
    {
        simd4_t r(*this);
        r -= v;
        return r;
    }
    inline simd4_t operator-(simd4_t<double>& v)
    {
        simd4_t r(*this);
        r -= v;
        return r;
    }

    inline simd4_t operator*(double d)
    {
        simd4_t r(*this);
        r *= d;
        return r;
    }
    inline simd4_t operator*(const __vec4d_t v)
    {
        simd4_t r(*this);
        r *= v;
        return r;
    }
    inline simd4_t operator*(simd4_t<double>& v)
    {
        simd4_t r(*this);
        r *= v;
        return r;
    }

    inline simd4_t operator/(double d)
    {
        simd4_t r(*this);
        r /= d;
        return r;
    }
    inline simd4_t operator/(const __vec4d_t v)
    {
        simd4_t r(*this);
        r /= v;
        return r;
    }
    inline simd4_t operator/(simd4_t<double>& v)
    {
        simd4_t r(*this);
        r /= v;
        return r;
    }

    inline simd4_t<double>& operator+=(double d)
    {
# ifdef __SSE2__
        simd4 += d;
# else
        vec[0] += d;
        vec[1] += d;
        vec[2] += d;
        vec[3] += d;
# endif
        return *this;
    }
    inline simd4_t<double>& operator+=(const __vec4d_t v)
    {
        simd4_t r(v);
        *this += r;
        return *this;
    }
    inline simd4_t<double>& operator+=(const simd4_t<double>& v)
    {
# ifdef __SSE2__
        simd4 += v.simd4;
# else
        vec[0] += v[0];
        vec[1] += v[1];
        vec[2] += v[2];
        vec[3] += v[3];
# endif
        return *this;
    }

    inline simd4_t<double>& operator-=(double d)
    {
# ifdef __SSE2__
        simd4 -= d;
# else
        vec[0] -= d;
        vec[1] -= d;
        vec[2] -= d;
        vec[3] -= d;
# endif
        return *this;
    }
    inline simd4_t<double>& operator-=(const __vec4d_t v)
    {
        simd4_t r(v);
        *this -= r;
        return *this;
    }
    inline simd4_t<double>& operator-=(const simd4_t<double>& v)
    {
# ifdef __SSE2__
        simd4 -= v.simd4;
# else
        vec[0] -= v[0];
        vec[1] -= v[1];
        vec[2] -= v[2];
        vec[3] -= v[3];
# endif
        return *this;
    }

    inline simd4_t<double>& operator *=(double d)
    {
# ifdef __SSE2__
        simd4 *= d;
# else
        vec[0] *= d;
        vec[1] *= d;
        vec[2] *= d;
        vec[3] *= d;
# endif
        return *this;
    }
    inline simd4_t<double>& operator*=(const __vec4d_t v)
    {
        simd4_t r(v);
        *this *= r;
        return *this;
    }
    inline simd4_t<double>& operator*=(const simd4_t<double>& v)
    {
# ifdef __SSE2__
        simd4 *= v.simd4;
# else
        vec[0] *= v[0];
        vec[1] *= v[1];
        vec[2] *= v[2];
        vec[3] *= v[3];
# endif
        return *this;
    }

    inline simd4_t<double>& operator/=(double d)
    {
# ifdef __SSE2__
        d = (1.0/d);
        simd4 *= d;
# else
        vec[0] /= d;
        vec[1] /= d;
        vec[2] /= d;
        vec[3] /= d;
# endif
        return *this;
    }
    inline simd4_t<double>& operator/=(const __vec4d_t v)
    {
        simd4_t r(v);
        *this /= r;
        return *this;
    }
    inline simd4_t<double>& operator/=(const simd4_t<double>& v)
    {
# ifdef __SSE2__
        simd4 /= v.simd4;
# else
        vec[0] /= v[0];
        vec[1] /= v[1];
        vec[2] /= v[2];
        vec[3] /= v[3];
# endif
        return *this;
    }

    inline operator const double*() const {
        return vec;
    }

    inline operator double*() {
        return vec;
    }
};


template<>
class simd4_t<int>
{
private:
   typedef int  __vec4i_t[4];

# ifdef __SSE2__
    union {
        __m128i simd4;
        __vec4i_t vec;
    };
# else
    __vec4i_t vec;
# endif

public:
    simd4_t() {}
    simd4_t(int i)
    {
# ifdef __SSE2__
        simd4 = _mm_set1_epi32(i);
# else
        vec[0] = vec[1] = vec[2] = vec[3] = i;
# endif
    }
    explicit simd4_t(const __vec4i_t v)
    {
# ifdef __SSE2__
        simd4 = _mm_loadu_si128((__m128i*)v);
# else
        std::memcpy(vec, v, sizeof(int[4]));
# endif
    }
    simd4_t(const simd4_t<int>& v)
    {
# ifdef __SSE2__
        simd4 = v.simd4;
# else
        std::memcpy(vec, v.vec, sizeof(int[4]));
# endif
    }

    inline int (&ptr(void))[4] {
        return vec;
    }

    inline const int (&ptr(void) const)[4] {
        return vec;
    }

    inline simd4_t<int>& operator=(int i)
    {
# ifdef __SSE2__
        simd4 = _mm_set1_epi32(i);
# else
        vec[0] = vec[1] = vec[2] = vec[3] = i;
# endif
        return *this;
    }
    inline simd4_t<int>& operator=(const __vec4i_t v)
    {
# ifdef __SSE2__
        simd4 = _mm_loadu_si128((__m128i*)v);
# else
        std::memcpy(vec, v, sizeof(int[4]));
# endif
        return *this;
    }
    inline simd4_t<int>& operator=(const simd4_t<int>& v)
    {
# ifdef __SSE2__
        simd4 = v.simd4;
# else
        std::memcpy(vec, v.vec, sizeof(int[4]));
# endif
        return *this;
    }

    inline simd4_t operator+(int i)
    {
        simd4_t r(*this);
        r += i;
        return r;
    }
    inline simd4_t operator+(const __vec4i_t v)
    {
        simd4_t r(*this);
        r += v;
        return r;
    }
    inline simd4_t operator+(const simd4_t<int>& v)
    {
        simd4_t r(*this);
        r += v;
        return r;
    }

    inline simd4_t operator-()
    {
        simd4_t r(0.0f);
        r -= vec;
        return r;
    }
    inline simd4_t operator-(int i)
    {
        simd4_t r(*this);
        r -= i;
        return r;
    }
    inline simd4_t operator-(const __vec4i_t v)
    {
        simd4_t r(*this);
        r -= v;
        return r;
    }
    inline simd4_t operator-(simd4_t<int>& v)
    {
        simd4_t r(*this);
        r -= v;
        return r;
    }

    inline simd4_t operator*(int i)
    {
        simd4_t r(*this);
        r *= i;
        return r;
    }
    inline simd4_t operator*(const __vec4i_t v)
    {
        simd4_t r(*this);
        r *= v;
        return r;
    }
    inline simd4_t operator*(simd4_t<int>& v)
    {
        simd4_t r(*this);
        r *= v;
        return r;
    }

    inline simd4_t operator/(int i)
    {
        simd4_t r(*this);
        r /= i;
        return r;
    }
    inline simd4_t operator/(const __vec4i_t v)
    {
        simd4_t r(*this);
        r /= v;
        return r;
    }
    inline simd4_t operator/(simd4_t<int>& v)
    {
        simd4_t r(*this);
        r /= v;
        return r;
    }

    inline simd4_t<int>& operator+=(int i)
    {
# ifdef __SSE2__
        simd4 += i;
# else
        vec[0] += i;
        vec[1] += i;
        vec[2] += i;
        vec[3] += i;
# endif
        return *this;
    }
    inline simd4_t<int>& operator+=(const __vec4i_t v)
    {
        simd4_t r(v);
        *this += r;
        return *this;
    }
    inline simd4_t<int>& operator+=(const simd4_t<int>& v)
    {
# ifdef __SSE2__
        simd4 += v.simd4;
# else
        vec[0] += v[0];
        vec[1] += v[1];
        vec[2] += v[2];
        vec[3] += v[3];
# endif
        return *this;
    }

    inline simd4_t<int>& operator-=(int i)
    {
# ifdef __SSE2__
        simd4 -= i;
# else
        vec[0] -= i;
        vec[1] -= i;
        vec[2] -= i;
        vec[3] -= i;
# endif
        return *this;
    }
    inline simd4_t<int>& operator-=(const __vec4i_t v)
    {
        simd4_t r(v);
        *this -= r;
        return *this;
    }
    inline simd4_t<int>& operator-=(const simd4_t<int>& v)
    {
# ifdef __SSE2__
        simd4 -= v.simd4;
# else
        vec[0] -= v[0];
        vec[1] -= v[1];
        vec[2] -= v[2];
        vec[3] -= v[3];
# endif
        return *this;
    }

    inline simd4_t<int>& operator *=(int i)
    {
# ifdef __SSE2__
        simd4 *= i;
# else
        vec[0] *= i;
        vec[1] *= i;
        vec[2] *= i;
        vec[3] *= i;
# endif
        return *this;
    }
    inline simd4_t<int>& operator*=(const __vec4i_t v)
    {
        simd4_t r(v);
        *this *= r;
        return *this;
    }
    inline simd4_t<int>& operator*=(const simd4_t<int>& v)
    {
# ifdef __SSE2__
        simd4 *= v.simd4;
# else
        vec[0] *= v[0];
        vec[1] *= v[1];
        vec[2] *= v[2];
        vec[3] *= v[3];
# endif
        return *this;
    }

    inline simd4_t<int>& operator/=(int i)
    {
# ifdef __SSE2__
        i = (1/i);
        simd4 *= i;
# else
        vec[0] /= i;
        vec[1] /= i;
        vec[2] /= i;
        vec[3] /= i;
# endif
        return *this;
    }
    inline simd4_t<int>& operator/=(const __vec4i_t v)
    {
        simd4_t r(v);
        *this /= r;
        return *this;
    }
    inline simd4_t<int>& operator/=(const simd4_t<int>& v)
    {
# ifdef __SSE2__
        simd4 /= v.simd4;
# else
        vec[0] /= v[0];
        vec[1] /= v[1];
        vec[2] /= v[2];
        vec[3] /= v[3];
# endif
        return *this;
    }

    inline operator const int*() const {
        return vec;
    }

    inline operator int*() {
        return vec;
    }
};


#endif /* __SIMD_H__ */

