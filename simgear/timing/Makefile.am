includedir = @includedir@/timing

lib_LIBRARIES = libsgtiming.a

include_HEADERS = \
	geocoord.h \
	sg_time.hxx \
	timezone.h

libsgtiming_a_SOURCES = \
	geocoord.cxx \
	lowleveltime.cxx lowleveltime.h \
	sg_time.cxx \
	timezone.cxx \
	# event.cxx event.hxx \
	# fg_timer.cxx fg_timer.hxx \
	# light.cxx light.hxx \
	# moonpos.cxx moonpos.hxx \
	# sunpos.cxx sunpos.hxx \
	# timestamp.hxx

INCLUDES += -I$(top_builddir)
