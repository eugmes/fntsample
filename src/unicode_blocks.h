/*
 * This file is in public domain
 * Author: Eugeniy Meshcheryakov <eugen@debian.org>
 */
#ifndef UNICODE_BLOCKS_H
#define UNICODE_BLOCKS_H

#include <stdlib.h>

struct unicode_block {
  unsigned long start;
  unsigned long end;
  const char *name;
};

extern const struct unicode_block static_unicode_blocks[];

struct unicode_block *read_blocks(const char *file_name, int *n);

#endif
