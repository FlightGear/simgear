
include (SimGearComponent)

set(HEADERS magvar.hxx coremag.hxx)
set(SOURCES magvar.cxx coremag.cxx)

simgear_component(magvar magvar "${SOURCES}" "${HEADERS}")