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

#ifndef __SIMD4X4_NEON_H__
#define __SIMD4X4_NEON_H__	1

#include "simd_neon.hxx"

#ifdef __ARM_NEON__
template<>
class simd4x4_t<float,4>
{
private:
    typedef float  __mtx4f_t[4][4];

    union alignas(16) {
        float32x4_t simd4x4[4];
        __mtx4f_t mtx;
        float array[4*4];
    }g;

public:
    simd4x4_t(void) {}
    simd4x4_t(float m00, float m01, float m02, float m03,
              float m10, float m11, float m12, float m13,
              float m20, float m21, float m22, float m23,
              float m30, float m31, float m32, float m33)
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
    simd4x4_t(const float m[4*4]) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = simd4_t<float,4>((const float*)&m[4*i]).v4();
        }
    }

    explicit simd4x4_t(const __mtx4f_t m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = simd4_t<float,4>(m[i]).v4();
        }
    }
    simd4x4_t(const simd4x4_t<float,4>& m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = m.m4x4()[i];
        }
    }
    ~simd4x4_t(void) {}

    inline float32x4_t (&m4x4(void))[4] {
        return simd4x4;
    }

    inline const float32x4_t (&m4x4(void) const)[4] {
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

    inline simd4x4_t<float,4>& operator=(const __mtx4f_t m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = simd4_t<float,4>(m[i]).v4();
        }
        return *this;
    }
    inline simd4x4_t<float,4>& operator=(const simd4x4_t<float,4>& m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = m.m4x4()[i];
        }
        return *this;
    }

    inline simd4x4_t<float,4>& operator+=(const simd4x4_t<float,4>& m) {
        for (int i=0; i<4; ++i) {
           simd4x4[i] = vaddq_f32(simd4x4[i], m.m4x4()[i]);
        }
        return *this;
    }

    inline simd4x4_t<float,4>& operator-=(const simd4x4_t<float,4>& m) {
        for (int i=0; i<4; ++i) {
           simd4x4[i] = vsubq_f32(simd4x4[i], m.m4x4()[i]);
        }
        return *this;
    }

    inline simd4x4_t<float,4>& operator*=(float f) {
        simd4_t<float,4> f4(f);
        for (int i=0; i<4; ++i) {
           simd4x4[i] = vmulq_f32(simd4x4[i], f4.v4());
        }
        return *this;
    }

    simd4x4_t<float,4>& operator*=(const simd4x4_t<float,4>& m2) {
        simd4x4_t<float,4> m1 = *this;
        float32x4_t row, col;
        for (int i=0; i<4; ++i) {
            col = vdupq_n_f32(m2.ptr()[i][0]);
            row = vmulq_f32(m1.m4x4()[0], col);
            for (int j=1; j<4; ++j) {
                col = vdupq_n_f32(m2.ptr()[i][j]);
                row = vaddq_f32(row, vmulq_f32(m1.m4x4()[j], col));
            }
            simd4x4[i] = row;
        }
        return *this;
    }
};

template<int M>
inline simd4_t<float,M> operator*(const simd4x4_t<float,4>& m, const simd4_t<float,M>& vi)
{
    float32x4_t mv = vmulq_f32(m.m4x4()[0], vdupq_n_f32(vi.ptr()[0]));
    for (int i=1; i<M; ++i) {
        float32x4_t row = vmulq_f32(m.m4x4()[i], vdupq_n_f32(vi.ptr()[i]));
        mv = vaddq_f32(mv, row);
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
// http://clb.demon.fi/MathGeoLib/nightly/docs/float4x4_neon.h_code.html
    float32x4x4_t x = vld4q_f32(m);
    vst1q_f32(&m[0], x.val[0]);
    vst1q_f32(&m[4], x.val[1]);
    vst1q_f32(&m[8], x.val[2]);
    vst1q_f32(&m[12], x.val[3]);
    return m;
}

inline void translate(simd4x4_t<float,4>& m, const simd4_t<float,3>& dist) {
    m.m4x4()[3] = vsubq_f32(m.m4x4()[3], dist.v4());
}

template<typename S>
inline void pre_translate(simd4x4_t<float,4>& m, const simd4_t<S,3>& dist)
{
    simd4x4_t<float,4> mt = simd4x4::transpose(m);
    float32x4_t row3 = mt.m4x4()[3];
    for (int i=0; i<3; ++i) {
        float32x4_t t = vdupq_n_f32(float(dist[i]));
        mt.m4x4()[i] = vaddq_f32(mt.m4x4()[i], vmulq_f32(t, row3));
    }
    m = simd4x4::transpose(mt);
}

template<typename S>
inline void post_translate(simd4x4_t<float,4>& m, const simd4_t<S,3>& dist)
{
    float32x4_t col3 = m.m4x4()[3];
    for (int i=0; i<3; ++i) {
        float32x4_t t = vdupq_n_f32(float(dist[i]));
        col3 = vaddq_f32(col3, vmulq_f32(t, m.m4x4()[i]));
    }
    m.m4x4()[3] = col3;
}

template<>
inline simd4_t<float,3> transform<float>(const simd4x4_t<float,4>& m, const simd4_t<float,3>& pt) {
    float32x4_t tpt = m.m4x4()[3];
    for (int i=0; i<3; ++i) {
        float32x4_t ptd = vdupq_n_f32(pt[i]);
        tpt = vaddq_f32(tpt, vmulq_f32(ptd, m.m4x4()[i]));
    }
    vsetq_lane_f32(0.0f, tpt, 3);
    return tpt;
}

} /* namespace simd4x */



#if 0

template<>
class simd4x4_t<double,4>
{
private:
    typedef double  __mtx4d_t[4][4];

    union alignas(32) {
        __m256d simd4x4[4];
        __mtx4d_t mtx;
        double array[4*4];
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
    explicit simd4x4_t(const double m[4*4]) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = simd4_t<double,4>((const double*)&m[4*i]).v4();
        }
    }

    explicit simd4x4_t(const __mtx4d_t m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = simd4_t<double,4>(m[i]).v4();
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

    inline simd4x4_t<double,4>& operator=(const __mtx4d_t m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = simd4_t<double,4>(m[i]).v4();
        }
        return *this;
    }
    inline simd4x4_t<double,4>& operator=(const simd4x4_t<double,4>& m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = m.m4x4()[i];
        }
        return *this;
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
        simd4_t<double,4> f4(f);
        for (int i=0; i<4; ++i) {
           simd4x4[i] = _mm256_mul_pd(simd4x4[i], f4.v4());
        }
        return *this;
    }

    simd4x4_t<double,4>& operator*=(const simd4x4_t<double,4>& m2) {
        simd4x4_t<double,4> m1 = *this;
        __m256d row, col;
        for (int i=0; i<4; ++i ) {
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
    __mm256d row3 = mt.m4x4()[3];
    for (int i=0; i<3; ++i) {
        __mm256d _mm256_set1_pd(float(dist[i]));
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
    res[3] = 0.0;
    return res;
}

} /* namespace simd4x4 */
# endif


template<>
class simd4x4_t<int,4>
{
private:
    typedef int  __mtx4i_t[4][4];

    union alignas(16) {
        int32x4_t simd4x4[4];
        __mtx4i_t mtx;
        int array[4*4];
    }g;

public:
    simd4x4_t(void) {}
    simd4x4_t(int m00, int m01, int m02, int m03,
              int m10, int m11, int m12, int m13,
              int m20, int m21, int m22, int m23,
              int m30, int m31, int m32, int m33)
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
    simd4x4_t(const int m[4*4]) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = simd4_t<int,4>((const int*)&m[4*i]).v4();
        }
    }
    explicit simd4x4_t(const __mtx4i_t m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = simd4_t<int,4>(m[i]).v4();
        }
    }
    simd4x4_t(const simd4x4_t<int,4>& m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = m.m4x4()[i];
        }
    }
    ~simd4x4_t(void) {}

    inline int32x4_t (&m4x4(void))[4] {
        return simd4x4;
    }

    inline const int32x4_t (&m4x4(void) const)[4] {
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

    inline simd4x4_t<int,4>& operator=(const __mtx4i_t m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = simd4_t<int,4>(m[i]).v4();
        }
        return *this;
    }
    inline simd4x4_t<int,4>& operator=(const simd4x4_t<int,4>& m) {
        for (int i=0; i<4; ++i) {
            simd4x4[i] = m.m4x4()[i];
        }
        return *this;
    }

    inline simd4x4_t<int,4>& operator+=(const simd4x4_t<int,4>& m) {
        for (int i=0; i<4; ++i) {
           simd4x4[i] += m.m4x4()[i];
        }
        return *this;
    }

    inline simd4x4_t<int,4>& operator-=(const simd4x4_t<int,4>& m) {
        for (int i=0; i<4; ++i) {
           simd4x4[i] -= m.m4x4()[i];
        }
        return *this;
    }

    inline simd4x4_t<int,4>& operator*=(int f) {
        simd4_t<int,4> f4(f);
        for (int i=0; i<4; ++i) {
           simd4x4[i] *= f4.v4();
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
    int32x4x4_t x = vld4q_s32(m);
    vst1q_s32(&m[0], x.val[0]);
    vst1q_s32(&m[4], x.val[1]);
    vst1q_s32(&m[8], x.val[2]);
    vst1q_s32(&m[12], x.val[3]);
    return m;
}

inline void translate(simd4x4_t<int,4>& m, const simd4_t<int,3>& dist) {
    m.m4x4()[3] = vsubq_s32(m.m4x4()[3], dist.v4());
}

template<typename S>
inline void pre_translate(simd4x4_t<int,4>& m, const simd4_t<S,3>& dist)
{
    simd4x4_t<int,4> mt = simd4x4::transpose(m);
    simd4_t<int,4> row3(mt.ptr()[3]);
    for (int i=0; i<3; ++i) {
        simd4_t<int,4> trow3 = int(dist[i]);
        trow3 *= row3.v4();
        mt.m4x4()[i] = vaddq_s32(mt.m4x4()[i], trow3.v4());
    }
    m = simd4x4::transpose(mt);
}

template<typename S>
inline void post_translate(simd4x4_t<int,4>& m, const simd4_t<S,3>& dist)
{
    int32x4_t col3 = m.m4x4()[3];
    for (int i=0; i<3; ++i) {
        simd4_t<int,4> trow3 = int(dist[i]);
        trow3 *= m.m4x4()[i];
        col3 = vaddq_s32(col3, trow3.v4());
    }
    m.m4x4()[3] = col3;
}

template<>
inline simd4_t<int,3> transform<int>(const simd4x4_t<int,4>& m, const simd4_t<int,3>& pt) {
    simd4_t<int,3> tpt = m.m4x4()[3];
    for (int i=0; i<3; ++i) {
        simd4_t<int,3> ptd = m.m4x4()[i];
        ptd *= pt[i];
        tpt.v4() = vaddq_s32(tpt.v4(), ptd.v4());
    }
    tpt[3] = 0.0;
    return tpt;
}


} /* namespace simd4x */
#endif

#endif /* __SIMD4X4_NEON_H__ */

