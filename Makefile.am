EXTRA_DIST = \
	acinclude.m4 \
	autogen.sh \
	DoxygenMain.cxx
	README.MSVC \
	README.metakit \
	README.zlib \
	SimGear.dsp \
	SimGear.dsw

SUBDIRS	= src-libs simgear

dist-hook:
	(cd $(top_srcdir); $(HOME)/projects/FlightGear-0.7/admin/am2dsp.pl)

#
# Rule to build RPM distribution package
#
rpm: dist
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

