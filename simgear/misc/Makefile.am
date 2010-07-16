includedir = @includedir@/misc

lib_LIBRARIES = libsgmisc.a

include_HEADERS = \
	sg_path.hxx \
	sgstream.hxx \
	stopwatch.hxx \
	strutils.hxx \
	tabbed_values.hxx \
	texcoord.hxx \
	zfstream.hxx \
	interpolator.hxx \
	stdint.hxx \
	PathOptions.hxx \
	sg_dir.hxx

libsgmisc_a_SOURCES = \
	sg_path.cxx \
	sgstream.cxx \
	strutils.cxx \
	tabbed_values.cxx \
	texcoord.cxx \
	zfstream.cxx \
	interpolator.cxx \
	PathOptions.cxx \
	sg_dir.cxx

#noinst_PROGRAMS = tabbed_value_test swap_test

#tabbed_value_test_SOURCES = tabbed_values_test.cxx
#tabbed_value_test_LDADD = \
#	libsgmisc.a \
#	$(top_builddir)/simgear/xml/libsgxml.a \
#	$(top_builddir)/simgear/debug/libsgdebug.a

$(top_builddir)/simgear/xml/libsgxml.a:
	cd $(top_builddir)/simgear/xml && $(MAKE) $(AM_MAKEFLAGS) libsgxml.a

$(top_builddir)/simgear/debug/libsgdebug.a:
	cd $(top_builddir)/simgear/debug && $(MAKE) $(AM_MAKEFLAGS) libsgdebug.a

#swap_test_SOURCES = swap_test.cpp

INCLUDES = -I$(top_srcdir)
