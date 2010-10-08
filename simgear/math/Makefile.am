includedir = @includedir@/math

check_PROGRAMS  = SGMathTest SGGeometryTest
TESTS           = $(check_PROGRAMS)

SGMathTest_SOURCES = SGMathTest.cxx

SGMathTest_LDADD = \
	libsgmath.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
	$(base_LIBS)

SGGeometryTest_SOURCES = SGGeometryTest.cxx

SGGeometryTest_LDADD = \
	libsgmath.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
	$(base_LIBS)

lib_LIBRARIES = libsgmath.a

include_HEADERS = \
	interpolater.hxx \
	leastsqs.hxx \
	sg_geodesy.hxx \
	sg_random.h \
	sg_types.hxx \
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
	SGVec4.hxx \
	beziercurve.hxx \
	project.hxx

libsgmath_a_SOURCES = \
	interpolater.cxx \
	leastsqs.cxx \
	sg_random.c \
	SGGeod.cxx \
	SGGeodesy.cxx \
	project.cxx

INCLUDES = -I$(top_srcdir)
