includedir = @includedir@/math

if HAVE_ZLIB
ZLIB_INCL =
else
ZLIB_INCL = -I$(top_builddir)/src/zlib
endif

lib_LIBRARIES = libsgmath.a

include_HEADERS = \
	fg_geodesy.hxx \
	fg_memory.h \
	fg_random.h \
	fg_types.hxx \
	interpolater.hxx \
	leastsqs.hxx \
	point3d.hxx \
	polar3d.hxx \
	vector.hxx

EXTRA_DIST = linintp2.h linintp2.inl sphrintp.h sphrintp.inl

libsgmath_a_SOURCES = \
	fg_geodesy.cxx \
	fg_random.c \
	interpolater.cxx \
	leastsqs.cxx \
	polar3d.cxx \
	vector.cxx

INCLUDES += -I$(top_builddir) $(ZLIB_INCL)
