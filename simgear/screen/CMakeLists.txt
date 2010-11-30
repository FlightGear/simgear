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

if(${JPEG_FACTORY})
    list(APPEND HEADERS jpgfactory.hxx)
    list(APPEND SOURCES jpgfactory.cxx)
endif()


simgear_component(screen screen "${SOURCES}" "${HEADERS}")