EXTRA_DIST = mksymlinks.sh acsite.m4 acconfig.h

dist-hook:
	tar cf - src/metar | (cd $(distdir); tar xvf -)

SUBDIRS = src
