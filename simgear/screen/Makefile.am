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

noinst_HEADERS = GLBitmaps.h

include_HEADERS = \
	$(IMAGE_SERVER_INCL) \
	screen-dump.hxx \
	colors.hxx \
	tr.h \
	extensions.hxx

libsgscreen_a_SOURCES = \
	$(IMAGE_SERVER_SRCS) \
	screen-dump.cxx \
	tr.cxx \
	extensions.cxx
	

INCLUDES = -I$(top_srcdir) -I$(top_builddir) 

