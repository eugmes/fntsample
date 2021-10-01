/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef UNICODE_RANGE_SET_H
#define UNICODE_RANGE_SET_H

#include "unicode_range.h"
#include <list>

template <typename T> class basic_range_set {
public:
    using range = basic_range<T>;

    bool contains(T e) const noexcept
    {
        bool in = !specs.empty() || !specs.front().include;
        for (auto spec : specs) {
            if (spec.r.contains(e)) {
                in = spec.include;
            }
        }
        return in;
    }

    void add(const range &r, bool include) noexcept { specs.push_back(range_spec {r, include}); }

private:
    struct range_spec {
        range r;
        bool include;
    };

    std::list<range_spec> specs;
};

using unicode_range_set = basic_range_set<char32_t>;

#endif
