include (SimGearComponent)

set(HEADERS 
    colors.hxx
    extensions.hxx
    screen-dump.hxx
    video-encoder.hxx
    )


set(SOURCES 
    extensions.cxx
    screen-dump.cxx
    video-encoder.cxx
    )

simgear_scene_component(screen screen "${SOURCES}" "${HEADERS}")
