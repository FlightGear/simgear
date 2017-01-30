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

#ifndef __SIMD_NEON_H__
#define __SIMD_NEON_H__	1

#ifdef __ARM_NEON__

static const uint32_t m2a32[] alignas(16) = { 0xffffffff,0xffffffff,0,0 };
static const uint32_t m3a32[] alignas(16) = { 0xffffffff,0xffffffff,0xffffffff,0 };

template<int N>
class simd4_t<float,N>
{
private:
   typedef float  __vec4f_t[N];

    union alignas(16) {
        float32x4_t simd4;
        float32x2x2_t simd2x2;
        __vec4f_t vec;
    };

public:
    simd4_t(void) {}
    simd4_t(float f) {}
    simd4_t(float x, float y) : simd4_t(x,y,0,0) {}
    simd4_t(float x, float y, float z) : simd4_t(x,y,z,0) {}
    simd4_t(float x, float y, float z, float w) {
        alignas(16) float data[4] = { x, y, z, w };
        simd4 = vld1q_f32(data);
    }
    simd4_t(const __vec4f_t v) {}
    template<int M>
    simd4_t(const simd4_t<float,M>& v) {
        simd4 = v.v4();
    }
    simd4_t(const float32x4_t& v) {
        simd4 = v;
    }

    inline const float32x2x2_t (&v2x2(void) const) {
        return simd2x2;
    }
    inline float32x2x2_t (&v2x2(void)) {
        return simd2x2;
    }

    inline const float32x4_t (&v4(void) const) {
        return simd4;
    }
    inline float32x4_t (&v4(void)) {
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

    inline simd4_t<float,N>& operator+=(float f) {
        return operator+=(simd4_t<float,N>(f));
    }
    template<int M>
    inline simd4_t<float,N>& operator+=(const simd4_t<float,M>& v) {
        simd4 = vaddq_f32(simd4, v.v4());
        return *this;
    }

    inline simd4_t<float,N>& operator-=(float f) {
        return operator-=(simd4_t<float,N>(f));
    }
    template<int M>
    inline simd4_t<float,N>& operator-=(const simd4_t<float,M>& v) {
        simd4 = vsubq_f32(simd4, v.v4());
        return *this;
    }

    inline simd4_t<float,N>& operator*=(float f) {
        return operator*=(simd4_t<float,N>(f));
    }
    template<int M>
    inline simd4_t<float,N>& operator*=(const simd4_t<float,M>& v) {
        simd4 = vmulq_f32(simd4, v.v4());
        return *this;
    }

    inline simd4_t<float,N>& operator/=(float f) {
        return operator/=(simd4_t<float,4>(f));
    }
    template<int M>
    inline simd4_t<float,N>& operator/=(const simd4_t<float,M>& v) {
// http://stackoverflow.com/questions/6759897/how-to-divide-in-neon-intrinsics-by-a-float-number
        float32x4_t recip = vrecpeq_f32(v.v4());
        recip = vmulq_f32(vrecpsq_f32(v.v4(), recip), recip);
        recip = vmulq_f32(vrecpsq_f32(v.v4(), recip), recip);
        recip = vmulq_f32(vrecpsq_f32(v.v4(), recip), recip);
        simd4 = vmulq_f32(simd4, recip);
        return *this;
    }
};
 
# define vandq_f32(a,b) vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)))

static const float32x4_t fmask2 = vld1q_f32((const float*)m2a32);
static const float32x4_t fmask3 = vld1q_f32((const float*)m3a32);

template<>
inline simd4_t<float,4>::simd4_t(const __vec4f_t v) {
    simd4 = vld1q_f32(v);
}
template<>
inline simd4_t<float,3>::simd4_t(const __vec4f_t v) {
    simd4 = vandq_f32(vld1q_f32(v), fmask3);
}
template<>
inline simd4_t<float,2>::simd4_t(const __vec4f_t v) {
    simd4 = vandq_f32(vld1q_f32(v), fmask2);
}
template<>
inline simd4_t<float,4>::simd4_t(float f) {
    simd4 = vdupq_n_f32(f);
}
template<>
inline simd4_t<float,3>::simd4_t(float f) {
    simd4 = vandq_f32(vdupq_n_f32(f), fmask3);
}

template<>
inline simd4_t<float,2>::simd4_t(float f) {
    simd4 = vandq_f32(vdupq_n_f32(f), fmask2);
}


namespace simd4
{
// http://stackoverflow.com/questions/6931217/sum-all-elements-in-a-quadword-vector-in-arm-assembly-with-neon
inline static float hsum_float32x4_neon(float32x4_t v) {
    float32x2_t r = vadd_f32(vget_high_f32(v), vget_low_f32(v));
    return vget_lane_f32(vpadd_f32(r, r), 0);
}

template<>
inline float magnitude2(simd4_t<float,4> v) {
    return hsum_float32x4_neon(v.v4()*v.v4());
}

template<>
inline float dot(simd4_t<float,4> v1, const simd4_t<float,4>& v2) {
    return hsum_float32x4_neon(v1.v4()*v2.v4());
}

template<>
inline simd4_t<float,3> cross(const simd4_t<float,3>& v1, const simd4_t<float,3>& v2)
{
    // from 0123 to 0213
    float32x2x2_t v1lh_2013 = vzip_f32(v1.v2x2().val[0], v1.v2x2().val[1]);
    float32x2x2_t v2lh_2013 = vzip_f32(v2.v2x2().val[0], v2.v2x2().val[1]);

    // from 0213 to 2013
    v1lh_2013.val[0] = vrev64_f32(v1lh_2013.val[0]);
    v2lh_2013.val[0] = vrev64_f32(v2lh_2013.val[0]);

    float32x4_t v1_2013 = vcombine_f32(v1lh_2013.val[0], v1lh_2013.val[1]);
    float32x4_t v2_2013 = vcombine_f32(v2lh_2013.val[0], v2lh_2013.val[1]);

    // from 2013 to 2103
    float32x2x2_t v1lh_1203 = vzip_f32(v1lh_2013.val[0], v1lh_2013.val[1]);
    float32x2x2_t v2lh_1203 = vzip_f32(v2lh_2013.val[0], v2lh_2013.val[1]);

    // from 2103 to 1203
    v1lh_1203.val[0] = vrev64_f32(v1lh_1203.val[0]);
    v2lh_1203.val[0] = vrev64_f32(v2lh_1203.val[0]);

    float32x4_t v1_1203 = vcombine_f32(v1lh_1203.val[0], v1lh_1203.val[1]);
    float32x4_t v2_1203 = vcombine_f32(v2lh_1203.val[0], v2lh_1203.val[1]);

    // calculate the cross product
    return vsubq_f32(vmulq_f32(v1_1203,v2_2013),vmulq_f32(v1_2013,v2_1203));
}


template<int N>
inline simd4_t<float,N> min(simd4_t<float,N> v1, const simd4_t<float,N>& v2) {
    v1 = vminq_f32(v1.v4(), v2.v4());
    return v1;
}

template<int N>
inline simd4_t<float,N> max(simd4_t<float,N> v1, const simd4_t<float,N>& v2) {
    v1 = vmaxq_f32(v1.v4(), v2.v4());
    return v1;
}

template<int N>
inline simd4_t<float,N>abs(simd4_t<float,N> v) {
    return vabsq_f32(v.v4());
}

} /* namsepace simd4 */


// TODO: 64-bit support for doubles
#if 0
template<int N>
class simd4_t<double,N>
{
private:
   typedef double  __vec4d_t[N];

    union alignas(32) {
        __m256d simd4;
        __vec4d_t vec;
    };

public:
    simd4_t(void) {}
    simd4_t(double d) {}
    simd4_t(double x, double y) : simd4_t(x,y,0,0) {}
    simd4_t(double x, double y, double z) : simd4_t(x,y,z,0) {}
    simd4_t(double x, double y, double z, double w) {
        simd4 = _mm256_set_pd(w,z,y,x);
    }
    simd4_t(const __vec4d_t v) { {}
    template<int M>
    simd4_t(const simd4_t<double,M>& v) {
        simd4 = v.v4();
    }
    simd4_t(const __m256d& v) {
        simd4 = v;
    }

    inline const __m256d (&v4(void) const) {
        return simd4;
    }
    inline __m256d (&v4(void)) {
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

    inline simd4_t<double,N>& operator+=(double d) {
        return operator+=(simd4_t<double,N>(d));
    }
    template<int M>
    inline simd4_t<double,N>& operator+=(const simd4_t<double,M>& v) {
        simd4 = _mm256_add_pd(simd4, v.v4());
        return *this;
    }

    inline simd4_t<double,N>& operator-=(double d) {
        return operator-=(simd4_t<double,N>(d));
    }
    template<int M>
    inline simd4_t<double,N>& operator-=(const simd4_t<double,M>& v) {
        simd4 = _mm256_sub_pd(simd4, v.v4());
        return *this;
    }

    inline simd4_t<double,N>& operator*=(double d) {
        return operator*=(simd4_t<double,N>(d));
    }
    template<int M>
    inline simd4_t<double,N>& operator*=(const simd4_t<double,M>& v) {
        simd4 = _mm256_mul_pd(simd4, v.v4());
        return *this;
    }

    inline simd4_t<double,N>& operator/=(double d) {
        return operator/=(simd4_t<double,4>(d));
    }
    template<int M>
    inline simd4_t<double,N>& operator/=(const simd4_t<double,M>& v) {
        simd4 = _mm256_div_pd(simd4, v.v4());
        return *this;
    }
};

static const __m256d dmask2 = _mm256_load_pd((const double*)m2a64);
static const __m256d dmask3 = _mm256_load_pd((const double*)m3a64);

template<>
inline simd4_t<double,4>::simd4_t(const __vec4d_t v) {
    simd4 = _mm256_loadu_pd(v);
}
template<>
inline simd4_t<double,3>::simd4_t(const __vec4d_t v) {
    simd4 = _mm256_and_pd(_mm256_loadu_pd(v), dmask3);
}
template<>
inline simd4_t<double,2>::simd4_t(const __vec4d_t v) {
    simd4 = _mm256_and_pd(_mm256_loadu_pd(v), dmask2);
}
template<>
inline simd4_t<double,4>::simd4_t(double d) {
    simd4 = _mm256_set1_pd(d);
}
template<>
inline simd4_t<double,3>::simd4_t(double d) {
    simd4 = _mm256_and_pd(_mm256_set1_pd(d), dmask3);
}
template<>
inline simd4_t<double,2>::simd4_t(double d) {
    simd4 = _mm256_and_pd(_mm256_set1_pd(d), dmask2);
}

namespace simd4
{
// http://berenger.eu/blog/sseavxsimd-horizontal-sum-sum-simd-vector-intrinsic/
inline static float hsum_pd_avx(__m256d v) {
    const float64x4_t valupper = _mm256_extractf128_pd(v, 1);
    const float64x4_t vallower = _mm256_castpd256_pd128(v);
    _mm256_zeroupper();
    const float64x4_t valval = _mm_add_pd(valupper, vallower);
    const float64x4_t sums =   _mm_add_pd(_mm_permute_pd(valval,1), valval);
    return                 _mm_cvtsd_f64(sums);
}

template<>
inline double magnitude2(simd4_t<double,4> v) {
    return hsum_pd_avx(_mm256_mul_pd(v.v4(),v.v4()));
}

template<>
inline double dot(simd4_t<double,4> v1, const simd4_t<double,4>& v2) {
    return hsum_pd_avx(_mm256_mul_pd(v1.v4(),v2.v4()));
}

template<>
inline simd4_t<double,3> cross(const simd4_t<double,3>& v1, const simd4_t<double,3>& v2)
{
    // https://gist.github.com/L2Program/219e07581e69110e7942
    __m256d v41 = v1.v4(), v42 = v2.v4();
    return _mm256_sub_pd(
               _mm256_mul_pd(
                     _mm256_permute4x64_pd(v41,_MM_SHUFFLE(3, 0, 2, 1)),
                     _mm256_permute4x64_pd(v42,_MM_SHUFFLE(3, 1, 0, 2))),
               _mm256_mul_pd(
                     _mm256_permute4x64_pd(v41,_MM_SHUFFLE(3, 1, 0, 2)),
                     _mm256_permute4x64_pd(v42,_MM_SHUFFLE(3, 0, 2, 1))));
}

template<int N>
inline simd4_t<double,N> min(simd4_t<double,N> v1, const simd4_t<double,N>& v2) {
    v1 = _mm256_min_pd(v1.v4(), v2.v4());
    return v1;
}

template<int N>
inline simd4_t<double,N> max(simd4_t<double,N> v1, const simd4_t<double,N>& v2) {
    v1 = _mm256_max_pd(v1.v4(), v2.v4());
    return v1;
}

template<int N>
inline simd4_t<double,N>abs(simd4_t<double,N> v) {
    static const __m256d sign_mask = _mm256_set1_pd(-0.); // -0. = 1 << 63
    v = _mm256_andnot_pd(sign_mask, v.v4());
    return v;
}

} /* namespace simd4 */
#endif


template<int N>
class simd4_t<int,N>
{
private:
   typedef int  __vec4i_t[N];

    union alignas(16) {
        int32x4_t simd4;
        __vec4i_t vec;
    };

public:
    simd4_t(void) {}
    simd4_t(int i) {}
    simd4_t(int x, int y) : simd4_t(x,y,0,0) {}
    simd4_t(int x, int y, int z) : simd4_t(x,y,z,0) {}
    simd4_t(int x, int y, int z, int w) {
        alignas(16) int32_t data[4] = { x, y, z, w };
        simd4 = vld1q_s32(data);
    }
    simd4_t(const __vec4i_t v) {}
    template<int M>
    simd4_t(const simd4_t<int,M>& v) {
        simd4 = v.v4();
    }
    simd4_t(const int32x4_t& v) {
        simd4 = v;
    }

    inline int32x4_t (&v4(void)) {
        return simd4;
    }

    inline const int32x4_t (&v4(void) const) {
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

    inline simd4_t<int,N>& operator+=(int i) {
        return operator+=(simd4_t<int,N>(i));
    }
    template<int M>
    inline simd4_t<int,N>& operator+=(const simd4_t<int,M>& v) {
        simd4 = vaddq_s32(simd4, v.v4());
        return *this;
    }

    inline simd4_t<int,N>& operator-=(int i) {
        return operator-=(simd4_t<int,N>(i));
    }
    template<int M>
    inline simd4_t<int,N>& operator-=(const simd4_t<int,M>& v) {
        simd4 = vsubq_s32(simd4, v.v4());
        return *this;
    }

    inline simd4_t<int,N>& operator*=(int i) {
        return operator*=(simd4_t<int,N>(i));
    }
    template<int M>
    inline simd4_t<int,N>& operator*=(const simd4_t<int,M>& v) {
         return operator*=(v.v4());
    }
    inline simd4_t<int,N>& operator*=(const int32x4_t& v) {
        simd4 = vmulq_s32(simd4, v);
        return *this;
    }

    inline simd4_t<int,N>& operator/=(int s) {
        return operator/=(simd4_t<int,4>(s));
    }
    template<int M>
    inline simd4_t<int,N>& operator/=(const simd4_t<int,M>& v) {
        for (int i=0; i<N; ++i) {
           vec[i] /= v[i];
        }
        return *this;
    }
};

static const int32x4_t imask2 = vld1q_s32((int32_t*)m2a32);
static const int32x4_t imask3 = vld1q_s32((int32_t*)m3a32);

template<>
inline simd4_t<int,4>::simd4_t(const __vec4i_t v) {
    simd4 = vld1q_s32(v);
}
template<>
inline simd4_t<int,3>::simd4_t(const __vec4i_t v) {
    simd4 = vandq_s32(vld1q_s32(v), imask3);
}
template<>
inline simd4_t<int,2>::simd4_t(const __vec4i_t v) {
    simd4 = vandq_s32(vld1q_s32(v), imask2);
}
template<>
inline simd4_t<int,4>::simd4_t(int i) {
    simd4 = vdupq_n_s32(i);
}
template<>
inline simd4_t<int,3>::simd4_t(int i) {
    simd4 = vandq_s32(vdupq_n_s32(i), imask3);
}
template<>
inline simd4_t<int,2>::simd4_t(int i) {
    simd4 = vandq_s32(vdupq_n_s32(i), imask2);
}


namespace simd4
{

template<int N>
inline simd4_t<int,N> min(simd4_t<int,N> v1, const simd4_t<int,N>& v2) {
    v1 = vminq_s32(v1.v4(), v2.v4());
    return v1;
}

template<int N>
inline simd4_t<int,N> max(simd4_t<int,N> v1, const simd4_t<int,N>& v2) {
    v1 = vmaxq_s32(v1.v4(), v2.v4());
    return v1;
}

} /* namespace simd4 */

# endif

#endif /* __SIMD_NEON_H__ */

