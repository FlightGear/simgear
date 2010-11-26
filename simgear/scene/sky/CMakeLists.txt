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

add_library(sgsky STATIC ${SOURCES} ${HEADERS} )

install (TARGETS sgsky ARCHIVE DESTINATION lib)
install (FILES ${HEADERS}  DESTINATION include/simgear/scene/sky)

