#include <simgear/compiler.h>

#include "parse_color.hxx"

#include <iostream>

#define COMPARE(a, b) \
  if( (a) != (b) ) \
  { \
    std::cerr << "failed:" << #a << " != " << #b << std::endl; \
    return 1; \
  }

#define VERIFY(a) \
  if( !(a) ) \
  { \
    std::cerr << "failed:" << #a << std::endl; \
    return 1; \
  }

#define VERIFY_COLOR(str, r, g, b, a) \
  VERIFY(simgear::parseColor(str, color)) \
  COMPARE(color, osg::Vec4(r, g, b, a))
    
int main (int ac, char ** av)
{
  osg::Vec4 color;
  VERIFY_COLOR("#ff0000", 1,0,0,1);
  VERIFY_COLOR("#00ff00", 0,1,0,1);
  VERIFY_COLOR("#0000ff", 0,0,1,1);
  VERIFY_COLOR("rgb( 255,\t127.5,0)", 1, 0.5, 0, 1);
  VERIFY_COLOR("rgba(255,  127.5,0, 0.5)", 1, 0.5, 0, 0.5);
  std::cout << "all tests passed successfully!" << std::endl;
  return 0;
}
