
include (SimGearComponent)

set(HEADERS magvar.hxx coremag.hxx)
set(SOURCES magvar.cxx coremag.cxx)

simgear_component(magvar magvar "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
add_executable(test_magvar testmagvar.cxx )

if (SIMGEAR_SHARED)
    target_link_libraries(test_magvar SimGearCore)
else()
    target_link_libraries(test_magvar sgmagvar)
endif()

endif(ENABLE_TESTS)
