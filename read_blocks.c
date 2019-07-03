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
    if (!input_file) {
        perror("fopen");
        exit(7);
    }

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
            b->name = strdup(block_name);
            if (b->name == NULL) {
                perror("strdup");
                exit(8);
            }

            *n += 1;
            if (*n >= nalloc) {
                int new_nalloc = nalloc + 256;
                struct unicode_block *new_blocks = realloc(blocks,
                                                           new_nalloc * sizeof(struct unicode_block));
                if (new_blocks == NULL) {
                    perror("realloc");
                    exit(9);
                }
                memset(new_blocks + nalloc, 0, (new_nalloc - nalloc) * sizeof(struct unicode_block));
                nalloc = new_nalloc;
                blocks = new_blocks;
            }
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
