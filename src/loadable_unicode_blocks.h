/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef LOADABLE_UNICODE_BLOCKS_H
#define LOADABLE_UNICODE_BLOCKS_H

#include "unicode_blocks.h"
#include <istream>
#include <vector>

class loadable_unicode_blocks final : public unicode_blocks {
public:
    explicit loadable_unicode_blocks(std::istream &stream);

    size_t size() const noexcept override { return blocks.size(); }
    block operator[](size_t i) const override
    {
        const auto &b = blocks[i];
        return {b.r, b.name.data()};
    }

    constexpr size_t error_line() const noexcept { return _error_line; }

private:
    struct entry {
        unicode_range r;
        std::string name;
    };

    std::vector<entry> blocks;
    size_t _error_line = 0;
};

#endif
