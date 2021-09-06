/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "unicode_blocks.h"
#include <stdio.h>

static void write_header(FILE *f)
{
    fprintf(f,
            "#include \"static_unicode_blocks.h\"\n"
            "\n"
            "const struct unicode_block static_unicode_blocks[] = {\n");
}

static void write_footer(FILE *f)
{
    fprintf(f,
            "    {0, 0, NULL},\n"
            "};\n");
}

static void write_block(FILE *f, const struct unicode_block *block)
{
    fprintf(f, "    {0x%04lx, 0x%04lx, \"%s\"},\n", block->start, block->end, block->name);
}

static void write_blocks(FILE *f, const struct unicode_block *blocks, int n)
{
    write_header(f);
    for (int i = 0; i < n; i++) {
        write_block(f, blocks + i);
    }
    write_footer(f);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s Blocks.txt output.c\n", argv[0]);
        return 1;
    }

    int n;
    struct unicode_block *blocks = read_blocks(argv[1], &n);

    if (!blocks) {
        fprintf(stderr, "Failed to read unicode blocks file.\n");
        return 2;
    }

    FILE *f = fopen(argv[2], "wb");
    if (!f) {
        perror("fopen");
        return 3;
    }

    write_blocks(f, blocks, n);
    free(blocks);
    fclose(f);

    return 0;
}
