
#ifndef __SIMD_H__
#define __SIMD_H__	1

# ifdef __SSE__
#  include <xmmintrin.h>
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
   typedef float  __vec4_t[4];

# ifdef __SSE__
    union {
        __m128 v4;
        __vec4_t vec;
    };
# else
    __vec4_t vec;
# endif

public:
    simd4_t() {}
    simd4_t(float f)
    {
        vec[0] = vec[1] = vec[2] = vec[3] = f;
    }
    simd4_t(const __vec4_t& v)
    {
        vec[0] = v[0];
        vec[1] = v[1];
        vec[2] = v[2];
        vec[3] = v[3];
    }

    float (&ptr(void))[4] {
        return vec;
    }

    inline const float (&ptr(void) const)[4] {
        return vec;
    }

    inline simd4_t& operator=(float f)
    {
        vec[0] = vec[1] = vec[2] = vec[3] = f;
        return *this;
    }

    inline simd4_t& operator=(const __vec4_t& v)
    {
        vec[0] = v[0];
        vec[1] = v[1];
        vec[2] = v[2];
        vec[3] = v[3];
        return *this;
    }

    inline simd4_t& operator=(const simd4_t& v)
    {
        vec[0] = v[0];
        vec[1] = v[1];
        vec[2] = v[2];
        vec[3] = v[3];
        return *this;
    }

    inline simd4_t operator-()
    {
        simd4_t r(0.0f);
        r -= vec;
        return r;
    }

    inline simd4_t operator+(float f)
    {
        simd4_t r(vec);
        r += f;
        return r;
    }

    inline simd4_t operator-(float f)
    {
        simd4_t r(vec);
        r -= f;
        return r;
    }

    inline simd4_t operator*(float f)
    {
        simd4_t r(vec);
        r *= f;
        return r;
    }

    inline simd4_t operator/(float f)
    {
        simd4_t r(vec);
        r /= f;
        return r;
    }

    inline simd4_t& operator +=(float f)
    {
# ifdef __SSE__
        v4 += f;
# else
        vec[0] += f;
        vec[1] += f;
        vec[2] += f;
        vec[3] += f;
# endif
        return *this;
    }

    inline simd4_t& operator -=(float f)
    {
# ifdef __SSE__
        v4 -= f;
# else
        vec[0] -= f;
        vec[1] -= f;
        vec[2] -= f;
        vec[3] -= f;
# endif
        return *this;
    }

    inline simd4_t& operator *=(float f)
    {
# ifdef __SSE__
        v4 *= f;
# else
        vec[0] *= f;
        vec[1] *= f;
        vec[2] *= f;
        vec[3] *= f;
# endif
        return *this;
    }

    inline simd4_t& operator /=(float f)
    {
# ifdef __SSE__
        v4 /= f;
# else
        vec[0] /= f;
        vec[1] /= f;
        vec[2] /= f;
        vec[3] /= f;
#endif
        return *this;
    }

    inline simd4_t& operator +=(__vec4_t v)
    {
        vec[0] += v[0];
        vec[1] += v[1];
        vec[2] += v[2];
        vec[3] += v[3];
        return *this;
    }

    inline simd4_t& operator -=(__vec4_t v)
    {
        vec[0] -= v[0];
        vec[1] -= v[1];
        vec[2] -= v[2];
        vec[3] -= v[3];
        return *this;
    }

    inline simd4_t& operator *=(__vec4_t v)
    {
        vec[0] *= v[0];
        vec[1] *= v[1];
        vec[2] *= v[2];
        vec[3] *= v[3];
        return *this;
    }

    inline simd4_t& operator /=(__vec4_t v)
    {
        vec[0] /= v[0];
        vec[1] /= v[1];
        vec[2] /= v[2];
        vec[3] /= v[3];
        return *this;
    }

    operator const float*() const {
        return vec;
    }

    operator float*() {
        return vec;
    }

    operator void*() {
        return vec;
    }
};

template<>
class simd4_t<double>
{
private:
   typedef double  __vec4_t[4];

# ifdef __SSE__
    union {
        __m128d v4;
        __vec4_t vec;
    };
# else
    __vec4_t vec;
# endif

public:
    simd4_t() {}
    simd4_t(double f)
    {
        vec[0] = vec[1] = vec[2] = vec[3] = f;
    }
    simd4_t(const __vec4_t& v)
    {
        vec[0] = v[0];
        vec[1] = v[1];
        vec[2] = v[2];
        vec[3] = v[3];
    }

    inline double (&ptr(void))[4] {
        return vec;
    }

    inline const double (&ptr(void) const)[4] {
        return vec;
    }

    inline simd4_t& operator=(double f)
    {
        vec[0] = vec[1] = vec[2] = vec[3] = f;
        return *this;
    }

    inline simd4_t& operator=(const __vec4_t& v)
    {
        vec[0] = v[0];
        vec[1] = v[1];
        vec[2] = v[2];
        vec[3] = v[3];
        return *this;
    }

    inline simd4_t& operator=(const simd4_t& v)
    {
        simd4_t(r);
        vec[0] = v[0];
        vec[1] = v[1];
        vec[2] = v[2];
        vec[3] = v[3];
        return *this;
    }

    inline simd4_t operator-()
    {
        simd4_t r(0.0f);
        r -= vec;
        return r;
    }

    inline simd4_t operator+(double f)
    {
        simd4_t r(vec);
        r += f;
        return r;
    }

    inline simd4_t operator-(double f)
    {
        simd4_t r(vec);
        r -= f;
        return r;
    }

    inline simd4_t operator*(double f)
    {
        simd4_t r(vec);
        r *= f;
        return r;
    }

    inline simd4_t operator/(double f)
    {
        simd4_t r(vec);
        r /= f;
        return r;
    }

    inline simd4_t& operator +=(double f)
    {
# ifdef __SSE__
        v4 += f;
# else
        vec[0] += f;
        vec[1] += f;
        vec[2] += f;
        vec[3] += f;
# endif
        return *this;
    }

    inline simd4_t& operator -=(double f)
    {
# ifdef __SSE__
        v4 -= f;
# else
        vec[0] -= f;
        vec[1] -= f;
        vec[2] -= f;
        vec[3] -= f;
# endif
        return *this;
    }

    inline simd4_t& operator *=(double f)
    {
# ifdef __SSE__
        v4 *= f;
# else
        vec[0] *= f;
        vec[1] *= f;
        vec[2] *= f;
        vec[3] *= f;
# endif
        return *this;
    }

    inline simd4_t& operator /=(double f)
    {
# ifdef __SSE__
        v4 /= f;
# else
        vec[0] /= f;
        vec[1] /= f;
        vec[2] /= f;
        vec[3] /= f;
#endif
        return *this;
    }

    inline simd4_t& operator +=(__vec4_t v)
    {
        vec[0] += v[0];
        vec[1] += v[1];
        vec[2] += v[2];
        vec[3] += v[3];
        return *this;
    }

    inline simd4_t& operator -=(__vec4_t v)
    {
        vec[0] -= v[0];
        vec[1] -= v[1];
        vec[2] -= v[2];
        vec[3] -= v[3];
        return *this;
    }

    inline simd4_t& operator *=(__vec4_t v)
    {
        vec[0] *= v[0];
        vec[1] *= v[1];
        vec[2] *= v[2];
        vec[3] *= v[3];
        return *this;
    }

    inline simd4_t& operator /=(__vec4_t v)
    {
        vec[0] /= v[0];
        vec[1] /= v[1];
        vec[2] /= v[2];
        vec[3] /= v[3];
        return *this;
    }

    operator const double*(void) const {
        return vec;
    }

    operator double*(void) {
        return vec;
    }
};

#if 0
vector operator+(const vector& v);
vector operator+(const vector& v1, const vector& v2);
#endif

#endif /* __SIMD_H__ */

