includedir = @includedir@/math

if HAVE_ZLIB
ZLIB_INCL =
else
ZLIB_INCL = -I$(top_srcdir)/src/zlib
endif

lib_LIBRARIES = libsgmath.a

include_HEADERS = \
	interpolater.hxx \
	leastsqs.hxx \
	localconsts.hxx \
	point3d.hxx \
	polar3d.hxx \
	sg_geodesy.hxx \
	sg_memory.h \
	sg_random.h \
	sg_types.hxx \
	vector.hxx

EXTRA_DIST = linintp2.h linintp2.inl sphrintp.h sphrintp.inl

libsgmath_a_SOURCES = \
	interpolater.cxx \
	leastsqs.cxx \
	polar3d.cxx \
	sg_geodesy.cxx \
	sg_random.c \
	vector.cxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) $(ZLIB_INCL)
else
INCLUDES = -I$(top_srcdir) $(ZLIB_INCL)
endif
