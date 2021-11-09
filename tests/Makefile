include ../Make.defs

.PHONY: all install clean

CFLAGS	= -D_GNU_SOURCE $(CCOPT) -I../library

all: curlies-test

curlies-test: curlies-test.c
	$(CC) $(CFLAGS) -o $@ curlies-test.c -L../library -lcurlies

test: curlies-test
	mkdir -p output
	@for conf in `ls input`; do \
		echo "Test read/write of $$conf"; \
		LD_PRELOAD=../library/libcurlies.so ./curlies-test input/$$conf >output/$$conf; \
		diff -u expected/$$conf output/$$conf || exit 1; \
		echo "  Okay, produced expected result"; \
	done

install: ;

clean:
	rm -f *.o curlies-test
	rm -rf output
