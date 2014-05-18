/// Unit tests for reference counting and smart pointer classes
#define BOOST_TEST_MODULE structure
#include <BoostTestTargetConfig.h>

#include "MouseEvent.hxx"
#include "CustomEvent.hxx"

namespace sc = simgear::canvas;

BOOST_AUTO_TEST_CASE( canvas_event_types )
{
  // Register type
  BOOST_REQUIRE_EQUAL( sc::Event::strToType("test"),
                       sc::Event::UNKNOWN );
  BOOST_REQUIRE_EQUAL( sc::Event::getOrRegisterType("test"),
                       sc::Event::CUSTOM_EVENT );
  BOOST_REQUIRE_EQUAL( sc::Event::strToType("test"),
                       sc::Event::CUSTOM_EVENT );
  BOOST_REQUIRE_EQUAL( sc::Event::typeToStr(sc::Event::CUSTOM_EVENT),
                       "test" );

  // Basic internal type
  BOOST_REQUIRE_EQUAL( sc::Event::typeToStr(sc::Event::MOUSE_DOWN),
                       "mousedown" );
  BOOST_REQUIRE_EQUAL( sc::Event::strToType("mousedown"),
                       sc::Event::MOUSE_DOWN );

  // Unknown type
  BOOST_REQUIRE_EQUAL( sc::Event::typeToStr(123),
                       "unknown" );

  // Register type through custom event instance
  sc::CustomEvent e("blub");
  BOOST_REQUIRE_EQUAL( e.getTypeString(), "blub" );
  BOOST_REQUIRE_NE( e.getType(), sc::Event::UNKNOWN );
}
