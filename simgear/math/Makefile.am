lib_LIBRARIES = libsgmath.a

include_HEADERS = \
	fg_geodesy.hxx \
	fg_memory.h \
	fg_random.h \
	fg_types.hxx \
	interpolater.hxx \
	leastsqs.hxx \
	mat3.h \
	point3d.hxx \
	polar3d.hxx \
	vector.hxx

EXTRA_DIST = linintp2.h linintp2.inl sphrintp.h sphrintp.inl

libsgmath_a_SOURCES = \
	MAT3geom.c \
	MAT3inv.c \
	MAT3mat.c \
	MAT3vec.c \
	fg_geodesy.cxx \
	fg_random.c \
	interpolater.cxx \
	leastsqs.cxx \
	mat3defs.h mat3err.h \
	polar3d.cxx \
	vector.cxx

INCLUDES += -I$(top_builddir)/src
