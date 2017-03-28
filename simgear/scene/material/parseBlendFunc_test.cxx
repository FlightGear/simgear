#include <simgear_config.h>
#include <simgear/compiler.h>
#include <simgear/misc/test_macros.hxx>

#include "parseBlendFunc.hxx"
#include <simgear/props/props.hxx>
#include <osg/BlendFunc>


int main (int ac, char ** av)
{
  osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;

  // default blendfunc
  SG_VERIFY( simgear::parseBlendFunc(ss) );

  osg::BlendFunc* bf = dynamic_cast<osg::BlendFunc*>(
    ss->getAttribute(osg::StateAttribute::BLENDFUNC)
  );

  SG_VERIFY( bf );
  SG_CHECK_EQUAL(bf->getSource(),      osg::BlendFunc::SRC_ALPHA);
  SG_CHECK_EQUAL(bf->getDestination(), osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
  SG_CHECK_EQUAL(bf->getSource(),      bf->getSourceAlpha());
  SG_CHECK_EQUAL(bf->getDestination(), bf->getDestinationAlpha());

  // now set some values
  SGPropertyNode_ptr src = new SGPropertyNode,
                     dest = new SGPropertyNode;

  src->setStringValue("src-alpha");
  dest->setStringValue("constant-color");

  SG_VERIFY( simgear::parseBlendFunc(ss, src, dest) );

  bf = dynamic_cast<osg::BlendFunc*>(
      ss->getAttribute(osg::StateAttribute::BLENDFUNC)
  );

  SG_VERIFY( bf );
  SG_CHECK_EQUAL(bf->getSource(),      osg::BlendFunc::SRC_ALPHA);
  SG_CHECK_EQUAL(bf->getDestination(), osg::BlendFunc::CONSTANT_COLOR);
  SG_CHECK_EQUAL(bf->getSource(),      bf->getSourceAlpha());
  SG_CHECK_EQUAL(bf->getDestination(), bf->getDestinationAlpha());

  std::cout << "all tests passed successfully!" << std::endl;
  return 0;
}
