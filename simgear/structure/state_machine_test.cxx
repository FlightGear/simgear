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

#include "StateMachine.hxx"

#include <simgear/structure/SGBinding.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/commands.hxx>

using std::string;
using std::cout;
using std::cerr;
using std::endl;

// SGCondition subclass we can trivially manipulate from test code.
class DummyCondition : public SGCondition
{
public:
    DummyCondition(): _state(false) { }
    
    virtual bool test() const
    {
        return _state;
    }
    
    bool _state;
};

class DummyThing
{
public:
    DummyThing() : dummy_cmd_state(0) { }
    
    bool someCommand(const SGPropertyNode* arg)
    {
        dummy_cmd_state++;
        return true;
    }
    
    int dummy_cmd_state;
};

#define COMPARE(a, b) \
    if ((a) != (b))  { \
        cerr << "failed:" << #a << " != " << #b << endl; \
        cerr << "\tgot:'" << a << "'" << endl; \
        exit(1); \
    }

#define VERIFY(a) \
    if (!(a))  { \
        cerr << "failed:" << #a << endl; \
        exit(1); \
    }
    
using namespace simgear;

#define BUILD_MACHINE_1() \
    StateMachine_ptr sm(new StateMachine); \
    StateMachine::State_ptr stateA = sm->createState("a"); \
    StateMachine::State_ptr stateB = sm->createState("b"); \
    StateMachine::State_ptr stateC = sm->createState("c"); \
    \
    DummyCondition* trigger1 = new DummyCondition; \
    StateMachine::Transition_ptr t1 = sm->createTransition(">b", stateB); \
    t1->addSourceState(stateA); \
    t1->setTriggerCondition(trigger1); \
    \
    DummyCondition* trigger2 = new DummyCondition; \
    StateMachine::Transition_ptr t2 = sm->createTransition(">c", stateC); \
    t2->addSourceState(stateB); \
    t2->setTriggerCondition(trigger2); \
    \
    DummyCondition* trigger3 = new DummyCondition; \
    StateMachine::Transition_ptr t3 = sm->createTransition(">a", stateA); \
    t3->addSourceState(stateC); \
    t3->addSourceState(stateB); \
    t3->setTriggerCondition(trigger3); \
    sm->init();

void testBasic()
{
    BUILD_MACHINE_1();
////////////////////////////////////////////
    COMPARE(sm->state()->name(), "a");
    
    COMPARE(sm->indexOfState(stateA), 0);
    COMPARE(sm->findStateByName("c"), stateC);
    
    sm->changeToState(stateC);
    COMPARE(sm->state(), stateC);
    
    trigger3->_state = true;
    sm->update(1.0);
    COMPARE(sm->state()->name(), "a");
    trigger3->_state = false;
    
    trigger1->_state = true;
    sm->update(1.0);
    trigger1->_state = false;
    COMPARE(sm->state()->name(), "b");
    
    trigger3->_state = true;
    sm->update(1.0);
    COMPARE(sm->state()->name(), "a");
    trigger3->_state = false;
    
    trigger1->_state = true;
    sm->update(1.0);
    trigger1->_state = false;
    COMPARE(sm->state()->name(), "b");
    
    trigger2->_state = true;
    sm->update(1.0);
    trigger2->_state = false;
    COMPARE(sm->state()->name(), "c");
    
//////////////////////////////////////////
    COMPARE(sm->root()->getIntValue("current-index"), 2);
    COMPARE(sm->root()->getStringValue("current-name"), string("c"));
    
    sm->root()->setStringValue("current-name", "b");
    COMPARE(sm->state()->name(), "b");

////////////////////////////////////////
    COMPARE(sm->findStateByName("foo"), NULL);
    COMPARE(sm->indexOfState(StateMachine::State_ptr()), -1);
    
    COMPARE(sm->stateByIndex(1), stateB);
    
    try {
        sm->stateByIndex(44);
        VERIFY(false && "should have raised an exception");
    } catch (sg_exception& e){
        // expected!
    }
}

void testNoSourcesTransition()
{
    BUILD_MACHINE_1();

    DummyCondition* trigger4 = new DummyCondition;
    StateMachine::Transition_ptr t4 = sm->createTransition("alwaysToA", stateA); \
    t4->setTriggerCondition(trigger4); \
    sm->init();
    
    COMPARE(sm->state()->name(), "a");
    trigger1->_state = true;
    sm->update(1.0);
    trigger1->_state = false;
    COMPARE(sm->state()->name(), "b");
    
    trigger4->_state = true;
    sm->update(1.0);
    trigger4->_state = false;
    COMPARE(sm->state()->name(), "a");
}

void testBindings()
{    
    SGCommandMgr* cmdMgr = SGCommandMgr::instance();
    DummyThing thing;
    cmdMgr->addCommand("dummy", &thing, &DummyThing::someCommand);
    BUILD_MACHINE_1();
    
    t2->addBinding(new SGBinding("dummy"));
    
    stateA->addEntryBinding(new SGBinding("dummy"));
    stateA->addExitBinding(new SGBinding("dummy"));
    stateC->addEntryBinding(new SGBinding("dummy"));
    
////////////////////////
    COMPARE(sm->state()->name(), "a");
    trigger1->_state = true;
    sm->update(1.0);
    trigger1->_state = false;
    COMPARE(sm->state()->name(), "b");
    COMPARE(thing.dummy_cmd_state, 1); // exit state A
    
    trigger2->_state = true;
    sm->update(1.0);
    trigger2->_state = false;
    COMPARE(thing.dummy_cmd_state, 3); // fire transition 2, enter state C
    
    thing.dummy_cmd_state = 0;
    sm->changeToState(stateA);
    COMPARE(thing.dummy_cmd_state, 1); // enter state A
    trigger1->_state = true;
    sm->update(1.0);
    trigger1->_state = false;
    COMPARE(thing.dummy_cmd_state, 2); // exit state A
    
////////////////////////
    t3->addBinding(new SGBinding("dummy"));
    t3->addBinding(new SGBinding("dummy"));
    t3->addBinding(new SGBinding("dummy"));
    
    sm->changeToStateName("b");
    thing.dummy_cmd_state = 0;
    trigger3->_state = true;
    sm->update(1.0);
    trigger3->_state = false;
    COMPARE(thing.dummy_cmd_state, 4); // three transition bindings, enter A
}

void testParse()
{
    const char* xml = "<?xml version=\"1.0\"?>"
        "<PropertyList>"
            "<state>"
                "<name>one</name>"
            "</state>"
            "<state>"
                "<name>two</name>"
            "</state>"
        "</PropertyList>";
    
    SGPropertyNode* desc = new SGPropertyNode;
    readProperties(xml, strlen(xml), desc);
    
    SGPropertyNode_ptr root(new SGPropertyNode);
    StateMachine_ptr sm = StateMachine::createFromPlist(desc, root);
    
    VERIFY(sm->findStateByName("one") != NULL);
    VERIFY(sm->findStateByName("two") != NULL);
}

int main(int argc, char* argv[])
{
    testBasic();
    testBindings();
    testParse();
    testNoSourcesTransition();
    
    cout << __FILE__ << ": All tests passed" << endl;
    return EXIT_SUCCESS;
}

