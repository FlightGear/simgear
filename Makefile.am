EXTRA_DIST = \
	acsite.m4 \
	acconfig.h \
	mksymlinks.sh \
	README.metakit \
	README.zlib \
	SimGear.dsp \
	SimGear.dsw

SUBDIRS	= src-libs simgear

#
# Rule to build RPM distribution package
#
rpm: dist
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

