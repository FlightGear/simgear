if ENABLE_HLA
HLA_DIR = hla
else
HLA_DIR =
endif

EXTRA_DIST = hla simgear_config.h-msvc90 version.h.in

include_HEADERS = \
	compiler.h constants.h sg_inlines.h version.h

if WANT_HEADLESS
SG_EXTRA_DIRS =
METAR_DIRS = 
else
SG_EXTRA_DIRS = scene sound screen
METAR_DIRS = environment
endif

if HAVE_THREADS
SGTHREAD_DIR = threads
else
SGTHREAD_DIR = 
endif

SUBDIRS_ALWAYS = \
	xml \
	debug \
	misc \
	structure \
	bucket \
	ephemeris \
	$(HLA_DIR) \
	$(SGTHREAD_DIR) \
	io \
	magvar \
	math \
	nasal \
	props \
	route \
	serial \
	timing

SUBDIRS = $(SUBDIRS_ALWAYS) \
	$(compatibility_DIR) \
	$(METAR_DIRS) \
	$(SG_EXTRA_DIRS)

DIST_SUBDIRS = $(SUBDIRS_ALWAYS) compatibility scene sound screen environment threads

