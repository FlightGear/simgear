if HAVE_ZLIB
ZLIB_INCL =
else
XLIB_INCL = -I$(top_builddir)/src/zlib
endif

lib_LIBRARIES = libsgxml.a

include_HEADERS = \
	easyxml.hxx

libsgxml_a_SOURCES = \
	hashtable.c \
	xmlparse.c \
	xmlrole.c \
	xmltok.c \
	easyxml.cxx

INCLUDES += -I$(top_builddir) $(ZLIB_INCL)
