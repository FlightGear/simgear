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
	libsgroute.a \
	$(top_builddir)/simgear/math/libsgmath.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(top_builddir)/simgear/props/libsgprops.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
	$(top_builddir)/simgear/xml/libsgxml.a \
	$(base_LIBS) \
	-lz

routetest_SOURCES = routetest.cxx

routetest_LDADD = \
	libsgroute.a \
	$(top_builddir)/simgear/math/libsgmath.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
	$(base_LIBS)
