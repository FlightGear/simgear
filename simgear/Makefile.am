EXTRA_DIST = simgear_config.h.vc5 simgear_config.h-msvc71 version.h.in

include_HEADERS = \
	compiler.h constants.h sg_inlines.h version.h

if WANT_HEADLESS
METAR_DIRS =
SG_EXTRA_DIRS =
else
SG_EXTRA_DIRS = scene sound screen
METAR_DIRS = environment
endif

if HAVE_THREADS
SGTHREAD_DIR = threads
else
SGTHREAD_DIR = 
endif

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
	nasal \
	props \
	route \
	$(SG_EXTRA_DIRS) \
	$(SGTHREAD_DIR) \
	serial \
	timing \

DIST_SUBDIRS = $(SUBDIRS) compatibility
