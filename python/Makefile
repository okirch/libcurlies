include ../Make.defs

CFLAGS	= -D_GNU_SOURCE -fPIC $(CCOPT) \
	  $(TWOPENCE_CFLAGS) \
	  $(PYTHON_CFLAGS) \
	  -I../library

OBJS	= extension.o \
	  config.o

all:	curly.so

test: ;

curly.so: $(OBJS)
	$(CC) --shared -o $@ $(OBJS) -L../library -lcurlies

distclean clean::
	rm -f *.o *.so

distclean::
	;

install: curly.so
	install -d $(DESTDIR)$(PYTHON_INSTDIR)
	install -m555 curly.so $(DESTDIR)$(PYTHON_INSTDIR)
