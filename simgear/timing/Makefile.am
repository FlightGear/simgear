includedir = @includedir@/timing

lib_LIBRARIES = libsgtiming.a

include_HEADERS = \
	fg_time.hxx \
	geocoord.h \
	timezone.h

libsgtiming_a_SOURCES = \
	fg_time.cxx \
	geocoord.cxx \
	lowleveltime.cxx lowleveltime.h \
	timezone.cxx \
	# event.cxx event.hxx \
	# fg_timer.cxx fg_timer.hxx \
	# light.cxx light.hxx \
	# moonpos.cxx moonpos.hxx \
	# sunpos.cxx sunpos.hxx \
	# timestamp.hxx

INCLUDES += -I$(top_builddir)
