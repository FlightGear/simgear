include (SimGearComponent)

set(HEADERS
    ODGauge.hxx
    )

set(SOURCES
    ODGauge.cxx
    )

add_subdirectory(ShivaVG/src)

simgear_scene_component(canvas canvas "${SOURCES}" "${HEADERS}")
