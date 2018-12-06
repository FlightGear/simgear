#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#ifdef NDEBUG
// Always enable DEBUG mode in test application, otherwise "assert" test
// statements have no effect and don't actually test anything (catch 17 ;-) ).
#undef NDEBUG
#endif

#include <simgear/compiler.h>

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "propertyObject.hxx"

#include <simgear/structure/exception.hxx>

using std::cout;
using std::cerr;
using std::endl;

using namespace simgear;


SGPropertyNode* testRoot = NULL;

bool testBasic()
{
  PropertyObject<int> aBar("a/bar");
  assert(aBar == 1234);


  PropertyObject<int> aWib("a/wib"); // doesn't exist
  aWib = 999; // create + set
  assert(aWib == 999); // read back
  assert(testRoot->getIntValue("a/wib") == 999);

  assert(aWib < 1000);
  assert(998 < aWib);

  PropertyObject<double> aFoo("a/foo");
  assert(aFoo == 12.0);

  double ff(aFoo);
  assert(ff == 12.0); // comparison with literal
  if (ff != 12.0) cout << "Error: a/foo != 12!" << endl;
  
  const float fff(12.0f);
  assert(fff == aFoo); // comparion with float value
  if (fff != aFoo) cout << "Error: 12 != a/foo" << endl;

  return true;
}

void testRelative()
{
  SGPropertyNode* n = testRoot->getNode("a");
  assert(n);

  PropertyObject<int> a1(n, "bar");
  assert(a1 == 1234);

  PropertyObject<int> a5(n, "some/child/path");
  a5 = 4321;
  assert(n->getIntValue("some/child/path") == 4321);

  SGPropertyNode* m = testRoot->getNode("a/alice");
  PropertyObject<std::string> a4(m);
  assert(a4 == "aaaa");
}

void testString()
{
  PropertyObject<std::string> sp("a/alice");
  assert(sp == "aaaa"); // read

  sp = "xxxx"; // assignment from char* literal
  assert(!strcmp(testRoot->getStringValue("a/alice"), "xxxx"));

  std::string d = "yyyy";
  sp = d; // assignment from std::string
  assert(sp == d);  // comaprisom with std::string

  std::string e(sp), f; // check construction-conversion
  assert(e == "yyyy");
  f = sp; // assignment conversion
  assert(f == "yyyy");
}

void testAssignment()
{
  PropertyObject<int> a1("a/bar");
  PropertyObject<int> a2("b/blah/foo[1]");

 // ensure assignment between property objects copies values, *not*
 // copies the property reference
  a2 = a1 = 88; // a2 should *not* point to a/bar after this!
  assert(testRoot->getIntValue("b/blah/foo[1]") == 88);
  assert(a2.node() == testRoot->getNode("b/blah/foo[1]"));
  a2 = 99;
  assert(a2 == 99);
  assert(a1 == 88);

  PropertyObject<int> a3(a1);
  assert(a1.node() == a3.node());
  a3 = 44;
  assert(a1 == 44);

  // Compound assignment ops
  a1 *= 2;
  assert(a1 == 88);
  a1 /= 2;
  assert(a1 == 44);
  a1 += 2;
  assert(a1 == 46);
  a1 -= 16;
  assert(a1 == 30);
  a1 %= 28;
  assert(a1 == 2);
  a1 >>= 1;
  assert(a1 == 1);
  a1 <<= 2;
  assert(a1 == 4);
  a1 &= 1;
  assert(a1 == 0);
  a1 ^= 2;
  assert(a1 == 2);
  a1 |= 1;
  assert(a1 == 3);
}

void testSTLContainer()
{
  std::vector<PropertyObject<int> > vec;
// enlarging the vec causes the copy-constructor to be called,
// when the storage is re-sized
  vec.push_back(PropertyObject<int>("a/thing[0]")); 
  vec.push_back(PropertyObject<int>("a/thing[1]")); 
  vec.push_back(PropertyObject<int>("a/thing[2]")); 
  vec.push_back(PropertyObject<int>("a/thing[3]")); 

  vec[0] = 1234;
  vec[1] = 2345;
  vec[2] = 6789;
  vec[3] = -11;

  assert(testRoot->getIntValue("a/thing[2]") == 6789);
  assert(testRoot->getIntValue("a/thing[3]") == -11);
  assert(testRoot->getIntValue("a/thing[0]") == 1234);

  for (int i=0; i<100; ++i) {
    char path[128];
    ::snprintf(path, 128, "d/foo[%d]", i);
    vec.push_back(PropertyObject<int>(path));
    testRoot->setIntValue(path, i * 4);
  }

  assert(vec[0] == 1234);
  assert(vec[3] == -11);
}

void testReadMissing()
{
  PropertyObject<bool> b("not/found/honest");

  try {
    bool v = b;
    assert(false && "read of missing property didn't throw");
    (void) v; // don't warn about unused variable
  } catch (sg_exception& e) {
    // expected
  }

  PropertyObject<std::string> s("also/missing");
  try {
    std::string s2 = s;
  } catch (sg_exception& e) {
    // expected
  }
}

void testCreate()
{
  PropertyObject<bool> a = PropertyObject<bool>::create("a/lemon", true);
  assert(a == true);
  assert(testRoot->getBoolValue("a/lemon") == true);
  

  PropertyObject<int> b(PropertyObject<int>::create("a/pear", 3142));
  assert(b == 3142);
  
  PropertyObject<std::string> c(PropertyObject<std::string>::create("a/lime", "fofofo"));
  assert(c == "fofofo");

// check overloads for string version
  SGPropertyNode* n = testRoot->getNode("b", true);
  PropertyObject<std::string> d(PropertyObject<std::string>::create(n, "grape", "xyz"));
  assert(!strcmp(testRoot->getStringValue("b/grape"), "xyz"));
  
  
}

void testDeclare()
{
    PropertyObject<bool>        a;
    PropertyObject<int>         b;
    PropertyObject<double>      c;
    PropertyObject<std::string> d;
}

void testDeclareThenDefine()
{
    // Declare.
    PropertyObject<bool>        a;
    PropertyObject<std::string> b;

    // Property nodes.
    SGPropertyNode_ptr propsA = new SGPropertyNode;
    SGPropertyNode_ptr propsB = new SGPropertyNode;
    propsA->setValue(true);
    propsB->setValue("test");

    // Now define.
    a = PropertyObject<bool>::create(propsA);
    b = PropertyObject<std::string>::create(propsB);

    assert(a == true);
    assert(b == "test");
}

int main(int argc, char* argv[])
{
	testRoot = new SGPropertyNode();
	simgear::PropertyObjectBase::setDefaultRoot(testRoot);

// create some properties 'manually'
	testRoot->setDoubleValue("a/foo", 12.0);
	testRoot->setIntValue("a/bar", 1234);
	testRoot->setBoolValue("a/flags[3]", true);
  testRoot->setStringValue("a/alice", "aaaa");

// basic reading / setting
	if (!testBasic()) {
		return EXIT_FAILURE;		
	}

  testRelative();
  testReadMissing();
  testString();
  testAssignment();
  testSTLContainer();
  testCreate();
  testDeclare();
  testDeclareThenDefine();

  return EXIT_SUCCESS;
}

