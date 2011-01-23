
include (SimGearComponent)

set(HEADERS 
    celestialBody.hxx
    ephemeris.hxx
    jupiter.hxx
    mars.hxx
    mercury.hxx
    moonpos.hxx
    neptune.hxx
    pluto.hxx
    saturn.hxx
    star.hxx
    stardata.hxx
    uranus.hxx
    venus.hxx
    )
    
set(SOURCES 
    celestialBody.cxx
    ephemeris.cxx
    jupiter.cxx
    mars.cxx
    mercury.cxx
    moonpos.cxx
    neptune.cxx
    saturn.cxx
    star.cxx
    stardata.cxx
    uranus.cxx
    venus.cxx
    )

simgear_component(ephem ephemeris "${SOURCES}" "${HEADERS}")
