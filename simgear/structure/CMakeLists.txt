
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

add_executable(test_subsystems subsystem_test.cxx)
target_link_libraries(test_subsystems ${TEST_LIBS})
add_test(subsystems ${EXECUTABLE_OUTPUT_PATH}/test_subsystems)

add_executable(test_state_machine state_machine_test.cxx)
target_link_libraries(test_state_machine ${TEST_LIBS})
add_test(state_machine ${EXECUTABLE_OUTPUT_PATH}/test_state_machine)

add_executable(test_expressions expression_test.cxx)
target_link_libraries(test_expressions ${TEST_LIBS})
add_test(expressions ${EXECUTABLE_OUTPUT_PATH}/test_expressions)

add_executable(test_shared_ptr shared_ptr_test.cpp)
target_link_libraries(test_shared_ptr ${TEST_LIBS})
add_test(shared_ptr ${EXECUTABLE_OUTPUT_PATH}/test_shared_ptr)

add_executable(test_commands test_commands.cxx)
target_link_libraries(test_commands ${TEST_LIBS})
add_test(subsystems ${EXECUTABLE_OUTPUT_PATH}/test_commands)

endif(ENABLE_TESTS)

add_boost_test(function_list
  SOURCES function_list_test.cxx
  LIBRARIES ${TEST_LIBS}
)
