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
# Additional all, clean, and distclean rules for additional packages.
# We can't just include the additional package dirs in SUBDIRS (which 
# would be nice because there are then problems with make dist
#

old_all: 
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

old_install: 
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

old_clean: 
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

old_distclean: 
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

old_dist-hook:
	(cd $(top_srcdir); \
		$(HOME)/projects/FlightGear-0.7/admin/am2dsp.pl --lib)
	pwd
	echo "copying simgear/metar"
	echo "other stuff"
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
	(cd $(top_srcdir); tar cf - simgear/metakit) | \
                (cd $(distdir); tar xvf -)


#
# Rule to build RPM distribution package
#
rpm: dist
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

