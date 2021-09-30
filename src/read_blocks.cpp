/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "unicode_blocks.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;

unicode_block *read_blocks(const char *file_name, int *n)
{
    int nalloc = 256;
    unicode_block *blocks = static_cast<unicode_block *>(calloc(nalloc, sizeof(unicode_block)));
    *n = 0;

    FILE *input_file = fopen(file_name, "r");
    if (!input_file) {
        perror("fopen");
        exit(7);
    }

    char *line = nullptr;
    size_t len = 0;
    ssize_t nread;

    while ((nread = getline(&line, &len, input_file)) != -1) {
        unsigned long block_start, block_end;
        char block_name[256];

        if (nread >= (ssize_t)sizeof(block_name))
            continue;

        int matched = sscanf(line, "%lx..%lx; %[^\r\n]", &block_start, &block_end, block_name);
        if (matched == 3) {
            unicode_block *b = blocks + *n;
            b->start = block_start;
            b->end = block_end;
            b->name = strdup(block_name);
            if (b->name == nullptr) {
                perror("strdup");
                exit(8);
            }

            *n += 1;
            if (*n >= nalloc) {
                int new_nalloc = nalloc + 256;
                unicode_block *new_blocks
                    = static_cast<unicode_block *>(realloc(blocks, new_nalloc * sizeof(unicode_block)));
                if (new_blocks == nullptr) {
                    perror("realloc");
                    exit(9);
                }
                memset(new_blocks + nalloc, 0,
                       (new_nalloc - nalloc) * sizeof(unicode_block));
                nalloc = new_nalloc;
                blocks = new_blocks;
            }
        }
    }
    free(line);

    if (*n == 0) {
        free(blocks);
        return nullptr;
    } else if (*n < nalloc) {
        blocks = static_cast<unicode_block *>(realloc(blocks, *n * sizeof(unicode_block)));
    }

    return blocks;
}
