includedir = @includedir@/misc

lib_LIBRARIES = libsgmisc.a

noinst_HEADERS = \
	colours.h

include_HEADERS = \
        commands.hxx \
        exception.hxx \
	props.hxx \
	props_io.hxx \
	sg_path.hxx \
	sgstream.hxx \
	stopwatch.hxx \
	strutils.hxx \
	tabbed_values.hxx \
	texture.hxx \
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
	tabbed_values.cxx \
	texture.cxx \
	texcoord.cxx \
	zfstream.cxx

noinst_PROGRAMS = props_test tabbed_value_test

props_test_SOURCES = props_test.cxx
props_test_LDADD = libsgmisc.a ../xml/libsgxml.a ../debug/libsgdebug.a

tabbed_value_test_SOURCES = tabbed_values_test.cxx
tabbed_value_test_LDADD = libsgmisc.a ../xml/libsgxml.a ../debug/libsgdebug.a

INCLUDES = -I$(top_srcdir)
