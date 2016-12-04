#include <simgear/compiler.h>

#include "CSSBorder.hxx"

#include <cmath>
#include <iostream>
#include <simgear/misc/test_macros.hxx>

#define COMPARE(a, b) SG_CHECK_EQUAL_EP2((a), (b), 1e-4)


using namespace simgear;

int main (int ac, char ** av)
{
  CSSBorder b = CSSBorder::parse("5");
  SG_VERIFY(b.isValid());
  SG_VERIFY(!b.isNone());
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

  b = CSSBorder::parse("5 10 15 20");
  o = b.getRelOffsets(SGRect<int>(0,0,100,200));
  COMPARE(o.t, 0.025);
  COMPARE(o.r, 0.1);
  COMPARE(o.b, 0.075);
  COMPARE(o.l, 0.2);

  b = CSSBorder::parse("5% 10% 15% 20%");
  o = b.getAbsOffsets(SGRect<int>(0,0,100,200));
  COMPARE(o.t, 10);
  COMPARE(o.r, 10);
  COMPARE(o.b, 30);
  COMPARE(o.l, 20);

  o = b.getRelOffsets(SGRect<int>(0,0,100,200));
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
  SG_VERIFY(b.getKeyword().empty());
  SG_VERIFY(b.isNone());

  b = CSSBorder::parse("none");
  o = b.getRelOffsets(SGRect<int>(0,0,200,200));
  COMPARE(o.t, 0);
  COMPARE(o.r, 0);
  COMPARE(o.b, 0);
  COMPARE(o.l, 0);
  SG_VERIFY(b.getKeyword().empty());
  SG_VERIFY(b.isNone());

  CSSBorder b2;
  SG_VERIFY(!b2.isValid());
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
