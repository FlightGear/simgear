# METAR_DIRS =
METAR_DIRS = environment

EXTRA_DIST = simgear_config.h.vc5 simgear_config.h-msvc71 version.h.in

include_HEADERS = \
	compiler.h constants.h sg_inlines.h version.h

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
	scene \
	screen \
	serial \
	sound \
	threads \
	timing

DIST_SUBDIRS = $(SUBDIRS) compatibility
