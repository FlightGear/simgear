if ENABLE_UNIX_SERIAL
SERIAL_DIRS = serial
else
SERIAL_DIRS =
endif

# if HAVE_GDBM
# GDBM_DIRS =
# else
# GDBM_DIRS = gdbm
# endif

if HAVE_ZLIB
ZLIB_DIRS =
else
ZLIB_DIRS = zlib
endif

if HAVE_THREADS
SGTHREAD_DIR = threads
else
SGTHREAD_DIR = 
endif

# METAR_DIRS =
METAR_DIRS = metar

EXTRA_DIST = version.h.in

include_HEADERS = \
	compiler.h constants.h sg_inlines.h sg_traits.hxx sg_zlib.h version.h

SUBDIRS = \
	bucket \
	debug \
	ephemeris \
	io \
	magvar \
	math \
	$(METAR_DIRS) \
	xml \
	misc \
	route \
	screen \
	$(SERIAL_DIRS) \
	sky \
	$(SGTHREAD_DIR)
	timing \
	xgl \
	$(ZLIB_DIRS)
