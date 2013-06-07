#include <simgear/compiler.h>

#include "CSSBorder.hxx"

#include <cmath>
#include <iostream>

#define COMPARE(a, b) \
  if( std::fabs((a) - (b)) > 1e-4 ) \
  { \
    std::cerr << "line " << __LINE__ << ": failed: "\
              << #a << " != " << #b << " d = " << ((a) - (b)) << std::endl; \
    return 1; \
  }

#define VERIFY(a) \
  if( !(a) ) \
  { \
    std::cerr << "line " << __LINE__ << ": failed: "\
              << #a << std::endl; \
    return 1; \
  }

using namespace simgear;

int main (int ac, char ** av)
{
  CSSBorder b = CSSBorder::parse("5");
  VERIFY(b.isValid());
  VERIFY(!b.isNone());
  CSSBorder::Offsets o = b.getAbsOffsets(SGRect<int>());
  COMPARE(o.t, 5);
  COMPARE(o.r, 5);
  COMPARE(o.b, 5);
  COMPARE(o.l, 5);

  b = CSSBorder::parse("5 10");
  o = b.getAbsOffsets(SGRect<int>());
  COMPARE(o.t, 5);
  COMPARE(o.r, 10);
  COMPARE(o.b, 5);
  COMPARE(o.l, 10);

  b = CSSBorder::parse("5 10 15");
  o = b.getAbsOffsets(SGRect<int>());
  COMPARE(o.t, 5);
  COMPARE(o.r, 10);
  COMPARE(o.b, 15);
  COMPARE(o.l, 10);

  b = CSSBorder::parse("5 10 15 20");
  o = b.getAbsOffsets(SGRect<int>());
  COMPARE(o.t, 5);
  COMPARE(o.r, 10);
  COMPARE(o.b, 15);
  COMPARE(o.l, 20);

  b = CSSBorder::parse("5% 10% 15% 20%");
  o = b.getAbsOffsets(SGRect<int>(0,0,200,200));
  COMPARE(o.t, 10);
  COMPARE(o.r, 20);
  COMPARE(o.b, 30);
  COMPARE(o.l, 40);

  o = b.getRelOffsets(SGRect<int>(0,0,200,200));
  COMPARE(o.t, 0.05);
  COMPARE(o.r, 0.1);
  COMPARE(o.b, 0.15);
  COMPARE(o.l, 0.2);

  b = CSSBorder::parse("5% none");
  o = b.getAbsOffsets(SGRect<int>(0,0,200,200));
  COMPARE(o.t, 0);
  COMPARE(o.r, 0);
  COMPARE(o.b, 0);
  COMPARE(o.l, 0);
  VERIFY(b.getKeyword().empty());
  VERIFY(b.isNone());

  b = CSSBorder::parse("none");
  o = b.getRelOffsets(SGRect<int>(0,0,200,200));
  COMPARE(o.t, 0);
  COMPARE(o.r, 0);
  COMPARE(o.b, 0);
  COMPARE(o.l, 0);
  VERIFY(b.getKeyword().empty());
  VERIFY(b.isNone());

  CSSBorder b2;
  VERIFY(!b2.isValid());
  o = b.getAbsOffsets(SGRect<int>(0,0,200,200));
  COMPARE(o.t, 0);
  COMPARE(o.r, 0);
  COMPARE(o.b, 0);
  COMPARE(o.l, 0);
  o = b.getRelOffsets(SGRect<int>(0,0,200,200));
  COMPARE(o.t, 0);
  COMPARE(o.r, 0);
  COMPARE(o.b, 0);
  COMPARE(o.l, 0);

  std::cout << "all tests passed successfully!" << std::endl;
  return 0;
}
