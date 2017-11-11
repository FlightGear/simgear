// -*- coding: utf-8 -*-
//
// Unit tests for reference counting and smart pointer classes

#include <iostream>
#include <cstdlib>              // EXIT_SUCCESS

#include <simgear/misc/test_macros.hxx>

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

void test_SGSharedPtr()
{
  std::cout << "Testing SGSharedPtr and SGReferenced" << std::endl;

  SG_CHECK_EQUAL( ReferenceCounted::count(0), 0 );

  RefPtr ptr( new ReferenceCounted() );
  SG_CHECK_EQUAL( instance_count, 1 );
  SG_CHECK_EQUAL( ReferenceCounted::count(ptr.get()), 1 );
  SG_CHECK_EQUAL( ptr.getNumRefs(), 1 );

  RefPtr ptr2 = ptr;
  SG_CHECK_EQUAL( ptr.getNumRefs(), 2 );
  SG_CHECK_EQUAL( ptr2.getNumRefs(), 2 );

  SG_CHECK_EQUAL( ptr, ptr2 );
  SG_CHECK_EQUAL( ptr.get(), ptr2.get() );

  ptr.reset();
  SG_CHECK_IS_NULL( ptr.get() );
  SG_CHECK_EQUAL( ptr.getNumRefs(), 0 );
  SG_CHECK_EQUAL( ReferenceCounted::count(ptr2.get()), 1 );
  SG_CHECK_EQUAL( ptr2.getNumRefs(), 1 );

  ptr2.reset();
  SG_CHECK_IS_NULL( ptr2.get() );
  SG_CHECK_EQUAL( ptr.getNumRefs(), 0 );
  SG_CHECK_EQUAL( ptr2.getNumRefs(), 0 );
  SG_CHECK_EQUAL( instance_count, 0) ;
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

void test_SGWeakPtr()
{
  std::cout << "Testing SGWeakPtr and SGVirtualWeakReferenced" << std::endl;

  SGSharedPtr<VirtualDerived> ptr( new VirtualDerived() );
  SGWeakPtr<VirtualDerived> weak_ptr( ptr );
  SG_CHECK_EQUAL( ptr.getNumRefs(), 1 );

  SGSharedPtr<Base1> ptr1( weak_ptr.lock() );
  SG_CHECK_EQUAL( ptr.getNumRefs(), 2 );

  // converting constructor
  SG_CHECK_EQUAL( SGSharedPtr<Base1>(weak_ptr), ptr1 );

  SGSharedPtr<Base2> ptr2( weak_ptr.lock() );
  SG_CHECK_EQUAL( ptr.getNumRefs(), 3 );

  SG_CHECK_IS_NOT_NULL( ptr );
  SG_CHECK_EQUAL( ptr.get(), ptr1.get() );
  SG_CHECK_EQUAL( ptr.get(), ptr2.get() );

  SGWeakPtr<Base1> weak_base1( ptr );
  SGWeakPtr<Base2> weak_base2( ptr );
  ptr1 = dynamic_cast<VirtualDerived*>(weak_base1.lock().get());
  ptr2 = dynamic_cast<VirtualDerived*>(weak_base2.lock().get());

  SG_CHECK_EQUAL( ptr.get(), ptr1.get() );
  SG_CHECK_EQUAL( ptr.get(), ptr2.get() );
  SG_CHECK_EQUAL( ptr.getNumRefs(), 3 );
}

int main(int argc, char* argv[])
{
  test_SGSharedPtr();
  test_SGWeakPtr();

  return EXIT_SUCCESS;
}
