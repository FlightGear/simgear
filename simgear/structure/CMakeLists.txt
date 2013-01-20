
include (SimGearComponent)

set(HEADERS
    OSGUtils.hxx
    OSGVersion.hxx
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
    intern.hxx
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
add_executable(test_state_machine state_machine_test.cxx)
target_link_libraries(test_state_machine SimGearCore)
add_test(test_state_machine ${EXECUTABLE_OUTPUT_PATH}/test_state_machine)

endif(ENABLE_TESTS)
