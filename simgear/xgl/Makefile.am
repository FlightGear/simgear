includedir = @includedir@/xgl

lib_LIBRARIES = libsgxgl.a

include_HEADERS = xgl.h

libsgxgl_a_SOURCES = xgl.c xglUtils.c

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir)
else
INCLUDES = -I$(top_srcdir)
endif
