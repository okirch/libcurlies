
#include <stdio.h>
#include "curlies.h"

int
main(int argc, char **argv)
{
	const char *filename;
	curly_node_t *cfg;

	if (argc <= 1) {
		fprintf(stderr, "Missing file name argument\n");
		return 1;
	}

	filename = argv[1];
	cfg = curly_node_read(filename);
	if (cfg == NULL) {
		fprintf(stderr, "Unable to parse file \"%s\"\n", filename);
		return 1;
	}

	curly_node_write_fp(cfg, stdout);

	curly_node_free(cfg);

	return 0;
}
