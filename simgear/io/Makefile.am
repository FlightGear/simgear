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

INCLUDES = -I$(top_srcdir)

noinst_PROGRAMS = decode_binobj socktest lowtest tcp_server tcp_client

tcp_server_SOURCES = tcp_server.cxx

if HAVE_FRAMEWORK_PLIB
tcp_server_LDFLAGS = $(plib_FRAMEWORK)
else
tcp_server_PLIB_LIBS = -lplibnet -lplibul
endif

tcp_server_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(tcp_server_PLIB_LIBS) -lz \
	$(network_LIBS) \
	$(base_LIBS)

tcp_client_SOURCES = tcp_client.cxx

if HAVE_FRAMEWORK_PLIB
tcp_client_LDFLAGS = $(plib_FRAMEWORK)
else
tcp_client_PLIB_LIBS = -lplibnet -lplibul
endif

tcp_client_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(tcp_client_PLIB_LIBS) -lz \
	$(network_LIBS) \
	$(base_LIBS)

socktest_SOURCES = socktest.cxx

if HAVE_FRAMEWORK_PLIB
socktest_LDFLAGS = $(plib_FRAMEWORK)
else
socktest_PLIB_LIBS = -lplibnet -lplibul
endif

socktest_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
        $(socktest_PLIB_LIBS) -lz \
	$(network_LIBS) \
	$(base_LIBS) 

lowtest_SOURCES = lowtest.cxx

lowtest_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(base_LIBS) -lz

decode_binobj_SOURCES = decode_binobj.cxx

decode_binobj_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(base_LIBS) -lz
