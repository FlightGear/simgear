includedir = @includedir@/debug

EXTRA_DIST = logtest.cxx

lib_LIBRARIES = libsgdebug.a

include_HEADERS = debug_types.h logstream.hxx

libsgdebug_a_SOURCES = logstream.cxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir)
else
INCLUDES = -I$(top_srcdir)
endif
