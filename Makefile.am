if ENABLE_UNIX_SERIAL
SERIAL_DIRS = serial
else
SERIAL_DIRS =
endif

if HAVE_ZLIB
ZLIB_DIRS =
else
ZLIB_DIRS = zlib
endif

METAR_DIRS =
# METAR_DIRS = metar

EXTRA_DIST = mksymlinks.sh

dist-hook:
	tar cf - metar | (cd $(distdir); tar xvf -)

SUBDIRS = \
	include \
	bucket \
	debug \
	math \
	$(METAR_DIRS) \
	misc \
	screen \
	$(SERIAL_DIRS) \
	xgl \
	$(ZLIB_DIRS)

