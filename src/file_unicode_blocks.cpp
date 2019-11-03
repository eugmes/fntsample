/* Copyright © 2019 Євгеній Мещеряков <eugen@debian.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "file_unicode_blocks.h"
#include <regex>

using namespace std;

file_unicode_blocks::file_unicode_blocks(istream &stream)
{
    const regex re("([0-9A-F]{4,6})\\.\\.([0-9A-F]{4,6}); ([^\\n\\r]*)", regex::ECMAScript);
    string line;

    while (getline(stream, line)) {
        smatch match;
        if (regex_match(line, match, re)) {
            uint32_t start = stoul(match.str(1), nullptr, 16);
            uint32_t end = stoul(match.str(2), nullptr, 16);

            m_blocks.push_back(block {start, end, match.str(3)});
        }
    }

    m_blocks.shrink_to_fit();
}

file_unicode_blocks::~file_unicode_blocks()
{
}

size_t file_unicode_blocks::size() const
{
    return m_blocks.size();
}

ssize_t file_unicode_blocks::find(uint32_t codepoint) const
{
    for (size_t i = 0; i < m_blocks.size(); i++) {
        if (m_blocks[i].contains(codepoint)) {
            return i;
        }
    }

    return -1;
}

unicode_blocks::block file_unicode_blocks::operator[](size_t index) const
{
    return m_blocks[index];
}
