#define BOOST_TEST_MODULE nasal
#include <BoostTestTargetConfig.h>

#include "TestContext.hxx"

#include <simgear/nasal/cppbind/NasalObjectHolder.hxx>

#include <iostream>
#include <set>

static std::set<intptr_t> active_instances;

static void ghost_destroy(void* p)
{
  active_instances.erase((intptr_t)p);
}

static naGhostType ghost_type = {
  &ghost_destroy,
  "TestGhost",
  0, // get_member
  0  // set_member
};

static naRef createTestGhost(TestContext& c, intptr_t p)
{
  active_instances.insert(p);
  return naNewGhost(c, &ghost_type, (void*)p);
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( ghost_gc )
{
  TestContext c;
  BOOST_REQUIRE(active_instances.empty());

  //-----------------------------------------------
  // Just create ghosts and let the gc destroy them

  naRef g1 = createTestGhost(c, 1),
        g2 = createTestGhost(c, 2);

  BOOST_CHECK_EQUAL(active_instances.count(1), 1);
  BOOST_CHECK_EQUAL(active_instances.count(2), 1);
  BOOST_CHECK_EQUAL(active_instances.size(), 2);

  c.runGC();

  BOOST_REQUIRE(active_instances.empty());


  //-----------------------------------------------
  // Create some more ghosts and save one instance
  // from being garbage collected.

  g1 = createTestGhost(c, 1);
  g2 = createTestGhost(c, 2);

  int gc1 = naGCSave(g1);
  c.runGC();

  BOOST_CHECK_EQUAL(active_instances.count(1), 1);
  BOOST_CHECK_EQUAL(active_instances.size(), 1);

  naGCRelease(gc1);
  c.runGC();

  BOOST_REQUIRE(active_instances.empty());


  //-----------------------------------------------
  // Now test attaching one ghost to another

  g1 = createTestGhost(c, 1);
  g2 = createTestGhost(c, 2);

  gc1 = naGCSave(g1);
  naGhost_setData(g1, g2); // bind lifetime of g2 to g1...

  c.runGC();

  BOOST_CHECK_EQUAL(active_instances.count(1), 1);
  BOOST_CHECK_EQUAL(active_instances.count(2), 1);
  BOOST_CHECK_EQUAL(active_instances.size(), 2);

  naGhost_setData(g1, naNil()); // cut connection
  c.runGC();

  BOOST_CHECK_EQUAL(active_instances.count(1), 1);
  BOOST_CHECK_EQUAL(active_instances.size(), 1);

  naGCRelease(gc1);
  c.runGC();

  BOOST_REQUIRE(active_instances.empty());
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( object_holder_gc )
{
  TestContext c;
  BOOST_REQUIRE_EQUAL(naNumSaved(), 0);
  BOOST_REQUIRE(active_instances.empty());

  //-----------------------------------------------
  // Put some ghosts in ObjectHolder and check if
  // they are saved from gc

  naRef g1 = createTestGhost(c, 1),
        g2 = createTestGhost(c, 2);

  nasal::ObjectHolder<> h1(g1);
  BOOST_CHECK_EQUAL(naNumSaved(), 1);
  BOOST_CHECK(naIsGhost(h1.get_naRef()));

  nasal::ObjectHolder<> h2(g2);
  BOOST_CHECK_EQUAL(naNumSaved(), 2);
  BOOST_CHECK(naIsGhost(h2.get_naRef()));

  c.runGC();

  BOOST_CHECK_EQUAL(active_instances.size(), 2);
  BOOST_CHECK_EQUAL(naNumSaved(), 2);

  h1.reset(naNum(1));
  h2.reset(naNum(2));
  BOOST_CHECK_EQUAL(naNumSaved(), 2);

  //-----------------------------------------------
  // Check that the saved objects are released

  h1.reset();
  BOOST_CHECK_EQUAL(naNumSaved(), 1);

  h2.reset();
  BOOST_CHECK_EQUAL(naNumSaved(), 0);

  c.runGC();
  BOOST_CHECK_EQUAL(active_instances.size(), 0);
}
