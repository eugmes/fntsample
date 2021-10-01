/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef STATIC_UNICODE_BLOCKS_H
#define STATIC_UNICODE_BLOCKS_H

#include "unicode_blocks.h"
#include <array>

template <std::size_t N> class static_unicode_blocks final : public unicode_blocks {
public:
    template <typename... T>
    constexpr explicit static_unicode_blocks(T &&... blocks) noexcept
        : blocks {std::forward<T>(blocks)...}
    {
    }

    size_t size() const noexcept override { return blocks.size(); }
    block operator[](size_t i) const override { return blocks[i]; }

private:
    std::array<block, N> blocks;
};

template <typename... T> static_unicode_blocks(T &&...)->static_unicode_blocks<sizeof...(T)>;

const unicode_blocks &get_static_blocks();

#endif
