includedir = @includedir@/serial

noinst_PROGRAMS = testserial

lib_LIBRARIES = libsgserial.a

include_HEADERS = serial.hxx

libsgserial_a_SOURCES = serial.cxx

testserial_SOURCES = testserial.cxx

testserial_LDADD = \
	$(top_builddir)/simgear/serial/libsgserial.a \
	$(top_builddir)/simgear/debug/libsgdebug.a

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir)
else
INCLUDES = -I$(top_srcdir)
endif
