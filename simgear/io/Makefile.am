includedir = @includedir@/io

lib_LIBRARIES = libsgio.a

include_HEADERS = \
	iochannel.hxx \
	lowlevel.hxx \
	sg_binobj.hxx \
	sg_file.hxx \
	sg_serial.hxx \
	sg_socket.hxx \
	sg_socket_udp.hxx

libsgio_a_SOURCES = \
	iochannel.cxx \
	lowlevel.cxx \
	sg_binobj.cxx \
	sg_file.cxx \
	sg_serial.cxx \
	sg_socket.cxx \
	sg_socket_udp.cxx 

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir)
else
INCLUDES = -I$(top_srcdir)
endif

if IS_MINGW
NETWORK_LIB = -lwsock32
else
NETWORK_LIB =
endif

noinst_PROGRAMS = decode_binobj socktest lowtest

socktest_SOURCES = socktest.cxx

socktest_LDADD = \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
        $(top_builddir)/simgear/io/libsgio.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(top_builddir)/simgear/xml/libsgxml.a \
	-lz

lowtest_SOURCES = lowtest.cxx

lowtest_LDADD = \
	$(top_builddir)/simgear/io/libsgio.a

decode_binobj_SOURCES = decode_binobj.cxx

decode_binobj_LDADD = \
	$(top_builddir)/simgear/io/libsgio.a \
        $(top_builddir)/simgear/bucket/libsgbucket.a \
        $(top_builddir)/simgear/misc/libsgmisc.a \
        $(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/xml/libsgxml.a \
	$(NETWORK_LIB) -lz
