
#include <stdio.h>
#include <cmath>
#include <cfloat>

#include "SGVec3.hxx"
#include "SGVec4.hxx"

#define TEST(a, p,q,r,s) \
 if (a[0] != p || a[1] != q || a[2] != r || a[3] != s) \
  printf("line: %i, diff: %5.4e, %5.4e, %5.4e, %5.4e\n", __LINE__, fabsf(a[0]-p), fabsf(a[1]-q), fabsf(a[2]-r), fabsf(a[3]-s));

int main()
{
    float p[4] = { 1.03f, 0.55f, 0.707f, -0.01f };
    const SGVec4f q( 0.31, 0.43, 0.69, 1.0 );
    SGVec4<float> v(p), x(p), y(p);
    float f, g;

    TEST(x, p[0], p[1], p[2], p[3]);
 
    x += q;
    TEST(x, p[0]+q[0], p[1]+q[1], p[2]+q[2], p[3]+q[3]);

    y -= q;
    TEST(y, p[0]-q[0], p[1]-q[1], p[2]-q[2], p[3]-q[3]);

    v = x; x *= 1.7f;
    TEST(x, v[0]*1.7f, v[1]*1.7f, v[2]*1.7f, v[3]*1.7f);

    v = y; y /= -.3f;
    TEST(y, v[0]/-.3f, v[1]/-.3f, v[2]/-.3f, v[3]/-.3f);

    v = +x;
    TEST(v, x[0], x[1], x[2], x[3]);

    v = -x;
    TEST(v, -x[0], -x[1], -x[2], -x[3]);

    v = y+x;
    TEST(v, y[0]+x[0], y[1]+x[1], y[2]+x[2], y[3]+x[3]);

    y = v*1.7f;
    TEST(y, v[0]*1.7f, v[1]*1.7f, v[2]*1.7f, v[3]*1.7f);

    y = 1.7f*v;
    TEST(y, v[0]*1.7f, v[1]*1.7f, v[2]*1.7f, v[3]*1.7f);

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
    SGVec4<float> t = x - y;
    f = dot(v, v);
    g = (t[0]*t[0] + t[1]*t[1] + t[2]*t[2] + t[3]*t[3]);
    if (f != g) {
        printf("line: %i: dot: f: %5.4f, g: %5.4f\n", __LINE__, f, g);
    }
}
