include (SimGearComponent)

set(HEADERS
    terrasync.hxx
    )

set(SOURCES 
    terrasync.cxx
    )

simgear_component(tsync scene/tsync "${SOURCES}" "${HEADERS}")
