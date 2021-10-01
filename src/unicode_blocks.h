/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef UNICODE_BLOCKS_H
#define UNICODE_BLOCKS_H

#include "unicode_range.h"
#include <optional>

// TODO: provide an iterator
class unicode_blocks {
public:
    struct block {
        unicode_range r;
        const char *name;

        constexpr bool contains(char32_t c) const noexcept { return r.contains(c); }
    };

    virtual size_t size() const noexcept = 0;
    virtual block operator[](size_t index) const = 0;

    std::optional<block> find_block(char32_t c) const noexcept;
};

#endif
