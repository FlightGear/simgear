includedir = @includedir@/xml

lib_LIBRARIES = libsgxml.a

include_HEADERS = \
	easyxml.hxx

noinst_HEADERS = \
	xmltok_impl.c xmltok_ns.c

libsgxml_a_SOURCES = \
        asciitab.h \
	easyxml.cxx \
	hashtable.h hashtable.c \
	iasciitab.h latin1tab.h nametab.h utf8tab.h xmldef.h \
	xmlparse.h xmlparse.c \
	xmlrole.h xmlrole.c \
	xmltok.h xmltok.c \
	xmltok_impl.h

INCLUDES = -I$(top_srcdir)
