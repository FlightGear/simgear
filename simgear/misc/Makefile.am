includedir = @includedir@/misc

lib_LIBRARIES = libsgmisc.a

include_HEADERS = \
        commands.hxx \
        exception.hxx \
	props.hxx \
  props_io.hxx \
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

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir)
else
INCLUDES = -I$(top_srcdir)
endif
