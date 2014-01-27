include (SimGearComponent)

set(HEADERS 
    colors.hxx
    extensions.hxx
    screen-dump.hxx
	tr.h
    )


set(SOURCES 
    extensions.cxx
    screen-dump.cxx
	tr.cxx
    )

simgear_scene_component(screen screen "${SOURCES}" "${HEADERS}")