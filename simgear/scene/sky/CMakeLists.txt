include (SimGearComponent)

set(HEADERS 
    CloudShaderGeometry.hxx
    cloud.hxx
    cloudfield.hxx
    dome.hxx
    moon.hxx
    newcloud.hxx
    oursun.hxx
    sky.hxx
    sphere.hxx
    stars.hxx
    )

set(SOURCES 
    CloudShaderGeometry.cxx
    cloud.cxx
    cloudfield.cxx
    dome.cxx
    moon.cxx
    newcloud.cxx
    oursun.cxx
    sky.cxx
    sphere.cxx
    stars.cxx
    )

simgear_scene_component(sky scene/sky "${SOURCES}" "${HEADERS}")
