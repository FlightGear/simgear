includedir = @includedir@/route

lib_LIBRARIES = libsgroute.a

include_HEADERS = waypoint.hxx

libsgroute_a_SOURCES = \
	waypoint.cxx

INCLUDES += -I$(top_srcdir)

noinst_PROGRAMS = waytest

waytest_SOURCES = waytest.cxx

waytest_LDADD = \
	$(top_builddir)/simgear/route/libsgroute.a \
	$(top_builddir)/simgear/math/libsgmath.a \
	$(top_builddir)/simgear/debug/libsgdebug.a