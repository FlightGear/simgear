includedir = @includedir@/serial

noinst_PROGRAMS = testserial

lib_LIBRARIES = libsgserial.a

include_HEADERS = serial.hxx

libsgserial_a_SOURCES = serial.cxx

testserial_SOURCES = testserial.cxx

testserial_LDADD = \
	$(top_builddir)/src/serial/libsgserial.a \
	$(top_builddir)/src/debug/libsgdebug.a

INCLUDES += -I$(top_builddir)/src
