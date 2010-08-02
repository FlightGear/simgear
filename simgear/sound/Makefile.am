includedir = @includedir@/sound

EXTRA_DIST = jet.wav

lib_LIBRARIES = libsgsound.a

noinst_HEADERS =

include_HEADERS = \
	soundmgr_openal.hxx \
	sample_group.hxx \
	sample_openal.hxx \
	sample_queue.hxx \
	xmlsound.hxx

libsgsound_a_SOURCES = \
	soundmgr_openal.cxx \
	sample_group.cxx \
	sample_openal.cxx  \
	sample_queue.cxx \
	xmlsound.cxx

check_PROGRAMS = openal_test1 openal_test2 openal_test3 openal_test4

openal_test1_SOURCES = openal_test1.cxx
openal_test2_SOURCES = openal_test2.cxx
openal_test3_SOURCES = openal_test3.cxx
openal_test4_SOURCES = openal_test4.cxx

openal_test1_LDADD = \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(openal_LIBS)

openal_test2_LDADD = \
	libsgsound.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
	$(top_builddir)/simgear/timing/libsgtiming.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(top_builddir)/simgear/math/libsgmath.a \
	$(openal_LIBS)

openal_test3_LDADD = \
	libsgsound.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
	$(top_builddir)/simgear/timing/libsgtiming.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(top_builddir)/simgear/math/libsgmath.a \
	$(openal_LIBS) -lstdc++

openal_test4_LDADD = \
	libsgsound.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
        $(top_builddir)/simgear/timing/libsgtiming.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(top_builddir)/simgear/math/libsgmath.a \
	$(openal_LIBS) -lstdc++

INCLUDES = -I$(top_srcdir) -DSRC_DIR=\"$(top_srcdir)/simgear/sound\"
