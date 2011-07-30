includedir = @includedir@/props

lib_LIBRARIES = libsgprops.a

include_HEADERS = \
	condition.hxx \
	props.hxx \
	props_io.hxx \
	propsfwd.hxx \
	AtomicChangeListener.hxx \
	ExtendedPropertyAdapter.hxx \
	propertyObject.hxx \
	tiedpropertylist.hxx

libsgprops_a_SOURCES = \
	condition.cxx \
	props.cxx \
	props_io.cxx \
	AtomicChangeListener.cxx \
	propertyObject.cxx

check_PROGRAMS = props_test propertyObject_test
TESTS           = propertyObject_test

props_test_SOURCES = props_test.cxx
props_test_LDADD = \
	libsgprops.a \
	$(top_builddir)/simgear/xml/libsgxml.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/structure/libsgstructure.a

propertyObject_test_SOURCES = propertyObject_test.cxx
propertyObject_test_LDADD = \
	libsgprops.a \
	$(top_builddir)/simgear/xml/libsgxml.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/structure/libsgstructure.a


if HAVE_FRAMEWORK_OSG
props_test_LDFLAGS = $(openthreads_FRAMEWORK)
else
props_test_LDFLAGS = -lOpenThreads
endif

INCLUDES = -I$(top_srcdir)
