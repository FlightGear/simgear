
////////////////////////////////////////////////////////////////////////
// Test harness.
////////////////////////////////////////////////////////////////////////

#include <simgear/compiler.h>

#include STL_IOSTREAM
#include "props.hxx"

#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(cout);
SG_USING_STD(cerr);
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
show_values (const SGPropertyNode * node)
{
  cout << "Bool: " << node->getBoolValue() << endl;
  cout << "Int: " << node->getIntValue() << endl;
  cout << "Float: " << node->getFloatValue() << endl;
  cout << "Double: " << node->getDoubleValue() << endl;
  cout << "String: " << node->getStringValue() << endl;
}



////////////////////////////////////////////////////////////////////////
// Test individual values.
////////////////////////////////////////////////////////////////////////

static void
test_value ()
{
  SGPropertyNode * node;

  cout << endl << "Value" << endl << endl;

  //
  // Test coercion for getters.
  //

  cout << "Testing coercion from bool (expect true)" << endl;
  node = new SGPropertyNode;
  node->setBoolValue(true);
  show_values(node);
  delete node;
  cout << endl;

  cout << "Testing coercion from int (expect 128)" << endl;
  node = new SGPropertyNode;
  node->setIntValue(128);
  show_values(node);
  delete node;
  cout << endl;

  cout << "Testing coercion from float (expect 1.0/3.0)" << endl;
  node = new SGPropertyNode;
  node->setFloatValue(1.0/3.0);
  show_values(node);
  delete node;
  cout << endl;

  cout << "Testing coercion from double (expect 1.0/3.0)" << endl;
  node = new SGPropertyNode;
  node->setDoubleValue(1.0/3.0);
  show_values(node);
  delete node;
  cout << endl;

  cout << "Testing coercion from string (expect 10e4)" << endl;
  node = new SGPropertyNode;
  node->setStringValue("10e4");
  show_values(node);
  delete node;
  cout << endl;

  cout << "Testing coercion from unspecified (expect -10e-4)" << endl;
  node = new SGPropertyNode;
  node->setUnspecifiedValue("-10e-4");
  show_values(node);
  delete node;
  cout << endl;

  //
  // Test coercion for setters.
  //

  node = new SGPropertyNode;

  cout << "Testing coercion to bool from bool (expect false)" << endl;
  node->setBoolValue(false);
  show_values(node);
  cout << endl;

  cout << "Testing coercion to bool from int (expect 1)" << endl;
  node->setIntValue(1);
  show_values(node);
  cout << endl;

  cout << "Testing coercion to bool from float (expect 1.1)" << endl;
  node->setFloatValue(1.1);
  show_values(node);
  cout << endl;

  cout << "Testing coercion to bool from double (expect 1.1)" << endl;
  node->setDoubleValue(1.1);
  show_values(node);
  cout << endl;

  cout << "Testing coercion to bool from string (expect 1e10)" << endl;
  node->setStringValue("1e10");
  show_values(node);
  cout << endl;

  cout << "Testing coercion to bool from unspecified (expect 1e10)" << endl;
  node->setUnspecifiedValue("1e10");
  show_values(node);
  cout << endl;

  // Test tying to a pointer.

  static int myValue = 10;

  cout << "Testing tying to a pointer (expect 10)" << endl;
  if (!node->tie(SGRawValuePointer<int>(&myValue), false))
    cout << "*** FAILED TO TIE VALUE!!!" << endl;
  show_values(node);
  cout << endl;

  cout << "Changing base variable (expect -5)" << endl;
  myValue = -5;
  show_values(node);
  if (!node->untie())
    cout << "*** FAILED TO UNTIE VALUE!!!" << endl;
  cout << endl;


  // Test tying to static functions.

  cout << "Create a new int value (expect 10)" << endl;
  node->setIntValue(10);
  show_values(node);
  cout << endl;

  cout << "Testing tying to static getter (expect 100)" << endl;
  if (!node->tie(SGRawValueFunctions<int>(get100)))
    cout << "*** FAILED TO TIE VALUE!!!" << endl;
  show_values(node);
  cout << endl;

  cout << "Try changing value with no setter (expect 100)" << endl;
  if (node->setIntValue(200))
    cout << "*** setIntValue did not return false!!!" << endl;
  show_values(node);
  cout << endl;

  cout << "Untie value (expect 100)" << endl;
  if (!node->untie())
    cout << "*** FAILED TO UNTIE VALUE!!!" << endl;
  show_values(node);
  cout << endl;

  cout << "Try changing value (expect 200)" << endl;
  if (!node->setIntValue(200))
    cout << "*** setIntValue RETURNED FALSE!!!" << endl;
  show_values(node);
  cout << endl;

  // Test tying to indexed static functions.

  cout << "Create a new int value (expect 10)" << endl;
  node->setIntValue(10);
  show_values(node);
  cout << endl;

  cout << "Testing tying to indexed static getter (0.3333...)" << endl;
  if (!node->tie(SGRawValueFunctionsIndexed<double>(3, getNum)))
    cout << "*** FAILED TO TIE VALUE!!!" << endl;
  show_values(node);
  cout << endl;

  cout << "Untie value (expect 0.3333...)" << endl;
  if (!node->untie())
    cout << "*** FAILED TO UNTIE VALUE!!!" << endl;
  show_values(node);
  cout << endl;


  // Test methods.

  cout << "Try tying to an object without defaults (expect 199)" << endl;
  Stuff stuff;
  SGRawValueMethods<class Stuff,float> tiedstuff(stuff,
						 &Stuff::getStuff,
						 &Stuff::setStuff);
  if (!node->tie(tiedstuff, false))
    cout << "*** FAILED TO TIE VALUE!!!" << endl;
  show_values(node);
  cout << endl;

  cout << "Try untying from object (expect 199)" << endl;
  if (!node->untie())
    cout << "*** FAILED TO UNTIE VALUE!!!" << endl;
  show_values(node);
  cout << endl;

  cout << "Try tying to an indexed method (expect 199)" << endl;
  if (!node->tie(SGRawValueMethodsIndexed<class Stuff, float>
		  (stuff, 2, &Stuff::getStuff, &Stuff::setStuff)))
    cout << "*** FAILED TO TIE VALUE!!!" << endl;
  show_values(node);
  cout << endl;

  node->untie();

  cout << "Change value (expect 50)" << endl;
  if (!node->setIntValue(50))
    cout << "*** FAILED TO SET VALUE!!!" << endl;
  show_values(node);
  cout << endl;

  cout << "Try tying to an object with defaults (expect 50)" << endl;
  if (!node->tie(tiedstuff, true))
    cout << "*** FAILED TO TIE VALUE!!!" << endl;
  show_values(node);
  cout << endl;

  delete node;
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
