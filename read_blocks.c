#include "unicode_blocks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct unicode_block *read_blocks(const char *file_name, int *n)
{
	int nalloc = 256;
	struct unicode_block *blocks = calloc(nalloc, sizeof(struct unicode_block));
	*n = 0;

	FILE *input_file = fopen(file_name, "r");
	if (!input_file)
		abort(); // TODO

	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

	while ((nread = getline(&line, &len, input_file)) != -1) {
		unsigned long block_start, block_end;
		char block_name[256];

		if (nread >= (ssize_t)sizeof(block_name))
			continue;

		int matched = sscanf(line, "%lx..%lx; %[^\r\n]", &block_start, &block_end, block_name);
		if (matched == 3) {
			struct unicode_block *b = blocks + *n;
			b->start = block_start;
			b->end = block_end;
			b->name = strdup(block_name); // TODO check for zero
			*n += 1;
			if (*n >= nalloc)
				break; // TODO handle this better
		}
	}
	free(line);

	if (*n == 0) {
		free(blocks);
		return NULL;
	} else if (*n < nalloc) {
		blocks = realloc(blocks, *n * sizeof(struct unicode_block));
	}

	return blocks;
}
