include (SimGearComponent)


set(HEADERS
    SGBox.hxx
    SGCMath.hxx
    SGGeoc.hxx
    SGGeod.hxx
    SGGeodesy.hxx
    SGGeometry.hxx
    SGGeometryFwd.hxx
    SGIntersect.hxx
    SGLimits.hxx
    SGLineSegment.hxx
    SGLocation.hxx
    SGMath.hxx
    SGMathFwd.hxx
    SGMatrix.hxx
    SGMisc.hxx
    SGPlane.hxx
    SGQuat.hxx
    SGRay.hxx
    SGRect.hxx
    SGSphere.hxx
    SGTriangle.hxx
    SGVec2.hxx
    SGVec3.hxx
    SGVec4.hxx
    beziercurve.hxx
    interpolater.hxx
    leastsqs.hxx
    sg_geodesy.hxx
    sg_types.hxx
    sg_random.h
    )

set(SOURCES 
    SGGeodesy.cxx
    interpolater.cxx
    leastsqs.cxx
    sg_random.c
    )

simgear_component(math math "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
add_executable(math_test SGMathTest.cxx)
target_link_libraries(math_test SimGearCore)
add_test(math ${EXECUTABLE_OUTPUT_PATH}/math_test)

add_executable(geometry_test SGGeometryTest.cxx)
target_link_libraries(geometry_test SimGearCore)
add_test(geometry ${EXECUTABLE_OUTPUT_PATH}/geometry_test)
endif(ENABLE_TESTS)
