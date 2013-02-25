
include (SimGearComponent)

set(HEADERS 
    ResourceManager.hxx
    interpolator.hxx
    sg_dir.hxx
    sg_path.hxx
    sgstream.hxx
    stdint.hxx
    stopwatch.hxx
    strutils.hxx
    tabbed_values.hxx
    texcoord.hxx
    zfstream.hxx
    gzcontainerfile.hxx
    )

set(SOURCES 
    ResourceManager.cxx
    interpolator.cxx
    sg_dir.cxx
    sg_path.cxx
    sgstream.cxx
    strutils.cxx
    tabbed_values.cxx
    texcoord.cxx
    zfstream.cxx
    gzcontainerfile.cxx
    )

simgear_component(misc misc "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

if (SIMGEAR_SHARED)
    set(TEST_LIBS SimGearCore)
else()
    set(TEST_LIBS SimGearCore ${CMAKE_THREAD_LIBS_INIT})
endif()

add_executable(test_tabbed_values tabbed_values_test.cxx)
add_test(tabbed_values ${EXECUTABLE_OUTPUT_PATH}/test_tabbed_values)
target_link_libraries(test_tabbed_values ${TEST_LIBS})

add_executable(test_strings strutils_test.cxx )
add_test(test_strings ${EXECUTABLE_OUTPUT_PATH}/test_strings)
target_link_libraries(test_strings ${TEST_LIBS})

add_executable(test_streams sgstream_test.cxx )
add_test(test_streams ${EXECUTABLE_OUTPUT_PATH}/test_streams)
target_link_libraries(test_streams ${TEST_LIBS})

add_executable(test_path path_test.cxx )
add_test(test_path ${EXECUTABLE_OUTPUT_PATH}/test_path)
target_link_libraries(test_path ${TEST_LIBS})
endif(ENABLE_TESTS)
