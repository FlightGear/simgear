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
    set(HEADERS "${HEADERS} jpgfactory.hxx")
    set(SOURCES "${SOURCES} jpgfactory.cxx")
endif()


simgear_component(screen screen "${SOURCES}" "${HEADERS}")