include (SimGearComponent)


set(HEADERS 
    Math.hxx
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
    SGMath.hxx
    SGMathFwd.hxx
    SGMatrix.hxx
    SGMisc.hxx
    SGPlane.hxx
    SGQuat.hxx
    SGRay.hxx
    SGSphere.hxx
    SGTriangle.hxx
    SGVec2.hxx
    SGVec3.hxx
    SGVec4.hxx
    beziercurve.hxx
    interpolater.hxx
    leastsqs.hxx
    project.hxx
    sg_geodesy.hxx
    sg_types.hxx
	sg_random.h
    )

set(SOURCES 
    SGGeod.cxx
    SGGeodesy.cxx
    interpolater.cxx
    leastsqs.cxx
    project.cxx
	sg_random.c
    )

simgear_component(math math "${SOURCES}" "${HEADERS}")

