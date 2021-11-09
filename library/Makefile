include ../Make.defs

.PHONY: all install clean

CFLAGS	= -D_GNU_SOURCE -fPIC $(CCOPT)

LIB_OBJS= curlies.o \
	  parser.o

SHLIB	= libcurlies.$(SHLIB_EXTENSION)
VERSIONED_SHLIB = \
	  $(SHLIB).$(CURLIES_MAJOR)

all: $(SHLIB)

test: ;

$(SHLIB): $(LIB_OBJS) Makefile
	$(CC) $(CFLAGS) -o $@ --shared -Wl,-soname,$(VERSIONED_SHLIB) $(LIB_OBJS)

install: $(SHLIB) curlies.h
	install -d $(DESTDIR)$(INCDIR)
	install -m444 curlies.h $(DESTDIR)$(INCDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install -m555 $(SHLIB) $(DESTDIR)$(LIBDIR)/$(VERSIONED_SHLIB)
	ln -sf $(VERSIONED_SHLIB) $(DESTDIR)$(LIBDIR)/$(SHLIB)


config.o: config.c curlies.h

clean:
	rm -f *.o *.so
