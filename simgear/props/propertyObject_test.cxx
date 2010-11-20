
#include <simgear/compiler.h>

#include <iostream>
#include <cassert>
#include <cstring>

#include "propertyObject.hxx"

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
  
  const float fff(12.0f);
  assert(fff == aFoo); // comparion with float value

  return true;
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

  testReadMissing();
  testString();
  testAssignment();
  testSTLContainer();

  return EXIT_SUCCESS;
}

