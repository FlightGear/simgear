includedir = @includedir@/misc

if HAVE_ZLIB
ZLIB_INCL =
else
ZLIB_INCL = -I$(top_builddir)/src/zlib
endif

lib_LIBRARIES = libsgmisc.a

include_HEADERS = \
	fgpath.hxx \
	fgstream.hxx \
	stopwatch.hxx \
	strutils.hxx \
	texcoord.hxx \
	zfstream.hxx

libsgmisc_a_SOURCES = \
	fgpath.cxx \
	fgstream.cxx \
	strutils.cxx \
	texcoord.cxx \
	zfstream.cxx

INCLUDES += -I$(top_builddir) $(ZLIB_INCL)
