includedir = @includedir@/route

lib_LIBRARIES = libsgroute.a

include_HEADERS = route.hxx waypoint.hxx

libsgroute_a_SOURCES = \
	route.cxx \
	waypoint.cxx

INCLUDES = -I$(top_srcdir)

noinst_PROGRAMS = waytest routetest

waytest_SOURCES = waytest.cxx

waytest_LDADD = \
	$(top_builddir)/simgear/route/libsgroute.a \
	$(top_builddir)/simgear/math/libsgmath.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(base_LIBS) \
	-lz

routetest_SOURCES = routetest.cxx

routetest_LDADD = \
	$(top_builddir)/simgear/route/libsgroute.a \
	$(top_builddir)/simgear/math/libsgmath.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(base_LIBS)
