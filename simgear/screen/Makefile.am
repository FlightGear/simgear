includedir = @includedir@/screen

lib_LIBRARIES = libsgscreen.a

include_HEADERS = screen-dump.hxx tr.h

libsgscreen_a_SOURCES = \
	GLBitmaps.cxx GLBitmaps.h \
	screen-dump.cxx \
        tr.cxx \
	win32-printer.h

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir)
else
INCLUDES = -I$(top_srcdir)
endif
