includedir = @includedir@/math

check_PROGRAMS  = SGMathTest
TESTS           = $(check_PROGRAMS)

SGMathTest_SOURCES = SGMathTest.cxx
SGMathTest_LDADD = libsgmath.a $(base_LIBS)


lib_LIBRARIES = libsgmath.a

include_HEADERS = \
	interpolater.hxx \
	leastsqs.hxx \
	point3d.hxx \
	polar3d.hxx \
	sg_geodesy.hxx \
	sg_memory.h \
	sg_random.h \
	sg_types.hxx \
	vector.hxx \
	SGCMath.hxx \
	SGGeoc.hxx \
	SGGeod.hxx \
	SGGeodesy.hxx \
	SGLimits.hxx \
	SGMatrix.hxx \
	SGMath.hxx \
	SGMathFwd.hxx \
	SGMisc.hxx \
	SGQuat.hxx \
	SGVec4.hxx \
	SGVec3.hxx


libsgmath_a_SOURCES = \
	interpolater.cxx \
	leastsqs.cxx \
	polar3d.cxx \
	sg_geodesy.cxx \
	sg_random.c \
	vector.cxx \
	SGGeodesy.cxx

INCLUDES = -I$(top_srcdir)
