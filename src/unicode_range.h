/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef UNICODE_RANGE_H
#define UNICODE_RANGE_H

template <typename T> struct basic_range {
    T start, end;

    constexpr bool contains(T e) const noexcept { return start <= e && e <= end; }
    constexpr bool is_valid() const noexcept { return start <= end; }
};

using unicode_range = basic_range<char32_t>;

#endif
