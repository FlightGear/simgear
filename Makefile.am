EXTRA_DIST = mksymlinks.sh

dist-hook:
	tar cf - src/metar | (cd $(distdir); tar xvf -)

SUBDIRS = src
