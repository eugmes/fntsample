/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <fstream>
#include <fmt/ostream.h>
#include <iostream>

#include "loadable_unicode_blocks.h"

using namespace std;

static void write_header(ostream &stream)
{
    stream <<
        R"(#include "static_unicode_blocks.h"

constexpr static_unicode_blocks static_blocks {
)";
}

static void write_footer(ostream &stream)
{
    stream << R"(};

const unicode_blocks &get_static_blocks() { return static_blocks; }
)";
}

static void write_block(ostream &stream, const unicode_blocks::block &block)
{
    fmt::print(stream, "    unicode_blocks::block {{{{0x{:04x}, 0x{:04x}}}, \"{}\"}},\n",
               block.r.start, block.r.end, block.name);
}

static void write_blocks(ostream &stream, const unicode_blocks &blocks)
{
    write_header(stream);
    for (size_t i = 0; i < blocks.size(); i++) {
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

    const char *input_file_name = argv[1];
    const char *output_file_name = argv[2];

    ifstream input(input_file_name, ios::binary);
    if (!input) {
        cerr << "Failed to open input file\n";
        return 1;
    }

    loadable_unicode_blocks blocks(input);
    input.close();

    if (blocks.error_line()) {
        fmt::print(cerr, "Parse error at line {}\n", blocks.error_line());
        return 1;
    }

    ofstream output(output_file_name, ios::out | ios::trunc | ios::binary);
    if (!output) {
        cerr << "Failed to open output file\n";
        return 1;
    }

    write_blocks(output, blocks);
    if (!output) {
        cerr << "Failed to write the output\n";
        return 1;
    }

    return 0;
}
