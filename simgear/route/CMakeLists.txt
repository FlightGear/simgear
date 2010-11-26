
include (SimGearComponent)

set(HEADERS 
    route.hxx
    waypoint.hxx
    )

set(SOURCES 
    route.cxx
    waypoint.cxx
    )

simgear_component(route route "${SOURCES}" "${HEADERS}")