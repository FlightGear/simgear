
include (SimGearComponent)

set(HEADERS magvar.hxx coremag.hxx)
set(SOURCES magvar.cxx coremag.cxx)

simgear_component(magvar magvar "${SOURCES}" "${HEADERS}")


add_executable(test_magvar testmagvar.cxx )
target_link_libraries(test_magvar sgmagvar)

