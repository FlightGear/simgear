
////////////////////////////////////////////////////////////////////////
// Test harness.
////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <algorithm>
#include <memory>               // std::unique_ptr
#include <iostream>
#include <map>

#include "props.hxx"
#include "props_io.hxx"

#include <simgear/misc/test_macros.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/test_macros.hxx>

using std::cout;
using std::cerr;
using std::endl;



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
  cout << "Bool: " << (node->getBoolValue() ? "true" : "false") << endl;
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
  writeProperties(cout, node, true);
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

    SG_CHECK_EQUAL(grandchild->getPosition(), 3);

  child = root.getChild("hack", 0, true);

    SG_CHECK_EQUAL(child->getPosition(), 1);


  grandchild = child->getChild("bar", 0, true);
  grandchild->setDoubleValue(100);
  grandchild = child->getChild("bar", 3, true);
  grandchild->setDoubleValue(200);

    SG_CHECK_EQUAL(grandchild->getPosition(), 1);


  grandchild = child->getChild("bar", 1, true);
  grandchild->setDoubleValue(300);
  grandchild = child->getChild("bar", 2, true);
  grandchild->setDoubleValue(400);

    SG_CHECK_EQUAL(grandchild->getPosition(), 3);


  dump_node(&root);

    SG_CHECK_EQUAL(child->getPosition(), 1);
    SG_CHECK_EQUAL(grandchild->getPosition(), 3);

  cout << "Trying path (expect /foo[0]/bar[0])" << endl;
  grandchild = root.getNode("/hack/../foo/./bar[0]");
  cout << "Path is " << grandchild->getPath() << endl;
  cout << endl;

  cout << "Looking for all /hack[0]/bar children" << endl;
  std::vector<SGPropertyNode_ptr> bar = child->getChildren("bar");
  cout << "There are " << bar.size() << " matches" << endl;
  for (int i = 0; i < (int)bar.size(); i++)
    cout << bar[i]->getName() << '[' << bar[i]->getIndex() << ']' << endl;
  cout << endl;

  cout << "Testing addition of a totally empty node" << endl;
  if (root.getNode("/a/b/c", true) == 0)
    cerr << "** FAILED to create /a/b/c" << endl;
  dump_node(&root);
  cout << endl;


}

void test_addChild()
{
  SGPropertyNode root;

  cout << "Testing the addChild function " << endl;
  cout << "Created root node " << root.getPath() << endl;

  SGPropertyNode *test = root.getChild("test", 0, true);
  SGPropertyNode *n = test->getNode("foo", true);

  cout << "Testing appending initial child node" << endl;
  test = root.addChild("child", 0, true);
  if (test == 0)
    cerr << "** FAILED to append initial child node" << endl;
  if (root.getNode("child", 0, false) != test)
      cerr << "** FAILED to create initial child node at expected index #0" << endl;

  n->getChild("child", 1, true)->setIntValue(1);
  n->getChild("child", 2, true)->setIntValue(2);
  n->getChild("child", 4, true)->setIntValue(4);
  dump_node(&root);

  SGPropertyNode *ch = n->addChild("child");
  ch->setIntValue(5);
  if (n->getChild("child", 5, false) != ch)
      cerr << "** FAILED to create child node at expected index #5" << endl;
  cerr << endl << "ADDED: " << ch->getPath() << endl << endl;

  cout << "Testing appending child node at first empty index (Skipping 0)" << endl;
  ch = n->addChild("child", 1, false);
  ch->setIntValue(3);
  if (n->getChild("child", 3, false) != ch)
      cerr << "** FAILED to create child node at expected index #3" << endl;

  cout << "Testing appending child node at first empty index" << endl;
  ch = n->addChild("child", 0, false);
  ch->setIntValue(0);
  if (n->getChild("child", 0, false) != ch)
      cerr << "** FAILED to create child node at expected index #0" << endl;

  cout << "Testing appending child node" << endl;
  ch = n->addChild("first", 0, false);
  if (ch == 0)
      cerr << "** Failed to add child node" << endl;
  else
      ch->setStringValue("append first");
  if (n->getChild("first", 0, false) != ch)
      cerr << "** FAILED to append child node with expected index #0" << endl;
  cerr << "ADDED: " << ch->getPath() << endl;
  if (root.getNode("test/foo/first", false) != ch)
      cerr << "** FAILED to append child node at expected path 'test/foo/first'" << endl;

  dump_node(&root);
}


bool ensureNListeners(SGPropertyNode* node, int n)
{
    if (node->nListeners() != n) {
        return false;
    }

    for (int c=0; c < node->nChildren(); ++c) {
        if (!ensureNListeners(node->getChild(c), n)) {
            return false;
        }
    }

    return true;
}

void defineSamplePropertyTree(SGPropertyNode_ptr root)
{
    root->setIntValue("position/body/a", 42);
    root->setIntValue("position/body/c", 1);
    root->setIntValue("position/body/d", 600);

    root->setDoubleValue("velocity/body/x", 1.23);
    root->setDoubleValue("velocity/body/y", 4.56);
    root->setDoubleValue("velocity/body/z", 7.89);

    root->setStringValue("settings/render/foo", "flightgear");
    root->setStringValue("version", "10.3.19");

    for (int i=0; i<4; ++i) {
        root->setDoubleValue("controls/engine[" + std::to_string(i) + "]/throttle", 0.1 * i);
        root->setBoolValue("controls/engine[" + std::to_string(i) + "]/starter", i % 2);
        root->setDoubleValue("engine[" + std::to_string(i) + "]/rpm", 1000 * i);
        root->setDoubleValue("engine[" + std::to_string(i) + "]/temp", 100 * i);
    }

    root->setDoubleValue("controls/doors/door[2]/position-norm", 0.5);
}

class TestListener : public SGPropertyChangeListener
{
public:
    TestListener(SGPropertyNode* root, bool recursive = false, bool self_unregister = false) :
        _root(root), _self_unregister(self_unregister) {}

    virtual void valueChanged(SGPropertyNode* node) override
    {
        std::cout << "saw value changed for:" << node->getPath() << std::endl;
        valueChangeCount[node]++;
        if (_self_unregister) {
            std::cout << "valueChanged(): calling removeChangeListener() to self-remove\n";
            node->removeChangeListener(this);
        }
    }

    int checkValueChangeCount(const std::string& path) const
    {
        auto it = valueChangeCount.find(_root->getNode(path));
        if (it == valueChangeCount.end()) {
            return 0;
        }

        return it->second;
    }

    struct ParentChange
    {
        SGPropertyNode_ptr parent, child;
    };

    virtual void childAdded(SGPropertyNode * parent, SGPropertyNode * child) override
    {
        adds.push_back({parent, child});
    }

    virtual void childRemoved(SGPropertyNode * parent, SGPropertyNode * child) override
    {
        removes.push_back({parent, child});
    }

    std::vector<SGPropertyNode*> addedTo(SGPropertyNode* pr)
    {
        std::vector<SGPropertyNode*> result;
        for (auto c : adds) {
            if (c.parent == pr) {
                result.push_back(c.child);
            }
        }
        return result;
    }

    std::vector<SGPropertyNode*> removedFrom(SGPropertyNode* pr)
    {
        std::vector<SGPropertyNode*> result;
        for (auto c : removes) {
            if (c.parent == pr) {
                result.push_back(c.child);
            }
        }
        return result;
    }

    bool checkAdded(const std::string& pr, SGPropertyNode* n)
    {
        auto adds = addedTo(_root->getNode(pr));
        return std::find(adds.begin(), adds.end(), n) != adds.end();
    }

    bool checkRemoved(const std::string& pr, SGPropertyNode* n)
    {
        auto removes = removedFrom(_root->getNode(pr));
        return std::find(removes.begin(), removes.end(), n) != removes.end();
    }

    bool checkRemoved(SGPropertyNode* pr, SGPropertyNode* n)
    {
        auto removes = removedFrom(pr);
        return std::find(removes.begin(), removes.end(), n) != removes.end();
    }

        void resetChangeCounts()
        {
            valueChangeCount.clear();
            adds.clear();
            removes.clear();
        }
private:
    SGPropertyNode* _root;
    bool            _self_unregister;
    std::map<SGPropertyNode*, unsigned int> valueChangeCount;

    std::vector<ParentChange> adds;
    std::vector<ParentChange> removes;

};

void testListener()
{
    SGPropertyNode_ptr tree(new SGPropertyNode);
    defineSamplePropertyTree(tree);

    // basic listen
    {
        TestListener l(tree.get());
        tree->getNode("position/body/c")->addChangeListener(&l);
        tree->getNode("velocity/body/z")->addChangeListener(&l);
        tree->getNode("controls/engine[2]/starter")->addChangeListener(&l);

        tree->setBoolValue("controls/engine[2]/starter", true);
        SG_CHECK_EQUAL(l.checkValueChangeCount("controls/engine[2]/starter"), 1);
        SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/c"), 0);

        tree->setIntValue("position/body/c", 123);
        SG_CHECK_EQUAL(l.checkValueChangeCount("controls/engine[2]/starter"), 1);
        SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/c"), 1);

        // verify that changing non-listened props doesn't affect anything
        tree->setIntValue("position/body/a", 19);
        tree->setBoolValue("controls/engine[1]/starter", true);

        SG_CHECK_EQUAL(l.checkValueChangeCount("controls/engine[2]/starter"), 1);
        SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/c"), 1);

    }

    // initial value set
    {
        TestListener l(tree.get());
        tree->getNode("velocity/body/y")->addChangeListener(&l, true);
        tree->getNode("controls/engine[2]/starter")->addChangeListener(&l, true);
        SG_CHECK_EQUAL(l.checkValueChangeCount("controls/engine[2]/starter"), 1);
        SG_CHECK_EQUAL(l.checkValueChangeCount("velocity/body/y"), 1);
    }

    // delete listener while listening, should be fine
    {
        std::unique_ptr<TestListener> l( new TestListener(tree.get()));
        tree->getNode("position/body/c")->addChangeListener(l.get());
        tree->getNode("velocity/body/z")->addChangeListener(l.get());
        tree->getNode("controls/engine[2]/starter")->addChangeListener(l.get());

        SG_CHECK_EQUAL(tree->getNode("position/body/c")->nListeners(), 1);
        SG_CHECK_EQUAL(
          tree->getNode("controls/engine[2]/starter")->nListeners(), 1);

        l.reset();

        SG_CHECK_EQUAL(tree->getNode("position/body/c")->nListeners(), 0);
        SG_CHECK_EQUAL(
          tree->getNode("controls/engine[2]/starter")->nListeners(), 0);

        tree->getNode("position/body/c")->setIntValue(49.0);
        tree->getNode("controls/engine[2]/starter")->setBoolValue(true);
    }


    // add/remove of child listen
    {
        TestListener l(tree.get());
        tree->getNode("controls")->addChangeListener(&l);
        tree->getNode("velocity/body")->addChangeListener(&l);

        tree->setDoubleValue("controls/pitch", 0.5);
        SG_VERIFY(l.checkAdded("controls", tree->getNode("controls/pitch")));

        tree->setIntValue("controls/yaw", 12);

        SG_VERIFY(l.checkAdded("controls", tree->getNode("controls/yaw")));

        // branch as well as leaf nodes should work the same
        tree->setBoolValue("controls/gears/gear[1]/locked", true);
        SG_VERIFY(l.checkAdded("controls", tree->getNode("controls/gears")));

        SGPropertyNode_ptr rm = tree->getNode("velocity/body/z");
        tree->getNode("velocity/body")->removeChild("z");
        SG_VERIFY(l.checkRemoved("velocity/body", rm.get()));

        SG_CHECK_EQUAL(
          l.checkRemoved("velocity/body", tree->getNode("velocity/body/x")),
          false);
    }

    tree = new SGPropertyNode;
    defineSamplePropertyTree(tree);

    // ensure basic listenter is not recursive
    // disabled for now, since all listeners are recurisve
#if 0
    {
        TestListener l(tree.get(), false /* not recursive */);
        tree->getNode("position/body")->addChangeListener(&l);
        tree->getNode("controls/")->addChangeListener(&l);

        SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/a"), 0);

        tree->setIntValue("position/body/a", 111);
        tree->setIntValue("position/body/z", 43);

        SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/a"), 0);
        SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/z"), 0);

        tree->getNode("position/body/new", true);
        SG_VERIFY(l.checkAdded("position/body",
                               tree->getNode("position/body/new")));
    }
#endif

    tree = new SGPropertyNode;
    defineSamplePropertyTree(tree);
    // recursive listen
    {
        TestListener l(tree.get(), true /* recursive */);
        tree->getNode("position/body")->addChangeListener(&l);
        tree->getNode("controls/")->addChangeListener(&l);

        SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/a"), 0);

        tree->setIntValue("position/body/a", 111);
        tree->setIntValue("position/body/z", 43);

        SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/a"), 1);
        SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/z"), 1);

        SG_VERIFY(l.checkAdded("position/body",
                               tree->getNode("position/body/z")));


        tree->setBoolValue("controls/engine[3]/starter", true);
        SG_CHECK_EQUAL(l.checkValueChangeCount("controls/engine[3]/starter"), 1);

        tree->setBoolValue("controls/engines[1]/fuel-cutoff", true);
        SG_VERIFY(
          l.checkAdded("controls/engines[1]",
                       tree->getNode("controls/engines[1]/fuel-cutoff")));

        SG_CHECK_EQUAL(
          l.checkValueChangeCount("controls/engines[1]/fuel-cutoff"), 1);

        tree->setDoubleValue("controls/doors/door[2]/position-norm", 0.5);
        SG_CHECK_EQUAL(
          l.checkValueChangeCount("controls/doors/door[2]/position-norm"), 1);

        SGPropertyNode_ptr door2Node = tree->getNode("controls/doors/door[2]");
        SGPropertyNode_ptr door2PosNode = tree->getNode("controls/doors/door[2]/position-norm");

        tree->getNode("controls/doors")->removeChild(door2Node);

        SG_VERIFY(l.checkRemoved("controls/doors", door2Node));

        // default is not recurse for children
        SG_CHECK_EQUAL(l.checkRemoved(door2Node, door2PosNode), false);

        // adds *are* seen recursively
        tree->setStringValue("controls/lights/light[3]/foo/state", "dim");
        SG_VERIFY(
          l.checkAdded("controls/lights/light[3]/foo",
                       tree->getNode("controls/lights/light[3]/foo/state")));


        // remove a listener
        tree->getNode("controls")->removeChangeListener(&l);

        // removing listener does not trigger remove notifications
        SG_VERIFY(!l.checkRemoved("controls",
                                  tree->getNode("controls/engine[3]")));
        SG_VERIFY(!l.checkRemoved("controls/engine[3]",
                                  tree->getNode("controls/engine[3]/starter")));

        tree->setBoolValue("controls/engines[1]/fuel-cutoff", false);
        tree->setBoolValue("controls/engines[9]/fuel-cutoff", true);

        // values should be unchanged
        SG_CHECK_EQUAL(
          l.checkValueChangeCount("controls/engines[1]/fuel-cutoff"), 1);
        SG_CHECK_EQUAL(
          l.checkValueChangeCount("controls/engines[9]/fuel-cutoff"), 0);

        // ensure additional calls to fireChildrenRecursive don't cause multiple adds

        // disabled for now since we don't add recursive listeners to sub-trees
      //  SG_VERIFY(ensureNListeners(tree->getNode("position/body"), 1));
        SG_VERIFY(ensureNListeners(tree->getNode("controls"), 0));

        tree->getNode("position/body")->fireCreatedRecursive();
        // disabled for now since we don't add recursive listeners to sub-trees
        //SG_VERIFY(ensureNListeners(tree->getNode("position/body"), 1));
    }

}

void testAliasedListeners()
{
    SGPropertyNode_ptr tree(new SGPropertyNode);
    defineSamplePropertyTree(tree);
    TestListener l(tree.get());

    tree->getNode("position/world/x", true)->alias(tree->getNode("position/body/a"));
    tree->getNode("position/earth", true)->alias(tree->getNode("position/body"));

    tree->getNode("position/world/x")->addChangeListener(&l);
    tree->getNode("position/earth")->addChangeListener(&l);

    tree->setIntValue("position/body/a", 99);
    SG_CHECK_EQUAL(tree->getIntValue("position/world/x"), 99);
    SG_CHECK_EQUAL(l.checkValueChangeCount("position/world/x"), 1);

    tree->setIntValue("position/world/x", 101);
    SG_CHECK_EQUAL(tree->getIntValue("position/body/a"), 101);
    SG_CHECK_EQUAL(l.checkValueChangeCount("position/world/x"), 2);
}

class TiedPropertyDonor
{

public:

    int someMember = 100;

    const char* getWrappedA() const { return wrappedMember.c_str(); }
    void setWrappedA(const char* s) { wrappedMember = std::string(s); }

    std::string wrappedMember;
    double wrappedB = 1.23;

    double getWrappedB() const { return wrappedB; }
};

void tiedPropertiesTest()
{
    TiedPropertyDonor donor;

    SGPropertyNode_ptr tree(new SGPropertyNode);
    defineSamplePropertyTree(tree);

    tree->setDoubleValue("position/body/mass", 145.0);

    tree->tie("position/body/a", SGRawValuePointer<int>(&donor.someMember));
    tree->tie("settings/render/foo", SGRawValueMethods<TiedPropertyDonor, const char*>(donor, &TiedPropertyDonor::getWrappedA, &TiedPropertyDonor::setWrappedA));
    tree->tie("position/body/mass", SGRawValueMethods<TiedPropertyDonor, double>(donor, &TiedPropertyDonor::getWrappedB, nullptr));

    // tie sets current values of the property onto the setter
    SG_CHECK_EQUAL(tree->getStringValue("settings/render/foo"),
                   std::string("flightgear"));
    SG_CHECK_EQUAL(tree->getIntValue("position/body/a"), 42);

    // but can't write to this one!
    SG_CHECK_EQUAL(tree->getDoubleValue("position/body/mass"), 1.23);

    donor.setWrappedA("hello world");
    donor.someMember = 13;
    SG_CHECK_EQUAL(tree->getIntValue("position/body/a"), 13);
    SG_CHECK_EQUAL(tree->getStringValue("settings/render/foo"),
                   std::string("hello world"));

    donor.someMember = 45;
    donor.wrappedMember = "apples";
    donor.wrappedB =  5000.0;

    SG_CHECK_EQUAL(tree->getIntValue("position/body/a"), 45);
    SG_CHECK_EQUAL(tree->getStringValue("settings/render/foo"),
                   std::string("apples"));
    SG_CHECK_EQUAL(tree->getDoubleValue("position/body/mass"), 5000.0);

    // set value externally
    tree->setIntValue("position/body/a", 99);
    SG_CHECK_EQUAL(donor.someMember, 99);

    tree->setStringValue("settings/render/foo", "lemons");
    SG_CHECK_EQUAL(donor.wrappedMember, "lemons");

    // set read-only
    bool success = tree->setDoubleValue("position/body/mass", 10000.0);
    SG_VERIFY(!success);
    SG_CHECK_EQUAL(donor.wrappedB, 5000.0); // must not have changed

    // un-tieing

    tree->untie("position/body/a");
    tree->untie("position/body/mass");

    SG_CHECK_EQUAL(tree->getIntValue("position/body/a"), 99);
    SG_CHECK_EQUAL(tree->getDoubleValue("position/body/mass"), 5000.0);
}

void tiedPropertiesListeners()
{

    TiedPropertyDonor donor;

    SGPropertyNode_ptr tree(new SGPropertyNode);
    defineSamplePropertyTree(tree);

    tree->tie("position/body/a", SGRawValuePointer<int>(&donor.someMember));
    tree->tie("settings/render/foo", SGRawValueMethods<TiedPropertyDonor, const char*>(donor, &TiedPropertyDonor::getWrappedA, &TiedPropertyDonor::setWrappedA));
    tree->tie("position/body/mass", SGRawValueMethods<TiedPropertyDonor, double>(donor, &TiedPropertyDonor::getWrappedB, nullptr));

    // tie sets current values of the property onto the setter
    SG_CHECK_EQUAL(tree->getStringValue("settings/render/foo"),
                   std::string("flightgear"));
    SG_CHECK_EQUAL(tree->getIntValue("position/body/a"), 42);

    TestListener l(tree.get());

    tree->getNode("position/body/a")->addChangeListener(&l);
    tree->getNode("position/body/mass")->addChangeListener(&l);
    tree->getNode("settings/render/foo")->addChangeListener(&l);

    // firstly test changes via setXXX API and verify they work
    tree->setIntValue("position/body/a", 99);
    SG_CHECK_EQUAL(donor.someMember, 99);
    SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/a"), 1);

    tree->setDoubleValue("position/body/mass", -123.0);
    SG_CHECK_EQUAL(donor.wrappedB, 1.23); // read-only!
    SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/mass"), 0);

    tree->setStringValue("settings/render/foo", "thingA");
    tree->setStringValue("settings/render/foo", "thingB");
    SG_CHECK_EQUAL(donor.wrappedMember, std::string("thingB"));
    SG_CHECK_EQUAL(l.checkValueChangeCount("settings/render/foo"), 2);

    // now change values from inside the donor and verify it doesn't fire
    // the listener

    donor.wrappedMember = "pineapples";
    SG_CHECK_EQUAL(tree->getStringValue("settings/render/foo"),
                   std::string("pineapples"));
    SG_CHECK_EQUAL(l.checkValueChangeCount("settings/render/foo"), 2);

    donor.someMember = 256;
    SG_CHECK_EQUAL(tree->getIntValue("position/body/a"), 256);
    SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/a"), 1);

    // now fire value changed
    l.resetChangeCounts();
    donor.wrappedB = 4.000000001;


    tree->getNode("position/body/a")->fireValueChanged();
    SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/a"), 1);
    SG_CHECK_EQUAL(tree->getIntValue("position/body/a"), 256);

    l.resetChangeCounts();

    tree->getNode("position/body/a")->fireValueChanged();
    SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/a"), 1);

    tree->setDoubleValue("position/body/mass", 4.000000001);
    // Not changed
    SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/mass"), 0);

    donor.someMember = 970;
    SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/a"), 1); // not changed
    tree->getNode("position/body/a")->fireValueChanged();
    SG_CHECK_EQUAL(l.checkValueChangeCount("position/body/a"), 2); // changed


    donor.wrappedMember = "pears";
    // Not changed
    SG_CHECK_EQUAL(l.checkValueChangeCount("settings/render/foo"), 0);
    tree->getNode("settings/render/foo")->fireValueChanged();
    // Changed
    SG_CHECK_EQUAL(l.checkValueChangeCount("settings/render/foo"), 1);

}


void testDeleterListener()
{
    SGPropertyNode_ptr tree = new SGPropertyNode;
    defineSamplePropertyTree(tree);


    // recursive listen
    {
        TestListener* l = new TestListener(tree.get(), true /* recursive */);
        tree->getNode("position/body")->addChangeListener(l);
        tree->getNode("controls/")->addChangeListener(l);

        SG_CHECK_EQUAL(l->checkValueChangeCount("position/body/a"), 0);

        // create additional children
        tree->setFloatValue("position/body/sub/theta", 0.1234);

        SG_VERIFY(l->checkAdded("position/body/sub",
                                tree->getNode("position/body/sub/theta")));

        SG_CHECK_EQUAL(l->checkValueChangeCount("position/body/sub/theta"), 1);

        delete l;

        tree->setFloatValue("position/body/sub/theta", 99.123);
        tree->setIntValue("position/body/a", 33);

        // verify no listeners at all
        SG_VERIFY(ensureNListeners(tree, 0));
    }

    // Self-unregister. prior to 2019-07-08 this would segv.
    {
        std::shared_ptr<TestListener>   l1( new TestListener(tree.get(), true /* recursive */, true /*self_unregister*/));
        tree->setFloatValue("position/body/sub/self-unregister", 0.1);
        tree->getNode("position/body/sub/self-unregister")->addChangeListener(l1.get());
        tree->setFloatValue("position/body/sub/self-unregister", 0.2);
    }
}

int main (int ac, char ** av)
{
  test_value();
  test_property_nodes();

  for (int i = 1; i < ac; i++) {
    try {
    //  cout << "Reading " << av[i] << endl;
      SGPropertyNode root;
        readProperties(SGPath::fromLocal8Bit(av[i]), &root);
      writeProperties(cout, &root, true);
   //   cout << endl;
    } catch (std::string &message) {
      cout << "Aborted with " << message << endl;
    }
  }

  test_addChild();

    testListener();
    tiedPropertiesTest();
    tiedPropertiesListeners();
    testDeleterListener();

    // disable test for the moment
   // testAliasedListeners();

  return 0;
}
