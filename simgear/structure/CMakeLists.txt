
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

add_executable(test_state_machine state_machine_test.cxx)
target_link_libraries(test_state_machine ${TEST_LIBS})
add_test(state_machine ${EXECUTABLE_OUTPUT_PATH}/test_state_machine)

add_executable(test_expressions expression_test.cxx)
target_link_libraries(test_expressions ${TEST_LIBS})
add_test(expressions ${EXECUTABLE_OUTPUT_PATH}/test_expressions)

endif(ENABLE_TESTS)

add_boost_test(shared_ptr
  SOURCES shared_ptr_test.cpp
  LIBRARIES ${TEST_LIBS}
)
