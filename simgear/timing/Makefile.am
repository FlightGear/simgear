includedir = @includedir@/timing

lib_LIBRARIES = libsgtiming.a

include_HEADERS = \
	geocoord.h \
	lowleveltime.h \
	sg_time.hxx \
	timestamp.hxx \
	timezone.h

libsgtiming_a_SOURCES = \
	geocoord.cxx \
	lowleveltime.cxx \
	sg_time.cxx \
	timestamp.cxx \
	timezone.cxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir)
else
INCLUDES = -I$(top_srcdir)
endif
