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
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

static void write_block(ostream &stream, const unicode_blocks::block &block, bool last)
{
    stream << showbase << hex
           << "    {" << block.start << ", " << block.end << ", \"" << block.name << "\"}";

    if (!last) {
        stream << "," << endl;
    }
}

static void subst_var(string &template_text, const string &var, const string &substitution)
{
    for (;;) {
        auto pos = template_text.find(var);
        if (pos == string::npos) {
            break;
        }

        template_text.replace(pos, var.size(), substitution);
    }
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <template> Blocks.txt" << endl;
        return 1;
    }

    stringstream buffer;
    buffer << ifstream(argv[1]).rdbuf();
    string template_text = buffer.str();

    ifstream ifs(argv[2]);
    file_unicode_blocks blocks(ifs);

    subst_var(template_text, "@BLOCKS_SIZE@", to_string(blocks.size()));

    stringstream blocks_data;

    for (size_t i = 0; i < blocks.size(); i++) {
        write_block(blocks_data, blocks[i], i == blocks.size() - 1);
    }

    subst_var(template_text, "@BLOCKS_DATA@", blocks_data.str());

    cout << template_text;

    return 0;
}
