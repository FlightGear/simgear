
include (SimGearComponent)

set(HEADERS newbucket.hxx)
set(SOURCES newbucket.cxx)

simgear_component(bucket bucket "${SOURCES}" "${HEADERS}")