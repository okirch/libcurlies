include Make.defs

BINDINGS = 
ifneq ($(WITH_PYTHON),none)
  BINDINGS += python
endif

SUBDIRS	= library $(BINDINGS) tests

all install clean distclean test::
	set -e; for dir in $(SUBDIRS); do make -C $$dir $@; done

ifneq ($(PKGCONFIG_DIR),)
install::
	mkdir -p $(DESTDIR)$(PKGCONFIG_DIR)
	cp libcurlies.pc $(DESTDIR)$(PKGCONFIG_DIR)
endif
