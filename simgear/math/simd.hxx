// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2016-2017 Erik Hofman <erik@ehofman.com>

#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>
#include <cmath>
#include <new>

#include <simgear/math/SGLimits.hxx>
#include <simgear/math/SGMisc.hxx>

template<typename T, int N> class simd4_t;

namespace simd4
{

template<typename T, int N>
inline simd4_t<T,N> min(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    for (int i=0; i<N; ++i) {
        v1[i] = SGMisc<T>::min(v1[i], v2[i]);
    }
    return v1;
}

template<typename T, int N>
inline simd4_t<T,N> max(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    for (int i=0; i<N; ++i) {
        v1[i] = SGMisc<T>::max(v1[i], v2[i]);
    }
    return v1;
}

template<typename T, int N>
inline simd4_t<T,N> abs(simd4_t<T,N> v) {
    for (int i=0; i<N; ++i) {
        v[i] = std::abs(v[i]);
    }
    return v;
}

template<typename T, int N>
inline T magnitude2(const simd4_t<T,N>& vi) {
    simd4_t<T,N> v(vi);
    T mag2 = 0;
    v = v*v;
    for (int i=0; i<N; ++i) {
        mag2 += v[i];
    }
    return mag2;
}

template<typename T, int N>
inline simd4_t<T,N> interpolate(T tau, const simd4_t<T,N>& v1, const simd4_t<T,N>& v2) {
    return v1 + tau*(v2-v1);
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
inline T dot(const simd4_t<T,N>& v1, const simd4_t<T,N>& v2) {
    simd4_t<T,N> v(v1*v2);
    T dp = T(0);
    for (int i=0; i<N; ++i) {
       dp += v[i];
    }
    return dp;
}

template<typename T>
inline simd4_t<T,3> cross(const simd4_t<T,3>& v1, const simd4_t<T,3>& v2)
{
    simd4_t<T,3> d;
    d[0] = v1[1]*v2[2] - v1[2]*v2[1];
    d[1] = v1[2]*v2[0] - v1[0]*v2[2];
    d[2] = v1[0]*v2[1] - v1[1]*v2[0];
    return d;
}

} /* namespace simd4 */


template<typename T, int N>
class simd4_t final
{
private:
    union {
       T _v4[4];
       T vec[N];
    };

public:
    simd4_t(void) {
        for (int i=0; i<4; ++i) _v4[i] = 0;
    }
    simd4_t(T s) {
        for (int i=0; i<N; ++i) vec[i] = s;
        for (int i=N; i<4; ++i) _v4[i] = 0;
    }
    simd4_t(T x, T y) : simd4_t(x,y,0,0) {}
    simd4_t(T x, T y, T z) : simd4_t(x,y,z,0) {}
    simd4_t(T x, T y, T z, T w) {
        _v4[0] = x; _v4[1] = y; _v4[2] = z; _v4[3] = w;
        for (int i=N; i<4; ++i) _v4[i] = 0;
    }
    explicit simd4_t(const T v[N]) {
        std::memcpy(vec, v, sizeof(T[N]));
        for (int i=N; i<4; ++i) _v4[i] = 0;
    }
    template<int M>
    simd4_t(const simd4_t<T,M>& v) {
        std::memcpy(_v4, v.ptr(), sizeof(T[M]));
        for (int i=(M<N)?M:N; i<4; ++i) _v4[i] = 0;
    }
    ~simd4_t(void) {}   // non-virtual intentional

    inline T (&v4(void))[N] {
        return vec;
    }

    inline const T (&v4(void) const)[N] {
        return vec;
    }

    inline const T (&ptr(void) const)[N] {
        return vec;
    }

    inline T (&ptr(void))[N] {
        return vec;
    }

    inline  const T& operator[](unsigned n) const {
        assert(n<N);
        return vec[n];
    }

    inline T& operator[](unsigned n) {
        assert(n<N);
        return vec[n];
    }

    template<int M>
    inline simd4_t<T,N>& operator=(const simd4_t<T,M>& v) {
        *this = simd4_t<T,N>(v);
        return *this;
    }

    inline simd4_t<T,N>& operator+=(T s) {
        for (int i=0; i<N; ++i) {
            vec[i] += s;
        }
        return *this;
    }
    inline simd4_t<T,N>& operator+=(const T v[N]) {
        for (int i=0; i<N; ++i) {
           vec[i] += v[i];
        }
        return *this;
    }
    inline simd4_t<T,N>& operator+=(const simd4_t<T,N>& v) {
        for (int i=0; i<N; ++i) {
           vec[i] += v[i];
        }
        return *this;
    }

    inline simd4_t<T,N>& operator-=(T s) {
        for (int i=0; i<N; ++i) {
           vec[i] -= s;
        }
        return *this;
    }
    inline simd4_t<T,N>& operator-=(const T v[N]) {
        for (int i=0; i<N; ++i) {
            vec[i] -= v[i];
        }
        return *this;
    }
    inline simd4_t<T,N>& operator-=(const simd4_t<T,N>& v) {
        for (int i=0; i<N; ++i) {
            vec[i] -= v[i];
        }
        return *this;
    }
    inline simd4_t<T,N>& operator*=(T s) {
        for (int i=0; i<N; ++i) {
           vec[i] *= s;
        }
        return *this;
    }
    inline simd4_t<T,N>& operator*=(const T v[N]) {
        for (int i=0; i<N; ++i) {
           vec[i] *= v[i];
        }
        return *this;
    }
    inline simd4_t<T,N>& operator*=(const simd4_t<T,N>& v) {
        for (int i=0; i<N; ++i) {
           vec[i] *= v[i];
        }
        return *this;
    }

    inline simd4_t<T,N>& operator/=(T s) {
        for (int i=0; i<N; ++i) {
           vec[i] /= s;
        }
        return *this;
    }
    inline simd4_t<T,N>& operator/=(const T v[N]) {
        for (int i=0; i<N; ++i) {
           vec[i] /= v[i];
        }
        return *this;
    }
    inline simd4_t<T,N>& operator/=(const simd4_t<T,N>& v) {
        for (int i=0; i<N; ++i) {
           vec[i] /= v[i];
        }
        return *this;
    }
};

template<typename T, int N>
inline simd4_t<T,N> operator-(const simd4_t<T,N>& v) {
    simd4_t<T,N> r(T(0));
    r -= v;
    return r;
}

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
inline simd4_t<T,N> operator/(simd4_t<T,N> v1, const simd4_t<T,N>& v2) {
    v1 /= v2;
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

