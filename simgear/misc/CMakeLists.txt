
include (SimGearComponent)

set(HEADERS 
    PathOptions.hxx
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
    
    )

set(SOURCES 
    PathOptions.cxx
    ResourceManager.cxx
    interpolator.cxx
    sg_dir.cxx
    sg_path.cxx
    sgstream.cxx
    strutils.cxx
    tabbed_values.cxx
    texcoord.cxx
    zfstream.cxx
    )

simgear_component(misc misc "${SOURCES}" "${HEADERS}")

add_executable(test_tabbed_values tabbed_values_test.cxx)
add_test(tabbed_values ${EXECUTABLE_OUTPUT_PATH}/test_tabbed_values)
target_link_libraries(test_tabbed_values sgmisc)

add_executable(test_strings strutils_test.cxx )
add_test(test_strings ${EXECUTABLE_OUTPUT_PATH}/test_strings)
target_link_libraries(test_strings sgmisc)

add_executable(test_path path_test.cxx )
add_test(test_path ${EXECUTABLE_OUTPUT_PATH}/test_path)
target_link_libraries(test_path sgmisc sgdebug)

