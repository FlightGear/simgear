
include (SimGearComponent)

set(HEADERS metar.hxx visual_enviro.hxx precipitation.hxx)
set(SOURCES metar.cxx visual_enviro.cxx precipitation.cxx)

simgear_component(environment environment "${SOURCES}" "${HEADERS}")
