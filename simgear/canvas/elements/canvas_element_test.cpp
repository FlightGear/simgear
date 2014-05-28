/// Unit tests for canvas::Element
#define BOOST_TEST_MODULE canvas
#include <BoostTestTargetConfig.h>

#include "CanvasElement.hxx"
#include "CanvasGroup.hxx"

namespace sc = simgear::canvas;

BOOST_AUTO_TEST_CASE( attr_data )
{
  // http://www.w3.org/TR/html5/dom.html#attr-data-*

#define SG_CHECK_ATTR2PROP(attr, prop)\
  BOOST_CHECK_EQUAL(sc::Element::attrToDataPropName(attr), prop)

  // If name starts with "data-", for each "-" (U+002D) character in the name
  // that is followed by a lowercase ASCII letter, remove the "-" (U+002D)
  // character and replace the character that followed it by the same character
  // converted to ASCII uppercase.

  SG_CHECK_ATTR2PROP("no-data", "");
  SG_CHECK_ATTR2PROP("data-blub", "blub");
  SG_CHECK_ATTR2PROP("data-blub-x-y", "blubXY");
  SG_CHECK_ATTR2PROP("data-blub-x-y-", "blubXY-");

#undef SG_CHECK_ATTR2PROP

#define SG_CHECK_PROP2ATTR(prop, attr)\
  BOOST_CHECK_EQUAL(sc::Element::dataPropToAttrName(prop), attr)

  // If name contains a "-" (U+002D) character followed by a lowercase ASCII
  // letter, throw a SyntaxError exception (empty string) and abort these steps.
  // For each uppercase ASCII letter in name, insert a "-" (U+002D) character
  // before the character and replace the character with the same character
  // converted to ASCII lowercase.
  // Insert the string "data-" at the front of name.

  SG_CHECK_PROP2ATTR("test", "data-test");
  SG_CHECK_PROP2ATTR("testIt", "data-test-it");
  SG_CHECK_PROP2ATTR("testIt-Hyphen", "data-test-it--hyphen");
  SG_CHECK_PROP2ATTR("-test", "");
  SG_CHECK_PROP2ATTR("test-", "data-test-");

#undef SG_CHECK_PROP2ATTR

  SGPropertyNode_ptr node = new SGPropertyNode;
  sc::ElementPtr el =
    sc::Element::create<sc::Group>(sc::CanvasWeakPtr(), node);

  el->setDataProp("myData", 3);
  BOOST_CHECK_EQUAL( el->getDataProp<int>("myData"), 3 );
  BOOST_CHECK_EQUAL( node->getIntValue("data-my-data"), 3 );

  SGPropertyNode* prop = el->getDataProp<SGPropertyNode*>("notExistingProp");
  BOOST_CHECK( !prop );
  prop = el->getDataProp<SGPropertyNode*>("myData");
  BOOST_CHECK(  prop );
  BOOST_CHECK_EQUAL( prop->getParent(), node );
  BOOST_CHECK_EQUAL( prop->getIntValue(), 3 );

  BOOST_CHECK( el->hasDataProp("myData") );
  el->removeDataProp("myData");
  BOOST_CHECK( !el->hasDataProp("myData") );
  BOOST_CHECK_EQUAL( el->getDataProp("myData", 5), 5 );
}
