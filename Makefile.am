EXTRA_DIST	= mksymlinks.sh acsite.m4 acconfig.h

SUBDIRS		= simgear

#
# Additional all, clean, and distclean rules for additional packages.
# We can't just include the additional package dirs in SUBDIRS (which 
# would be nice because there are then problems with make dist
#

all: 
	@list='@subdirs@'; \
	target=`echo $@`; \
	for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done

install: 
	@list='@subdirs@'; \
	target=`echo $@`; \
	for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done

clean: 
	@list='@subdirs@'; \
	target=`echo $@`; \
	for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done

distclean: 
	@list='@subdirs@'; \
	target=`echo $@`; \
	for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done

#
# distclean the associated packages and include them in the dist
#

dist-hook:
	tar cf - simgear/metar | (cd $(distdir); tar xvf -)
	@list='@subdirs@'; \
	target=distclean; \
	for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) echo "distclean error ignored...";; *k*) fail=yes;; *) echo "distclean error ignored...";; esac; \
	done
	tar cf - simgear/metakit | (cd $(distdir); tar xvf -)
	

#
# Rule to build RPM distribution package
#
rpm: dist
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/packages/SOURCES
	rpm -ba @PACKAGE@.spec

