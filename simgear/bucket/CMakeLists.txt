
include (SimGearComponent)

set(HEADERS newbucket.hxx)
set(SOURCES newbucket.cxx)

simgear_component(bucket bucket "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
    add_simgear_autotest(test_bucket test_bucket.cxx)
endif(ENABLE_TESTS)