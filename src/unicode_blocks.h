/*
 * Author: Ievgenii Meshcheriakov <eugen@debian.org>
 * SPDX-License-Identifier: CC-PDDC
 */
#ifndef UNICODE_BLOCKS_H
#define UNICODE_BLOCKS_H

#include <stdlib.h>

struct unicode_block {
    unsigned long start;
    unsigned long end;
    const char *name;
};

struct unicode_block *read_blocks(const char *file_name, int *n);

#endif
