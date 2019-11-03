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
#ifndef STATIC_UNICODE_BLOCKS_H
#define STATIC_UNICODE_BLOCKS_H

#include "unicode_blocks.h"

class static_unicode_blocks: public unicode_blocks {
public:
    ~static_unicode_blocks() override;

    size_t size() const override;
    ssize_t find(uint32_t codepoint) const override;
    block operator[](size_t index) const override;
};

#endif
