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
	colors.hxx \
	$(IMAGE_SERVER_INCL) \
	screen-dump.hxx \
	extensions.hxx \
	RenderTexture.h \
	shader.h \
	tr.h

libsgscreen_a_SOURCES = \
	GLBitmaps.cxx \
	$(IMAGE_SERVER_SRCS) \
	screen-dump.cxx \
	tr.cxx \
	extensions.cxx \
	RenderTexture.cpp \
	shader.cpp \
	win32-printer.h

if HAVE_GLUT
noinst_PROGRAMS = TestRenderTexture

TestRenderTexture_SOURCES = TestRenderTexture.cpp

TestRenderTexture_LDADD = \
	libsgscreen.a \
	$(top_builddir)/simgear/debug/libsgdebug.a \
	$(opengl_LIBS) \
	-ldl
endif

INCLUDES = -I$(top_srcdir) -I$(top_builddir) -DGLX_GLXEXT_PROTOTYPES
