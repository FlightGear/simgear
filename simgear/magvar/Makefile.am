includedir = @includedir@/magvar

lib_LIBRARIES = libsgmagvar.a

include_HEADERS = magvar.hxx

libsgmagvar_a_SOURCES = \
	coremag.hxx coremag.cxx \
	magvar.cxx

noinst_PROGRAMS = testmagvar

testmagvar_SOURCES = testmagvar.cxx

testmagvar_LDADD = $(top_builddir)/simgear/magvar/libsgmagvar.a

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir)
else
INCLUDES = -I$(top_srcdir)
endif
