
include (SimGearComponent)

set(HEADERS 
    PathOptions.hxx
    ResourceManager.hxx
    interpolator.hxx
    sg_dir.hxx
    sg_path.hxx
    sg_sleep.hxx
    sgstream.hxx
    stdint.hxx
    stopwatch.hxx
    strutils.hxx
    tabbed_values.hxx
    texcoord.hxx
    zfstream.hxx
    
    )

set(SOURCES 
    PathOptions.cxx
    ResourceManager.cxx
    interpolator.cxx
    sg_dir.cxx
    sg_path.cxx
    sg_sleep.cxx
    sgstream.cxx
    strutils.cxx
    tabbed_values.cxx
    texcoord.cxx
    zfstream.cxx
    )

simgear_component(misc misc "${SOURCES}" "${HEADERS}")
