includedir = @includedir@/sound

EXTRA_DIST = jet.wav

lib_LIBRARIES = libsgsound.a

noinst_HEADERS =

include_HEADERS = \
	sample_openal.hxx \
	soundmgr_openal.hxx \
	xmlsound.hxx 

libsgsound_a_SOURCES = \
	sample_openal.cxx  \
	soundmgr_openal.cxx \
	xmlsound.cxx 

noinst_PROGRAMS = openal_test1 openal_test2

openal_test1_SOURCES = openal_test1.cxx
openal_test2_SOURCES = openal_test2.cxx

openal_test1_LDADD = \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(openal_LIBS)
openal_test2_LDADD = \
	$(top_builddir)/simgear/sound/libsgsound.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(top_builddir)/simgear/misc/libsgmisc.a \
	$(top_builddir)/simgear/structure/libsgstructure.a \
	$(openal_LIBS)

INCLUDES = -I$(top_srcdir)
