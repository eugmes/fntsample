/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "unicode_blocks.h"

using namespace std;

/*
 * Locate unicode block that contains given character code.
 * Returns this block or nullptr if not found.
 */
optional<unicode_blocks::block> unicode_blocks::find_block(char32_t c) const noexcept
{
    // TODO use binary search
    for (size_t i = 0; i < size(); i++) {
        const auto &block = (*this)[i];
        if (block.r.contains(c)) {
            return block;
        }
    }

    return {};
}
