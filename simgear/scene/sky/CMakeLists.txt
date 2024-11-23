include (SimGearComponent)

set(HEADERS 
    CloudShaderGeometry.hxx
    cloud.hxx
    cloudfield.hxx
    dome.hxx
    galaxy.hxx
    moon.hxx
    newcloud.hxx
    oursun.hxx
    planets.hxx
    sky.hxx
    sphere.hxx
    stars.hxx
    )

set(SOURCES 
    CloudShaderGeometry.cxx
    cloud.cxx
    cloudfield.cxx
    dome.cxx
    galaxy.cxx
    moon.cxx
    newcloud.cxx
    oursun.cxx
    planets.cxx
    sky.cxx
    sphere.cxx
    stars.cxx
    )

simgear_scene_component(sky scene/sky "${SOURCES}" "${HEADERS}")
