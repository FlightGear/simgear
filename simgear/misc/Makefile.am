includedir = @includedir@/misc

if HAVE_ZLIB
ZLIB_INCL =
else
ZLIB_INCL = -I$(top_srcdir)/src/zlib
endif

lib_LIBRARIES = libsgmisc.a

include_HEADERS = \
        commands.hxx \
        exception.hxx
	props.hxx \
	sg_path.hxx \
	sgstream.hxx \
	stopwatch.hxx \
	strutils.hxx \
	texcoord.hxx \
	zfstream.hxx

libsgmisc_a_SOURCES = \
        commands.cxx \
        exception.cxx \
	props.cxx \
        props_io.cxx \
	sg_path.cxx \
	sgstream.cxx \
	strutils.cxx \
	texcoord.cxx \
	zfstream.cxx

noinst_PROGRAMS = props_test

props_test_SOURCES = props_test.cxx
props_test_LDADD = libsgmisc.a ../xml/libsgxml.a ../debug/libsgdebug.a

INCLUDES += -I$(top_srcdir) $(ZLIB_INCL)
