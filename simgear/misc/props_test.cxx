
////////////////////////////////////////////////////////////////////////
// Test harness.
////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include STL_IOSTREAM
#include "props.hxx"

#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(cout);
SG_USING_STD(endl);
#endif



////////////////////////////////////////////////////////////////////////
// Sample object.
////////////////////////////////////////////////////////////////////////

class Stuff {
public:
  Stuff () : _stuff(199.0) {}
  virtual float getStuff () const { return _stuff; }
  virtual void setStuff (float stuff) { _stuff = stuff; }
  virtual float getStuff (int index) const { return _stuff * index; }
  virtual void setStuff (int index, float val) {
    _stuff = val / index;
  }
private:
  float _stuff;
};



////////////////////////////////////////////////////////////////////////
// Sample function.
////////////////////////////////////////////////////////////////////////

static int get100 () { return 100; }

static double getNum (int index) { return 1.0 / index; }



////////////////////////////////////////////////////////////////////////
// Show a value.
////////////////////////////////////////////////////////////////////////

static void
show_values (const SGValue * value)
{
  cout << "Bool: " << value->getBoolValue() << endl;
  cout << "Int: " << value->getIntValue() << endl;
  cout << "Float: " << value->getFloatValue() << endl;
  cout << "Double: " << value->getDoubleValue() << endl;
  cout << "String: " << value->getStringValue() << endl;
}



////////////////////////////////////////////////////////////////////////
// Test individual values.
////////////////////////////////////////////////////////////////////////

static void
test_value ()
{
  SGValue * value;

  cout << endl << "Value" << endl << endl;

  //
  // Test coercion for getters.
  //

  cout << "Testing coercion from bool (expect true)" << endl;
  value = new SGValue;
  value->setBoolValue(true);
  show_values(value);
  delete value;
  cout << endl;

  cout << "Testing coercion from int (expect 128)" << endl;
  value = new SGValue;
  value->setIntValue(128);
  show_values(value);
  delete value;
  cout << endl;

  cout << "Testing coercion from float (expect 1.0/3.0)" << endl;
  value = new SGValue;
  value->setFloatValue(1.0/3.0);
  show_values(value);
  delete value;
  cout << endl;

  cout << "Testing coercion from double (expect 1.0/3.0)" << endl;
  value = new SGValue;
  value->setDoubleValue(1.0/3.0);
  show_values(value);
  delete value;
  cout << endl;

  cout << "Testing coercion from string (expect 10e4)" << endl;
  value = new SGValue;
  value->setStringValue("10e4");
  show_values(value);
  delete value;
  cout << endl;

  cout << "Testing coercion from unknown (expect -10e-4)" << endl;
  value = new SGValue;
  value->setUnknownValue("-10e-4");
  show_values(value);
  delete value;
  cout << endl;

  //
  // Test coercion for setters.
  //

  value = new SGValue;

  cout << "Testing coercion to bool from bool (expect false)" << endl;
  value->setBoolValue(false);
  show_values(value);
  cout << endl;

  cout << "Testing coercion to bool from int (expect 1)" << endl;
  value->setIntValue(1);
  show_values(value);
  cout << endl;

  cout << "Testing coercion to bool from float (expect 1.1)" << endl;
  value->setFloatValue(1.1);
  show_values(value);
  cout << endl;

  cout << "Testing coercion to bool from double (expect 1.1)" << endl;
  value->setDoubleValue(1.1);
  show_values(value);
  cout << endl;

  cout << "Testing coercion to bool from string (expect 1e10)" << endl;
  value->setStringValue("1e10");
  show_values(value);
  cout << endl;

  cout << "Testing coercion to bool from unknown (expect 1e10)" << endl;
  value->setUnknownValue("1e10");
  show_values(value);
  cout << endl;

  // Test tying to a pointer.

  static int myValue = 10;

  cout << "Testing tying to a pointer (expect 10)" << endl;
  if (!value->tie(SGRawValuePointer<int>(&myValue)))
    cout << "*** FAILED TO TIE VALUE!!!" << endl;
  show_values(value);
  cout << endl;

  cout << "Changing base variable (expect -5)" << endl;
  myValue = -5;
  show_values(value);
  if (!value->untie())
    cout << "*** FAIELD TO UNTIE VALUE!!!" << endl;
  cout << endl;


  // Test tying to static functions.

  cout << "Create a new int value (expect 10)" << endl;
  value->setIntValue(10);
  show_values(value);
  cout << endl;

  cout << "Testing tying to static getter (expect 100)" << endl;
  if (!value->tie(SGRawValueFunctions<int>(get100)))
    cout << "*** FAILED TO TIE VALUE!!!" << endl;
  show_values(value);
  cout << endl;

  cout << "Try changing value with no setter (expect 100)" << endl;
  if (value->setIntValue(200))
    cout << "*** setIntValue did not return false!!!" << endl;
  show_values(value);
  cout << endl;

  cout << "Untie value (expect 100)" << endl;
  if (!value->untie())
    cout << "*** FAILED TO UNTIE VALUE!!!" << endl;
  show_values(value);
  cout << endl;

  cout << "Try changing value (expect 200)" << endl;
  if (!value->setIntValue(200))
    cout << "*** setIntValue RETURNED FALSE!!!" << endl;
  show_values(value);
  cout << endl;

  // Test tying to indexed static functions.

  cout << "Create a new int value (expect 10)" << endl;
  value->setIntValue(10);
  show_values(value);
  cout << endl;

  cout << "Testing tying to indexed static getter (0.3333...)" << endl;
  if (!value->tie(SGRawValueFunctionsIndexed<double>(3, getNum)))
    cout << "*** FAILED TO TIE VALUE!!!" << endl;
  show_values(value);
  cout << endl;

  cout << "Untie value (expect 0.3333...)" << endl;
  if (!value->untie())
    cout << "*** FAILED TO UNTIE VALUE!!!" << endl;
  show_values(value);
  cout << endl;


  // Test methods.

  cout << "Try tying to an object without defaults (expect 199)" << endl;
  Stuff stuff;
  SGRawValueMethods<class Stuff,float> tiedstuff(stuff,
						 &Stuff::getStuff,
						 &Stuff::setStuff);
  if (!value->tie(tiedstuff, false))
    cout << "*** FAILED TO TIE VALUE!!!" << endl;
  show_values(value);
  cout << endl;

  cout << "Try untying from object (expect 199)" << endl;
  if (!value->untie())
    cout << "*** FAILED TO UNTIE VALUE!!!" << endl;
  show_values(value);
  cout << endl;

  cout << "Try tying to an indexed method (expect 199)" << endl;
  if (!value->tie(SGRawValueMethodsIndexed<class Stuff, float>
		  (stuff, 2, &Stuff::getStuff, &Stuff::setStuff)))
    cout << "*** FAILED TO TIE VALUE!!!" << endl;
  show_values(value);
  cout << endl;

  value->untie();

  cout << "Change value (expect 50)" << endl;
  if (!value->setIntValue(50))
    cout << "*** FAILED TO SET VALUE!!!" << endl;
  show_values(value);
  cout << endl;

  cout << "Try tying to an object with defaults (expect 50)" << endl;
  if (!value->tie(tiedstuff, true))
    cout << "*** FAILED TO TIE VALUE!!!" << endl;
  show_values(value);
  cout << endl;

  delete value;
}



////////////////////////////////////////////////////////////////////////
// Check property nodes.
////////////////////////////////////////////////////////////////////////

static void
dump_node (const SGPropertyNode * node)
{
  writeProperties(cout, node);
}

static void
test_property_nodes ()
{
  SGPropertyNode root;
  cout << "Created root node " << root.getPath() << endl;

  SGPropertyNode * child = root.getChild("foo", 0, true);

  SGPropertyNode *grandchild = child->getChild("bar", 0, true);
  grandchild->setDoubleValue(100);
  grandchild = child->getChild("bar", 1, true);
  grandchild->setDoubleValue(200);
  grandchild = child->getChild("bar", 2, true);
  grandchild->setDoubleValue(300);
  grandchild = child->getChild("bar", 3, true);
  grandchild->setDoubleValue(400);

  child = root.getChild("hack", 0, true);

  grandchild = child->getChild("bar", 0, true);
  grandchild->setDoubleValue(100);
  grandchild = child->getChild("bar", 3, true);
  grandchild->setDoubleValue(200);
  grandchild = child->getChild("bar", 1, true);
  grandchild->setDoubleValue(300);
  grandchild = child->getChild("bar", 2, true);
  grandchild->setDoubleValue(400);
  dump_node(&root);

  cout << "Trying path (expect /foo[0]/bar[0])" << endl;
  grandchild = root.getNode("/hack/../foo/./bar[0]");
  cout << "Path is " << grandchild->getPath() << endl;
  cout << endl;

  cout << "Looking for all /hack[0]/bar children" << endl;
  vector<SGPropertyNode *> bar = child->getChildren("bar");
  cout << "There are " << bar.size() << " matches" << endl;
  for (int i = 0; i < (int)bar.size(); i++)
    cout << bar[i]->getName() << '[' << bar[i]->getIndex() << ']' << endl;
  cout << endl;

  cout << "Testing addition of a totally empty node" << endl;
  if (root.getNode("/a/b/c", true) == 0)
    cerr << "** failed to create /a/b/c" << endl;
  dump_node(&root);
  cout << endl;
}


int main (int ac, char ** av)
{
  test_value();
  test_property_nodes();

  for (int i = 1; i < ac; i++) {
    cout << "Reading " << av[i] << endl;
    SGPropertyNode root;
    if (!readProperties(av[i], &root))
      cerr << "Failed to read properties from " << av[i] << endl;
    else
      writeProperties(cout, &root);
    cout << endl;
  }

  return 0;
}
