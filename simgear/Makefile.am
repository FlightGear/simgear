if ENABLE_UNIX_SERIAL
SERIAL_DIRS = serial
else
SERIAL_DIRS =
endif

if HAVE_GDBM
GDBM_DIRS =
else
GDBM_DIRS = gdbm
endif

if HAVE_ZLIB
ZLIB_DIRS =
else
ZLIB_DIRS = zlib
endif

METAR_DIRS =
# METAR_DIRS = metar

EXTRA_DIST = version.h.in

include_HEADERS = compiler.h constants.h fg_traits.hxx fg_zlib.h version.h

SUBDIRS = \
	bucket \
	debug \
	$(GDBM_DIRS) \
	magvar \
	math \
	$(METAR_DIRS) \
	misc \
	screen \
	$(SERIAL_DIRS) \
	xgl \
	$(ZLIB_DIRS)

