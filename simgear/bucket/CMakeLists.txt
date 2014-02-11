
include (SimGearComponent)

set(HEADERS newbucket.hxx)
set(SOURCES newbucket.cxx)

simgear_component(bucket bucket "${SOURCES}" "${HEADERS}")


if(ENABLE_TESTS)

add_executable(test_bucket test_bucket.cxx)
add_test(test_bucket ${EXECUTABLE_OUTPUT_PATH}/test_bucket)
target_link_libraries(test_bucket ${TEST_LIBS})

endif(ENABLE_TESTS)