includedir = @includedir@/io

lib_LIBRARIES = libsgio.a

include_HEADERS = \
	iochannel.hxx \
	lowlevel.hxx \
	sg_binobj.hxx \
	sg_file.hxx \
	sg_serial.hxx \
	sg_socket.hxx \
	sg_socket_udp.hxx \
	raw_socket.hxx 

libsgio_a_SOURCES = \
	iochannel.cxx \
	lowlevel.cxx \
	sg_binobj.cxx \
	sg_file.cxx \
	sg_serial.cxx \
	sg_socket.cxx \
	sg_socket_udp.cxx \
	raw_socket.cxx

INCLUDES = -I$(top_srcdir)

noinst_PROGRAMS = decode_binobj socktest lowtest tcp_server tcp_client

tcp_server_SOURCES = tcp_server.cxx

tcp_server_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \	
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	-lz \
	$(network_LIBS) \
	$(base_LIBS)

tcp_client_SOURCES = tcp_client.cxx

tcp_client_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	-lz \
	$(network_LIBS) \
	$(base_LIBS)

socktest_SOURCES = socktest.cxx

socktest_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
    -lz \
	$(network_LIBS) \
	$(base_LIBS) 

lowtest_SOURCES = lowtest.cxx

lowtest_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
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
