includedir = @includedir@/io

lib_LIBRARIES = libsgio.a

include_HEADERS = \
	iochannel.hxx \
	sg_file.hxx \
	sg_serial.hxx \
	sg_socket.hxx

libsgio_a_SOURCES = \
	iochannel.cxx \
	sg_file.cxx \
	sg_serial.cxx \
	sg_socket.cxx 

INCLUDES += -I$(top_srcdir)

noinst_PROGRAMS = socktest

socktest_SOURCES = socktest.cxx

socktest_LDADD = \
	$(top_builddir)/simgear/io/libsgio.a \
	$(top_builddir)/simgear/debug/libsgdebug.a 