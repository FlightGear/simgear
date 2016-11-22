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
    simd.hxx
    simd4x4.hxx
    )

set(SOURCES 
    SGGeodesy.cxx
    interpolater.cxx
    leastsqs.cxx
    sg_random.c
    )

simgear_component(math math "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

add_executable(sgvec4_test test_sgvec4.cxx)
target_link_libraries(sgvec4_test ${TEST_LIBS})
add_test(sgvec4 ${EXECUTABLE_OUTPUT_PATH}/sgvec4_test)

add_executable(math_test SGMathTest.cxx)
target_link_libraries(math_test ${TEST_LIBS})
add_test(math ${EXECUTABLE_OUTPUT_PATH}/math_test)

add_executable(geometry_test SGGeometryTest.cxx)
target_link_libraries(geometry_test ${TEST_LIBS})
add_test(geometry ${EXECUTABLE_OUTPUT_PATH}/geometry_test)

endif(ENABLE_TESTS)
