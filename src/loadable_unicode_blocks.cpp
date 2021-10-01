/* Copyright © Євгеній Мещеряков <eugen@debian.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "loadable_unicode_blocks.h"
#include <regex>
#include <charconv>
#include <string_view>
#include "unicode_utils.h"

using namespace std;

loadable_unicode_blocks::loadable_unicode_blocks(istream &stream)
{
    static const regex re(R"(([[:xdigit:]]+)\.\.([[:xdigit:]]+); (.*))");
    string line;

    for (size_t line_no = 1; getline(stream, line); line_no++) {
        string_view v(line);

        // Remove comment if any
        if (auto c_pos = v.find('#'); c_pos != v.npos) {
            v.remove_suffix(v.size() - c_pos);
        }

        // Trim whitespace at the end
        auto ws_pos = v.end();
        for (ws_pos--; ws_pos >= v.begin(); ws_pos--) {
            auto c = *ws_pos;
            if (!(c == '\n' || c == '\r' || c == ' ' || c == '\t')) {
                break;
            }
        }
        v.remove_suffix(v.size() - (ws_pos + 1 - v.begin()));

        if (v.empty()) {
            continue;
        }

        cmatch m;
        if (!regex_match(v.data(), v.data() + v.size(), m, re)) {
            _error_line = line_no;
            blocks.clear();
            return;
        }

        auto parse_code = [](cmatch::const_reference sm) -> optional<char32_t> {
            unsigned long n;
            auto [ptr, ec] = from_chars(sm.first, sm.second, n, 16);
            if (ec == std::errc()) {
                return unicode_utils::to_char32_t(n);
            }
            return {};
        };

        auto start = parse_code(m[1]);
        auto end = parse_code(m[2]);
        if (!start || !end) {
            _error_line = line_no;
            blocks.clear();
            return;
        }

        unicode_range r {*start, *end};
        if (!r.is_valid()) {
            _error_line = line_no;
            blocks.clear();
            return;
        }

        blocks.push_back({r, string(m[3].first, m[3].second)});
    }
}
