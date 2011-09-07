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
	raw_socket.hxx  \
	sg_netBuffer.hxx \
	sg_netChannel.hxx \
	sg_netChat.hxx \
	HTTPClient.hxx \
	HTTPRequest.hxx

libsgio_a_SOURCES = \
	iochannel.cxx \
	lowlevel.cxx \
	sg_binobj.cxx \
	sg_file.cxx \
	sg_serial.cxx \
	sg_socket.cxx \
	sg_socket_udp.cxx \
	raw_socket.cxx \
	sg_netBuffer.cxx \
	sg_netChannel.cxx \
	sg_netChat.cxx \
	HTTPClient.cxx \
	HTTPRequest.cxx

INCLUDES = -I$(top_srcdir)

noinst_PROGRAMS = decode_binobj socktest lowtest tcp_server tcp_client

tcp_server_SOURCES = tcp_server.cxx

tcp_server_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
	$(top_builddir)/simgear/threads/libsgthreads.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	-lz \
	$(network_LIBS) \
	$(osg_LIBS) \
	$(base_LIBS)

tcp_client_SOURCES = tcp_client.cxx

tcp_client_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
	$(top_builddir)/simgear/threads/libsgthreads.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	-lz \
	$(network_LIBS) \
	$(osg_LIBS) \
	$(base_LIBS)

socktest_SOURCES = socktest.cxx

socktest_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
	$(top_builddir)/simgear/threads/libsgthreads.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
    -lz \
	$(network_LIBS) \
	$(osg_LIBS) \
	$(base_LIBS) 

lowtest_SOURCES = lowtest.cxx

lowtest_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
	$(top_builddir)/simgear/threads/libsgthreads.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(base_LIBS) -lz $(osg_LIBS)

decode_binobj_SOURCES = decode_binobj.cxx

decode_binobj_LDADD = \
	libsgio.a \
	$(top_builddir)/simgear/threads/libsgthreads.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/bucket/libsgbucket.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(base_LIBS) -lz $(osg_LIBS)
