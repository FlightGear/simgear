
include (SimGearComponent)

set(HEADERS serial.hxx)
set(SOURCES serial.cxx)

simgear_component(serial serial "${SOURCES}" "${HEADERS}")