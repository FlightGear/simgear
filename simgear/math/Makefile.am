includedir = @includedir@/math

check_PROGRAMS  = SGMathTest SGGeometryTest
TESTS           = $(check_PROGRAMS)

SGMathTest_SOURCES = SGMathTest.cxx
SGMathTest_LDADD = libsgmath.a $(base_LIBS)

SGGeometryTest_SOURCES = SGGeometryTest.cxx
SGGeometryTest_LDADD = libsgmath.a $(base_LIBS)

lib_LIBRARIES = libsgmath.a

include_HEADERS = \
	interpolater.hxx \
	leastsqs.hxx \
	point3d.hxx \
	polar3d.hxx \
	sg_geodesy.hxx \
	sg_random.h \
	sg_types.hxx \
	vector.hxx \
	Math.hxx \
	SGBox.hxx \
	SGCMath.hxx \
	SGGeoc.hxx \
	SGGeod.hxx \
	SGGeodesy.hxx \
	SGGeometry.hxx \
	SGGeometryFwd.hxx \
	SGIntersect.hxx \
	SGLimits.hxx \
	SGLineSegment.hxx \
	SGMatrix.hxx \
	SGMath.hxx \
	SGMathFwd.hxx \
	SGMisc.hxx \
	SGPlane.hxx \
	SGQuat.hxx \
	SGRay.hxx \
	SGSphere.hxx \
	SGTriangle.hxx \
	SGVec2.hxx \
	SGVec3.hxx \
	SGVec4.hxx

libsgmath_a_SOURCES = \
	interpolater.cxx \
	leastsqs.cxx \
	sg_random.c \
	vector.cxx \
	SGGeod.cxx \
	SGGeodesy.cxx

INCLUDES = -I$(top_srcdir)
