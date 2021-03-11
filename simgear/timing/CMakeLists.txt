

include (SimGearComponent)

set(HEADERS 
    sg_time.hxx
    timestamp.hxx
    timezone.h
    zonedetect.h
    lowleveltime.h
    )
    
set(SOURCES 
    lowleveltime.cxx
    sg_time.cxx
    timestamp.cxx
    timezone.cxx
    zonedetect.c
    )

simgear_component(timing timing "${SOURCES}" "${HEADERS}")
