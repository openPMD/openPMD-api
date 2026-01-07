/* Copyright 2026 Franz Poeschel
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#include "openPMD/cli/convert-toml-json.hpp"

void print_help_message(char const *program_name)
{
    std::cout << "Merge multiple JSON/TOML files into one.\nUsage: "
              << std::string(program_name) << R"( [json_or_toml]+
'json_or_toml' can be a JSON or TOML dataset specified inline or a reference
to a file prepended by an '@'.
Inline datasets will be interpreted as JSON if they start with an '{', as TOML
otherwise. Datasets from a file will be interpreted as JSON or TOML depending
on the file ending '.json' or '.toml' respectively.

In order to support large numbers of files to be merged, the paths to those
files can also be specified line-by-line per stdin, replacing the limitations
of command line arguments.

If the JSON/TOML files are mixed, then the output type (JSON or TOML) will be
determined by the type of the first file.
)";
}

int main(int argc, char const **argv)
{
    using convert = convert_json_toml<from_format_to_format::ID>;
    convert::run_application(
        argc, argv, convert::UseStdinAs::ListOfJson, print_help_message);
}
