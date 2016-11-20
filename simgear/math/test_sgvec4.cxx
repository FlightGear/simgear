
#include <stdio.h>
#include <cmath>
#include <cfloat>

#include "SGVec3.hxx"
#include "SGVec4.hxx"

// set to 0 for a timing test
#define TEST_ACCURACY	1

#if TEST_ACCURACY
# define TEST(a, p,q,r,s) \
 if (a[0] != (p) || a[1] != (q) || a[2] != (r) || a[3] != (s)) \
  printf("line: %i, diff: %5.4e, %5.4e, %5.4e, %5.4e\n", __LINE__, fabsf(a[0]-(p)), fabsf(a[1]-(q)), fabsf(a[2]-(r)), fabsf(a[3]-(s)));
# define MAX     1

#else
# define TEST(a, p,q,r,s)
# define MAX     1000000
#endif

#define T	double

int main()
{
  T p[4] = { 1.03, 0.55, 0.707, -0.01 };
  const SGVec4<T> q( 0.31, 0.43, 0.69, 1.0 );
  long int i;

  for (i=0; i<MAX; i++)
  {
    SGVec4<T> v(p);
    SGVec4<T> x(v), y(v);
    T f, g;

    TEST(x, p[0], p[1], p[2], p[3]);
 
    x += q;
    TEST(x, p[0]+q[0], p[1]+q[1], p[2]+q[2], p[3]+q[3]);

    y -= q;
    TEST(y, p[0]-q[0], p[1]-q[1], p[2]-q[2], p[3]-q[3]);

    v = x; x *= 1.7;
    TEST(x, v[0]*1.7, v[1]*1.7, v[2]*1.7, v[3]*1.7);

    v = y; y /= -.3;
    TEST(y, v[0]/-.3, v[1]/-.3, v[2]/-.3, v[3]/-.3);

    v = +x;
    TEST(v, x[0], x[1], x[2], x[3]);

    v = -x;
    TEST(v, -x[0], -x[1], -x[2], -x[3]);

    v = y+x;
    TEST(v, y[0]+x[0], y[1]+x[1], y[2]+x[2], y[3]+x[3]);

    y = v*1.7;
    TEST(y, v[0]*1.7, v[1]*1.7, v[2]*1.7, v[3]*1.7);

    y = 1.7*v;
    TEST(y, v[0]*1.7, v[1]*1.7, v[2]*1.7, v[3]*1.7);

    v = y-x;
    TEST(v, y[0]-x[0], y[1]-x[1], y[2]-x[2], y[3]-x[3]);

    f = dot(x, v);
    g = (x[0]*v[0] + x[1]*v[1] + x[2]*v[2] + x[3]*v[3]);
    if (f != g) {
        printf("line: %i: dot: f: %5.4f, g: %5.4f\n", __LINE__, f, g);
    }

    f = dot(v, v);
    g = (v[0]*v[0] + v[1]*v[1] + v[2]*v[2] + v[3]*v[3]);
    if (f != g) {
        printf("line: %i: dot: f: %5.4f, g: %5.4f\n", __LINE__, f, g);
    }
    
    x = v; v -= y;
    SGVec4<T> t = x - y;
    f = dot(v, v);
    g = (t[0]*t[0] + t[1]*t[1] + t[2]*t[2] + t[3]*t[3]);
    if (f != g) {
        printf("line: %i: dot: f: %5.4f, g: %5.4f\n", __LINE__, f, g);
    }
  }
}
