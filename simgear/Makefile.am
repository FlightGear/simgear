if HAVE_THREADS
SGTHREAD_DIR = threads
else
SGTHREAD_DIR = 
endif

# METAR_DIRS =
METAR_DIRS = metar

EXTRA_DIST = simgear_config.h.vc5 version.h.in

include_HEADERS = \
	compiler.h constants.h sg_inlines.h sg_traits.hxx version.h

SUBDIRS = \
	$(compatibility_DIR) \
	xml \
	debug \
	misc \
	structure \
	bucket \
	ephemeris \
	io \
	magvar \
	math \
	$(METAR_DIRS) \
	props \
	route \
	scene \
	screen \
	serial \
	sound \
	$(SGTHREAD_DIR) \
	timing \
	xgl

DIST_SUBDIRS = $(SUBDIRS) compatibility metar threads
