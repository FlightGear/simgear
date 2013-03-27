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

#include <simgear/misc/test_macros.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/timestamp.hxx>

using namespace std;    
using namespace simgear;

bool testVar1 = false;
unsigned int testVar2 = 0;
unsigned int testVar3 = 0;

std::auto_ptr<SGEventMgr> mgr;
  
void testCb1()
{
  testVar1 = !testVar1;
}

void testCb2()
{
  ++testVar2;
}

void testCb4()
{
  ++testVar3;
}

void testBasic()
{
  mgr.reset(new SGEventMgr);
  
  SGTimeStamp n;
  n.stamp();
  
  mgr->addEvent("test-single", &testCb1, 1.2);
  
  testVar1 = false;
  while (n.elapsedMSec() < 1250) {
    bool shouldHaveFired = (n.elapsedMSec() >= 1200);
    mgr->update(0.0);
    
    
    VERIFY(testVar1 == shouldHaveFired);
    SGTimeStamp::sleepForMSec(1);
  } // of wait loop
  
}

void testRepeatWithoutDelay()
{
  cout << "========== testRepeatWithoutDelay" << endl;
    
  mgr.reset(new SGEventMgr);
  
  SGTimeStamp n;
  n.stamp();
  
  mgr->addTask("test-repeat", &testCb2, 0.1);
  
  testVar2 = 0;
  while (n.elapsedMSec() < 500) {
    mgr->update(0.0);
    
    if (n.elapsedMSec() < 100) {
      COMPARE(testVar2, 0);
    } else if (n.elapsedMSec() < 200) {
      COMPARE(testVar2, 1);
    }
   
    SGTimeStamp::sleepForMSec(1);
  } // of wait loop

  COMPARE(testVar2, 4);
}

void testOrdering()
{
    cout << "========== testOrdering" << endl;
    
    mgr.reset(new SGEventMgr);

    SGTimeStamp n;
    n.stamp();
  
    testVar2 = 0;
    testVar3 = 0;    
    mgr->addTask("repeat-fast", &testCb2, 0.1);
    mgr->addTask("repeat-slow", &testCb4, 0.4);
    
    while (n.elapsedMSec() < 200) {
      mgr->update(0.0);
      SGTimeStamp::sleepForMSec(1);
    } // of wait loop
    
    COMPARE(testVar2, 1);
    COMPARE(testVar3, 0);
}

void testCb3()
{
  mgr->removeTask("remove-me");
  testVar2 = 99;
}

void testRemoveWhileRunning()
{
    cout << "========== testRemoveWhileRunning" << endl;
    
  mgr.reset(new SGEventMgr);
  
  SGTimeStamp n;
  n.stamp();
  
  mgr->addTask("remove-me", &testCb3, 0.1);
  
  testVar2 = 0;
  while (n.elapsedMSec() < 200) {
    mgr->update(0.0);
    bool shouldBeZero = (n.elapsedMSec() < 100);
    VERIFY((testVar2 == 0) == shouldBeZero);
    SGTimeStamp::sleepForMSec(1);
  } // of wait loop

  COMPARE(testVar2, 99);
}

void testRemoveTask()
{
    cout << "========== testRemoveTask" << endl;
    
  mgr.reset(new SGEventMgr);
  testVar2 = 0;
  // should not fire!
  mgr->addTask("task-1", &testCb2, 0.1);
  mgr->removeTask("task-1");
  COMPARE(testVar2, 0);
}

void testSimTime()
{
  mgr.reset(new SGEventMgr);
  
  mgr->addEvent("sim-event", &testCb1, 0.1, true /* sim-time */);
  
  testVar1 = false;
  for (int i=0; i<6; ++i) {
    mgr->update(0.02);
  } // of wait loop
  
  COMPARE(testVar1, true)
}


void testNextIterationTime()
{
    cout << "========== testNextIterationTime" << endl;
  mgr.reset(new SGEventMgr);
  
  testVar2 = 0;
  mgr->addEvent("sim-event", &testCb2, 0.0, true /* sim-time */);
  mgr->addEvent("real-event", &testCb2, 0.0, false /* real-time */);
  
  mgr->update(0.0);
  
  // sim-event should not fire; real-event should
  COMPARE(testVar2, 1);
  
  mgr->addEvent("real-event2", &testCb2, 0.0, false /* real-time */);
  
  // now real-event2 and sim-event should fire
  mgr->update(0.00000001); // very small dt  
  COMPARE(testVar2, 3);
}

int main(int argc, char* argv[])
{
    sglog().setLogLevels( SG_ALL, SG_INFO );
 
    testBasic();
    testRepeatWithoutDelay();
    testRemoveWhileRunning();
    testRemoveTask();
    testSimTime();
    testNextIterationTime();
    testOrdering();
    
    cout << __FILE__ << ": All tests passed" << endl;
    return EXIT_SUCCESS;
}

