if ENABLE_UNIX_SERIAL
SERIAL_DIRS = serial
else
SERIAL_DIRS =
endif

if HAVE_THREADS
SGTHREAD_DIR = threads
else
SGTHREAD_DIR = 
endif

# METAR_DIRS =
METAR_DIRS = metar

EXTRA_DIST = simgear_config.h.vc5 version.h.in

include_HEADERS = \
	compiler.h constants.h sg_inlines.h sg_traits.hxx sg_zlib.h version.h

SUBDIRS = \
	xml \
	interpreter \
	debug \
	misc \
	bucket \
	ephemeris \
	io \
	magvar \
	math \
	$(METAR_DIRS) \
	route \
	screen \
	$(SERIAL_DIRS) \
	sky \
	$(SGTHREAD_DIR) \
	timing \
	xgl

