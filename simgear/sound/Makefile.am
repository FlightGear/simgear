includedir = @includedir@/sound

EXTRA_DIST = jet.wav

lib_LIBRARIES = libsgsound.a

noinst_HEADERS =

include_HEADERS = \
	sample_group.hxx \
	sample_openal.hxx \
	soundmgr_openal.hxx \
	xmlsound.hxx 

libsgsound_a_SOURCES = \
	sample_group.cxx \
	sample_openal.cxx  \
	soundmgr_openal.cxx \
	xmlsound.cxx 

check_PROGRAMS = openal_test1 openal_test2

openal_test1_SOURCES = openal_test1.cxx
openal_test2_SOURCES = openal_test2.cxx

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

INCLUDES = -I$(top_srcdir) -DSRC_DIR=\"$(top_srcdir)/simgear/sound\"
