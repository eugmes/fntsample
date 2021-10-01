/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef UNICODE_UTILS_H
#define UNICODE_UTILS_H

#include <limits>
#include <optional>
#include <type_traits>

namespace unicode_utils {

constexpr char32_t last_codepoint = 0x10ffff;

template <typename U> constexpr std::optional<char32_t> to_char32_t(U n) noexcept
{
    static_assert(std::is_unsigned_v<U>);
    static_assert(std::numeric_limits<U>::max() >= last_codepoint);
    if (n <= last_codepoint) {
        return static_cast<char32_t>(n);
    }
    return {};
}

}

#endif
