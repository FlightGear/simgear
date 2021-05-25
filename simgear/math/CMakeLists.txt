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
    sg_random.hxx
    simd.hxx
    simd4x4.hxx
    )

set(SOURCES 
    SGGeodesy.cxx
    interpolater.cxx
    leastsqs.cxx
    sg_random.cxx
    )

simgear_component(math math "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

add_simgear_autotest(sgvec4_test test_sgvec4.cxx)
add_simgear_autotest(math_test SGMathTest.cxx)
add_simgear_autotest(geometry_test SGGeometryTest.cxx)

endif(ENABLE_TESTS)
