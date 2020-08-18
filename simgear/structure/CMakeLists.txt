
include (SimGearComponent)

set(HEADERS
    OSGUtils.hxx
    SGAtomic.hxx
    SGBinding.hxx
    SGExpression.hxx
    SGReferenced.hxx
    SGSharedPtr.hxx
    SGSmplhist.hxx
    SGSmplstat.hxx
    SGWeakPtr.hxx
    SGWeakReferenced.hxx
    SGPerfMon.hxx
    singleton.hpp
    Singleton.hxx
    StringTable.hxx
    callback.hxx
    commands.hxx
    event_mgr.hxx
    exception.hxx
    function_list.hxx
    intern.hxx
    map.hxx
    subsystem_mgr.hxx
    StateMachine.hxx
    )

set(SOURCES
    SGAtomic.cxx
    SGBinding.cxx
    SGExpression.cxx
    SGSmplhist.cxx
    SGSmplstat.cxx
    SGPerfMon.cxx
    StringTable.cxx
    commands.cxx
    event_mgr.cxx
    exception.cxx
    subsystem_mgr.cxx
    StateMachine.cxx
    )

simgear_component(structure structure "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
  add_simgear_autotest(test_subsystems subsystem_test.cxx)
  add_simgear_autotest(test_state_machine state_machine_test.cxx)
  add_simgear_autotest(test_expressions expression_test.cxx)
  add_simgear_autotest(test_shared_ptr shared_ptr_test.cpp)
  add_simgear_autotest(test_commands test_commands.cxx)
endif(ENABLE_TESTS)

add_boost_test(function_list
  SOURCES function_list_test.cxx
  LIBRARIES SimGearCore
)
