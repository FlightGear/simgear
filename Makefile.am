EXTRA_DIST	= mksymlinks.sh acsite.m4 acconfig.h

SUBDIRS		= simgear

#
# Rule to build RPM distribution package
#
rpm: dist
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/packages/SOURCES
	rpm -ba @PACKAGE@.spec

dist-hook:
	tar cf - simgear/metar | (cd $(distdir); tar xvf -)

