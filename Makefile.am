EXTRA_DIST = \
	acinclude.m4 \
	autogen.sh \
	DoxygenMain.cxx \
	README.zlib \
	README.plib \
	README.OpenAL \
	README.OSG \
	projects

SUBDIRS	= simgear

dist-hook:
	(cd $(top_srcdir); $(HOME)/Projects/FlightGear/admin/am2dsp.pl)
	rm -rf `find $(distdir)/projects -name CVS`

#
# Rule to build RPM distribution package
#
rpm: dist
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

