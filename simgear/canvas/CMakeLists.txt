include (SimGearComponent)

set(HEADERS
    ODGauge.hxx
    )

set(SOURCES
    ODGauge.cxx
    )

simgear_scene_component(canvas canvas "${SOURCES}" "${HEADERS}")
