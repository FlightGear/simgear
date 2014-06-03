#define BOOST_TEST_MODULE cppbind
#include <BoostTestTargetConfig.h>

#include "Ghost.hxx"

class Base1:
  public virtual SGVirtualWeakReferenced
{};

class Base2:
  public virtual SGVirtualWeakReferenced
{};

class Derived:
  public Base1,
  public Base2
{};

typedef SGSharedPtr<Base1> Base1Ptr;
typedef SGSharedPtr<Base2> Base2Ptr;
typedef SGSharedPtr<Derived> DerivedPtr;
typedef SGWeakPtr<Derived> DerivedWeakPtr;

typedef SGSharedPtr<SGReferenced> SGReferencedPtr;

// Check if shared_ptr_traits give correct types for strong and weak shared
// pointer
#define CHECK_PTR_TRAIT_TYPE(ptr_t, type_name, type)\
  BOOST_STATIC_ASSERT((boost::is_same<\
    nasal::shared_ptr_traits<ptr_t>::type_name,\
    type\
  >::value));

#define CHECK_PTR_TRAIT(ref, weak)\
  CHECK_PTR_TRAIT_TYPE(ref, strong_ref, ref)\
  CHECK_PTR_TRAIT_TYPE(weak, weak_ref, weak)\

CHECK_PTR_TRAIT(DerivedPtr, DerivedWeakPtr)

#undef CHECK_PTR_TRAIT
#undef CHECK_PTR_TRAIT_TYPE

BOOST_STATIC_ASSERT(( nasal::supports_weak_ref<Base1Ptr>::value));
BOOST_STATIC_ASSERT(( nasal::supports_weak_ref<Base2Ptr>::value));
BOOST_STATIC_ASSERT(( nasal::supports_weak_ref<DerivedPtr>::value));
BOOST_STATIC_ASSERT(( nasal::supports_weak_ref<DerivedWeakPtr>::value));
BOOST_STATIC_ASSERT((!nasal::supports_weak_ref<SGReferencedPtr>::value));

BOOST_AUTO_TEST_CASE( ghost_weak_strong_nasal_conversion )
{
  nasal::Ghost<Base1Ptr>::init("Base1");
  nasal::Ghost<Base2Ptr>::init("Base2");
  nasal::Ghost<DerivedPtr>::init("Derived")
    .bases<Base1Ptr>()
    .bases<Base2Ptr>();

  naContext c = naNewContext();

  DerivedPtr d = new Derived();
  DerivedWeakPtr weak(d);

  // store weak pointer and extract strong pointer
  naRef na_d = nasal::to_nasal(c, DerivedWeakPtr(d));
  BOOST_REQUIRE( naIsGhost(na_d) );
  BOOST_CHECK_EQUAL( nasal::from_nasal<Base1Ptr>(c, na_d), d );
  BOOST_CHECK_EQUAL( nasal::from_nasal<Base2Ptr>(c, na_d), d );
  BOOST_CHECK_EQUAL( nasal::from_nasal<DerivedPtr>(c, na_d), d );

  d.reset();
  BOOST_REQUIRE( !nasal::from_nasal<DerivedPtr>(c, na_d) );

  // store strong pointer and extract weak pointer
  d.reset(new Derived);
  na_d = nasal::to_nasal(c, d);
  BOOST_REQUIRE( naIsGhost(na_d) );

  weak = nasal::from_nasal<DerivedWeakPtr>(c, na_d);
  BOOST_CHECK_EQUAL( weak.lock(), d );

  d.reset();
  BOOST_REQUIRE( nasal::from_nasal<DerivedPtr>(c, na_d) );
  BOOST_REQUIRE( weak.lock() );

  naFreeContext(c);
  naGC();

  BOOST_REQUIRE( !weak.lock() );
}
