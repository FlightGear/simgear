includedir = @includedir@/math

if HAVE_ZLIB
ZLIB_INCL =
else
ZLIB_INCL = -I$(top_srcdir)/src/zlib
endif

lib_LIBRARIES = libsgmath.a

include_HEADERS = \
	fg_memory.h \
	fg_random.h \
	interpolater.hxx \
	leastsqs.hxx \
	localconsts.hxx \
	point3d.hxx \
	polar3d.hxx \
	sg_geodesy.hxx \
	sg_types.hxx \
	vector.hxx

EXTRA_DIST = linintp2.h linintp2.inl sphrintp.h sphrintp.inl

libsgmath_a_SOURCES = \
	fg_random.c \
	interpolater.cxx \
	leastsqs.cxx \
	polar3d.cxx \
	sg_geodesy.cxx \
	vector.cxx

INCLUDES += -I$(top_srcdir) $(ZLIB_INCL)
