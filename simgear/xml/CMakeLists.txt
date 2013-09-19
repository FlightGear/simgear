
include (SimGearComponent)


set(HEADERS 
    easyxml.hxx
    )
    
set(SOURCES 
    easyxml.cxx
    )

simgear_component(xml xml "${SOURCES}" "${HEADERS}")
