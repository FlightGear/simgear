includedir = @includedir@/misc

lib_LIBRARIES = libsgmisc.a

include_HEADERS = \
	sg_path.hxx \
	sgstream.hxx \
	stopwatch.hxx \
	strutils.hxx \
	tabbed_values.hxx \
	texcoord.hxx \
	zfstream.hxx

libsgmisc_a_SOURCES = \
	sg_path.cxx \
	sgstream.cxx \
	strutils.cxx \
	tabbed_values.cxx \
	texcoord.cxx \
	zfstream.cxx

noinst_PROGRAMS = tabbed_value_test

tabbed_value_test_SOURCES = tabbed_values_test.cxx
tabbed_value_test_LDADD = \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(top_builddir)/simgear/xml/libsgxml.a \
	$(top_builddir)/simgear/debug/libsgdebug.a

INCLUDES = -I$(top_srcdir)
