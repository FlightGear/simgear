#include <simgear/compiler.h>

#include "parseBlendFunc.hxx"
#include <simgear/props/props.hxx>
#include <osg/BlendFunc>

#include <iostream>

#define COMPARE(a, b) \
  if( (a) != (b) ) \
  { \
    std::cerr << "line " << __LINE__ << ": failed: "\
              << #a << " != " << #b << std::endl; \
    return 1; \
  }

#define VERIFY(a) \
  if( !(a) ) \
  { \
    std::cerr << "line " << __LINE__ << ": failed: "\
              << #a << std::endl; \
    return 1; \
  }
    
int main (int ac, char ** av)
{
  osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;

  // default blendfunc
  VERIFY( simgear::parseBlendFunc(ss) );

  osg::BlendFunc* bf = dynamic_cast<osg::BlendFunc*>(
    ss->getAttribute(osg::StateAttribute::BLENDFUNC)
  );

  VERIFY( bf );
  COMPARE(bf->getSource(),      osg::BlendFunc::SRC_ALPHA);
  COMPARE(bf->getDestination(), osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
  COMPARE(bf->getSource(),      bf->getSourceAlpha());
  COMPARE(bf->getDestination(), bf->getDestinationAlpha());

  // now set some values
  SGPropertyNode_ptr src = new SGPropertyNode,
                     dest = new SGPropertyNode;

  src->setStringValue("src-alpha");
  dest->setStringValue("constant-color");

  VERIFY( simgear::parseBlendFunc(ss, src, dest) );

  bf = dynamic_cast<osg::BlendFunc*>(
      ss->getAttribute(osg::StateAttribute::BLENDFUNC)
  );

  VERIFY( bf );
  COMPARE(bf->getSource(),      osg::BlendFunc::SRC_ALPHA);
  COMPARE(bf->getDestination(), osg::BlendFunc::CONSTANT_COLOR);
  COMPARE(bf->getSource(),      bf->getSourceAlpha());
  COMPARE(bf->getDestination(), bf->getDestinationAlpha());

  std::cout << "all tests passed successfully!" << std::endl;
  return 0;
}
