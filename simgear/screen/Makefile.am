includedir = @includedir@/screen

lib_LIBRARIES = libsgscreen.a

EXTRA_DIST = jpgfactory.cxx jpgfactory.hxx

if ENABLE_JPEG_SERVER
IMAGE_SERVER_INCL = jpgfactory.hxx
IMAGE_SERVER_SRCS = jpgfactory.cxx
else
IMAGE_SERVER_INCL = 
IMAGE_SERVER_SRCS = 
endif

noinst_HEADERS = colours.h

include_HEADERS = \
	colors.hxx \
	texture.hxx \
	$(IMAGE_SERVER_INCL) \
	screen-dump.hxx \
	extensions.hxx \
	tr.h

libsgscreen_a_SOURCES = \
	texture.cxx \
	GLBitmaps.cxx GLBitmaps.h \
	$(IMAGE_SERVER_SRCS) \
	screen-dump.cxx \
        tr.cxx \
	win32-printer.h

INCLUDES = -I$(top_srcdir)
