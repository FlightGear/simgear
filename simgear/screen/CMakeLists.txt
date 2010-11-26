include (SimGearComponent)

set(HEADERS 
    colors.hxx
    extensions.hxx
    screen-dump.hxx
    )


set(SOURCES 
    extensions.cxx
    screen-dump.cxx
    )

if(${JPEG_FACTORY})
    set(HEADERS "${HEADERS} jpgfactory.hxx")
    set(SOURCES "${SOURCES} jpgfactory.cxx tr.cxx")
endif()


simgear_component(screen screen "${SOURCES}" "${HEADERS}")