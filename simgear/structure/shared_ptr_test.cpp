/// Unit tests for reference counting and smart pointer classes
#define BOOST_TEST_MODULE structure
#include <BoostTestTargetConfig.h>

#include "SGSharedPtr.hxx"
#include "SGWeakPtr.hxx"

static int instance_count = 0;
struct ReferenceCounted:
  public SGReferenced
{
  ReferenceCounted()
  {
    ++instance_count;
  }

  ~ReferenceCounted()
  {
    --instance_count;
  }
};
typedef SGSharedPtr<ReferenceCounted> RefPtr;

BOOST_AUTO_TEST_CASE( shared_ptr )
{
  BOOST_REQUIRE_EQUAL( ReferenceCounted::count(0), 0 );

  RefPtr ptr( new ReferenceCounted() );
  BOOST_REQUIRE_EQUAL( instance_count, 1 );
  BOOST_REQUIRE_EQUAL( ReferenceCounted::count(ptr.get()), 1 );
  BOOST_REQUIRE_EQUAL( ptr.getNumRefs(), 1 );

  RefPtr ptr2 = ptr;
  BOOST_REQUIRE_EQUAL( ptr.getNumRefs(), 2 );
  BOOST_REQUIRE_EQUAL( ptr2.getNumRefs(), 2 );

  BOOST_REQUIRE_EQUAL( ptr, ptr2 );
  BOOST_REQUIRE_EQUAL( ptr.get(), ptr2.get() );

  ptr.reset();
  BOOST_REQUIRE( !ptr.get() );
  BOOST_REQUIRE_EQUAL( ptr.getNumRefs(), 0 );
  BOOST_REQUIRE_EQUAL( ReferenceCounted::count(ptr2.get()), 1 );
  BOOST_REQUIRE_EQUAL( ptr2.getNumRefs(), 1 );

  ptr2.reset();
  BOOST_REQUIRE( !ptr2.get() );
  BOOST_REQUIRE_EQUAL( ptr.getNumRefs(), 0 );
  BOOST_REQUIRE_EQUAL( ptr2.getNumRefs(), 0 );
  BOOST_REQUIRE_EQUAL( instance_count, 0) ;
}

class Base1:
  public virtual SGVirtualWeakReferenced
{};

class Base2:
  public virtual SGVirtualWeakReferenced
{};

class VirtualDerived:
  public Base1,
  public Base2
{};

BOOST_AUTO_TEST_CASE( virtual_weak_ptr )
{
  SGSharedPtr<VirtualDerived> ptr( new VirtualDerived() );
  SGWeakPtr<VirtualDerived> weak_ptr( ptr );
  BOOST_REQUIRE_EQUAL( ptr.getNumRefs(), 1 );

  SGSharedPtr<Base1> ptr1( weak_ptr.lock() );
  BOOST_REQUIRE_EQUAL( ptr.getNumRefs(), 2 );

  SGSharedPtr<Base2> ptr2( weak_ptr.lock() );
  BOOST_REQUIRE_EQUAL( ptr.getNumRefs(), 3 );

  BOOST_REQUIRE( ptr != NULL );
  BOOST_REQUIRE_EQUAL( ptr.get(), ptr1.get() );
  BOOST_REQUIRE_EQUAL( ptr.get(), ptr2.get() );

  SGWeakPtr<Base1> weak_base1( ptr );
  SGWeakPtr<Base2> weak_base2( ptr );
  ptr1 = dynamic_cast<VirtualDerived*>(weak_base1.lock().get());
  ptr2 = dynamic_cast<VirtualDerived*>(weak_base2.lock().get());

  BOOST_REQUIRE_EQUAL( ptr.get(), ptr1.get() );
  BOOST_REQUIRE_EQUAL( ptr.get(), ptr2.get() );
  BOOST_REQUIRE_EQUAL( ptr.getNumRefs(), 3 );
}
