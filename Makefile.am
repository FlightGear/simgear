EXTRA_DIST = \
	acsite.m4 \
	acconfig.h \
	mksymlinks.sh \
	SimGear.dsp \
	SimGear.dsw \
	metakit-2.4.2-32.tar.gz \
	zlib-1.1.3.tar.gz

SUBDIRS	= simgear

#
# Rule to build RPM distribution package
#
rpm: dist
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

