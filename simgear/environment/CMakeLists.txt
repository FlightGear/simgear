
include (SimGearComponent)

set(HEADERS metar.hxx precipitation.hxx)
set(SOURCES metar.cxx precipitation.cxx)

simgear_component(environment environment "${SOURCES}" "${HEADERS}")
