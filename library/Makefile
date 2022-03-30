include ../Make.defs

.PHONY: all install clean

CFLAGS	= -D_GNU_SOURCE $(CCOPT)

LIBOBJS = curlies.o \
	  parser.o
STATIC_LIBOBJS = $(addprefix static/,$(LIBOBJS))

SHLIB	= libcurlies.$(SHLIB_EXTENSION)
VERSIONED_SHLIB = \
	  $(SHLIB).$(CURLIES_MAJOR)
STATICLIB = libcurlies.a

BUILD	= $(SHLIB)
INSTALL	= install-shlib install-headers

ifeq ($(BUILD_STATIC),true)
  BUILD += $(STATICLIB)
  INSTALL += install-staticlib
endif

all: $(BUILD)

test: ;

install: $(INSTALL)

$(SHLIB): $(LIBOBJS) Makefile
	$(CC) $(CFLAGS) -o $@ --shared -Wl,-soname,$(VERSIONED_SHLIB) $(LIBOBJS)


$(STATICLIB): $(STATIC_LIBOBJS)
	$(AR) crv $@ $(STATIC_LIBOBJS)

install-headers: curlies.h
	install -d $(DESTDIR)$(INCDIR)
	install -m444 curlies.h $(DESTDIR)$(INCDIR)

install-shlib: $(SHLIB)
	install -d $(DESTDIR)$(LIBDIR)
	install -m555 $(SHLIB) $(DESTDIR)$(LIBDIR)/$(VERSIONED_SHLIB)
	ln -sf $(VERSIONED_SHLIB) $(DESTDIR)$(LIBDIR)/$(SHLIB)

install-staticlib: $(STATICLIB)
	install -d $(DESTDIR)$(LIBDIR)
	install -m644 $(STATICLIB) $(DESTDIR)$(LIBDIR)/$(STATICLIB)

%.o: %.c
	$(CC) $(CFLAGS) -c -fPIC -o $@ $<

static/%.o: %.c
	@mkdir -p static
	$(CC) $(CFLAGS) -c -o $@ $<

config.o: config.c curlies.h

clean:
	rm -f *.o *.so *.a
	rm -rf static
