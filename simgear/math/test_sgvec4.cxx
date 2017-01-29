
#include <stdio.h>
#include <cmath>
#include <cfloat>

#include "SGMathFwd.hxx"
#include "SGVec2.hxx"
#include "SGVec3.hxx"
#include "SGVec4.hxx"

// set to 0 for a timing test
#define TESTV_ACCURACY	1

#define N               4
#define T               double

#if TESTV_ACCURACY
# define TESTF(a, b) \
 if ( std::abs(a - b) > T(1e-7) ) \
   printf("line: %i, diff: %5.4e\n", __LINE__, double(std::abs(a - b)));
# define TESTV(a, p,q,r,s) \
 if (   std::abs(a[0] - (p)) > T(1e-7) \
     || std::abs(a[1] - (q)) > T(1e-7) \
     || (N > 2 && std::abs(a[2] - (r)) > T(1e-7)) \
     || (N > 3 && std::abs(a[3] - (s)) > T(1e-7)) \
    ) \
  printf("line: %i, diff: %5.4e, %5.4e, %5.4e, %5.4e\n", __LINE__, double(std::abs(a[0]-(p))), double(std::abs(a[1]-(q))), double(std::abs(a[2]-(r))), double(std::abs(a[3]-(s))));
# define MAX     1

#else
# define TESTF(a, b)
# define TESTV(a, p,q,r,s)
# define MAX     1000000
#endif

#define _VEC(NSTR)	SGVec ## NSTR
#define VEC(N)		_VEC(N)

int main()
{
  T init[4] = { T(1.31), T(3.43), T(5.69), T(1.0) };
  T p[4] = { T(1.03), T(3.55), T(2.707), T(-4.01) };
  float res = 0;
  long int i;
  VEC(N)<T> q;

  for (int i=0; i<N; i++) {
      q[i] = init[i];
  }

  simd4_t<T,N> qq, rr(T(-2.31), T(3.43), T(-4.69), T(-1.0));
  TESTV(rr, T(-2.31), T(3.43), T(-4.69), T(-1.0));

  qq = simd4::min(rr, simd4_t<T,N>(T(0)));
  TESTV(qq, T(-2.31), 0, T(-4.69), T(-1.00));

  qq = simd4::max(rr, simd4_t<T,N>(0.0));
  TESTV(qq, 0, T(3.43), 0, 0);

  qq = simd4::abs(rr);
  TESTV(qq, T(2.31), T(3.43), T(4.69), T(1.00));

  for (i=0; i<MAX; i++)
  {
    VEC(N)<T> v(p);
    VEC(N)<T> x(v), y(v);
    T f, g;

    TESTV(x, p[0], p[1], p[2], p[3]);
    TESTV(y, p[0], p[1], p[2], p[3]);
 
    x += q;
    TESTV(x, p[0]+q[0], p[1]+q[1], p[2]+q[2], p[3]+q[3]);

    y -= q;
    TESTV(y, p[0]-q[0], p[1]-q[1], p[2]-q[2], p[3]-q[3]);

    v = x; x *= T(1.7);
    TESTV(x, v[0]*T(1.7), v[1]*T(1.7), v[2]*T(1.7), v[3]*T(1.7));

    v = y; y /= T(-1.3);
    TESTV(y, v[0]/T(-1.3), v[1]/T(-1.3), v[2]/T(-1.3), v[3]/T(-1.3));

    v = +x;
    TESTV(v, x[0], x[1], x[2], x[3]);

    v = -x;
    TESTV(v, -x[0], -x[1], -x[2], -x[3]);

    v = y+x;
    TESTV(v, y[0]+x[0], y[1]+x[1], y[2]+x[2], y[3]+x[3]);

    y = v*T(1.7);
    TESTV(y, v[0]*T(1.7), v[1]*T(1.7), v[2]*T(1.7), v[3]*T(1.7));

    y = T(1.7)*v;
    TESTV(y, v[0]*T(1.7), v[1]*T(1.7), v[2]*T(1.7), v[3]*T(1.7));

    v = y-x;
    TESTV(v, y[0]-x[0], y[1]-x[1], y[2]-x[2], y[3]-x[3]);

    f = norm(v); g = 0;
    for (int i=0; i<N; i++) g += v[i]*v[i];
    g = std::sqrt(g);
    TESTF(f, g);

    x = v; v -= y;
    VEC(N)<T> t = x - y;
    f = norm(v); g = 0;
    for (int i=0; i<N; i++) g += t[i]*t[i];
    g = std::sqrt(g);
    TESTF(f, g);

    f += dot(x, v); // g = 0;
    for (int i=0; i<N; i++) g += x[i]*v[i];
    TESTF(f, g);

    f += dot(v, v); // g = 0;
    for (int i=0; i<N; i++) g += v[i]*v[i];
    TESTF(f, g);

    res += f;
  }

  printf("res: %f\n", res);
  return 0;
}
