/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "unicode_blocks.h"
#include <fstream>
#include <fmt/ostream.h>
#include <iostream>

using namespace std;

static void write_header(ostream &stream)
{
    stream <<
R"(#include "static_unicode_blocks.h"

const unicode_block static_unicode_blocks[] = {
)";
}

static void write_footer(ostream &stream)
{
    stream <<
R"(    {0, 0, nullptr},
};
)";
}

static void write_block(ostream &stream, const unicode_block &block)
{
    fmt::print(stream, "    {{0x{:04x}, 0x{:04x}, \"{}\"}},\n", block.start, block.end, block.name);
}

static void write_blocks(ostream &stream, const unicode_block *blocks, int n)
{
    write_header(stream);
    for (int i = 0; i < n; i++) {
        write_block(stream, blocks[i]);
    }
    write_footer(stream);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fmt::print(cerr, "Usage: {} Blocks.txt output.c\n", argv[0]);
        return 1;
    }

    int n;
    unicode_block *blocks = read_blocks(argv[1], &n);

    if (!blocks) {
        cerr << "Failed to read unicode blocks file.\n";
        return 2;
    }

    try {
        ofstream f(argv[2], ios::out | ios::trunc);
        f.exceptions(ios::badbit | ios::failbit);
        write_blocks(f, blocks, n);
    } catch (const ios_base::failure &e) {
        // FIXME: How to get useful error messages?
        fmt::print(cerr, "Failed to save code file: {}\n", e.what());
        return 1;
    }

    free(blocks);

    return 0;
}
