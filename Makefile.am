EXTRA_DIST = \
	acinclude.m4 \
	autogen.sh \
	DoxygenMain.cxx \
	project/VC8 \
	README.MSVC \
	README.zlib \
        projects \
	SimGear.dsp \
	SimGear.dsw

SUBDIRS	= simgear

dist-hook:
	(cd $(top_srcdir); $(HOME)/Projects/FlightGear/admin/am2dsp.pl)
	rm -rf `find $(distdir)/projects -name CVS`

#
# Rule to build RPM distribution package
#
rpm: dist
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

