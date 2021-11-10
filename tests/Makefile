include ../Make.defs

.PHONY: all install clean

CFLAGS	= -D_GNU_SOURCE $(CCOPT) -I../library

all: curlies-test

curlies-test: curlies-test.c
	$(CC) $(CFLAGS) -o $@ curlies-test.c -L../library -lcurlies

# Test libcurlies.
# We read each test file, write it out and compare to an expected result.
# Then we re-read the file we just wrote, write it out once again, and compare it again.
#
# This ensures that we parse all sample files as expected, and that we're also
# able to parse the files we're writing.
test: curlies-test
	mkdir -p output
	@for conf in `ls input`; do \
		echo "Test read/write of $$conf"; \
		LD_PRELOAD=../library/libcurlies.so ./curlies-test input/$$conf >output/$$conf; \
		diff -wu expected/$$conf output/$$conf || exit 1; \
		LD_PRELOAD=../library/libcurlies.so ./curlies-test output/$$conf | diff -wu - expected/$$conf || exit 1; \
		echo "  Okay, produced expected result"; \
	done

install: ;

clean:
	rm -f *.o curlies-test
	rm -rf output
