include (SimGearComponent)

set(HEADERS 
    sample_group.hxx
    sample_openal.hxx
    sample_queue.hxx
    soundmgr_openal.hxx
    xmlsound.hxx
    )
    
set(SOURCES 
    sample_group.cxx
    sample_openal.cxx
    sample_queue.cxx
    soundmgr_openal.cxx
    xmlsound.cxx
    )

simgear_component(sound sound "${SOURCES}" "${HEADERS}")