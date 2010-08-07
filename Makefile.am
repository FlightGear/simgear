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

#
# Rule to build RPM distribution package
#
rpm: dist
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

