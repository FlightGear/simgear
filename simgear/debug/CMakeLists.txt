
include (SimGearComponent)

set(HEADERS debug_types.h logstream.hxx)
set(SOURCES logstream.cxx)

simgear_component(debug debug "${SOURCES}" "${HEADERS}")