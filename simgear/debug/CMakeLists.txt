
include (SimGearComponent)

set(HEADERS debug_types.h logstream.hxx BufferedLogCallback.hxx)
set(SOURCES logstream.cxx BufferedLogCallback.cxx)

simgear_component(debug debug "${SOURCES}" "${HEADERS}")