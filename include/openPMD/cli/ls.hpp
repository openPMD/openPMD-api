/* Copyright 2020-2021 Axel Huebl
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
#pragma once

#include "openPMD/Series.hpp"
#include "openPMD/helper/list_series.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace openPMD
{
namespace cli
{
    namespace ls
    {
        inline void print_help(std::string const program_name)
        {
            std::cout << "Usage: " << program_name << " openPMD-series\n";
            std::cout << "List information about an openPMD data series.\n\n";
            std::cout << "Options:\n";
            std::cout << "    -h, --help    display this help and exit\n";
            std::cout
                << "    -v, --version output version information and exit\n";
            std::cout << "\n";
            std::cout << "Examples:\n";
            std::cout << "    " << program_name
                      << " ./samples/git-sample/data%T.h5\n";
            std::cout << "    " << program_name
                      << " ./samples/git-sample/data%08T.h5\n";
            std::cout << "    " << program_name
                      << " ./samples/serial_write.json\n";
            std::cout << "    " << program_name
                      << " ./samples/serial_patch.bp\n";
        }

        inline void print_version(std::string const program_name)
        {
            std::cout << program_name << " (openPMD-api) " << getVersion()
                      << "\n";
            std::cout << "Copyright 2017-2021 openPMD contributors\n";
            std::cout << "Authors: Axel Huebl et al.\n";
            std::cout << "License: LGPLv3+\n";
            std::cout
                << "This is free software: you are free to change and "
                   "redistribute it.\n"
                   "There is NO WARRANTY, to the extent permitted by law.\n";
        }

        /** Run the openpmd-ls command line tool
         *
         * @param argv command line arguments 1-N
         * @return exit code (zero for success)
         */
        inline int run(std::vector<std::string> const &argv)
        {
            using namespace openPMD;
            auto const argc = argv.size();

            if (argc < 2)
            {
                print_help(argv[0]);
                return 0;
            }

            for (int c = 1; c < int(argc); c++)
            {
                if (std::string("--help") == argv[c] ||
                    std::string("-h") == argv[c])
                {
                    print_help(argv[0]);
                    return 0;
                }
                if (std::string("--version") == argv[c] ||
                    std::string("-v") == argv[c])
                {
                    print_version(argv[0]);
                    return 0;
                }
            }

            if (argc > 2)
            {
                std::cerr << "Too many arguments! See: " << argv[0]
                          << " --help\n";
                return 1;
            }

            try
            {
                auto s = Series(
                    argv[1],
                    Access::READ_ONLY,
                    R"({"defer_iteration_parsing": true})");

                helper::listSeries(s, true, std::cout);
            }
            catch (std::exception const &e)
            {
                std::cerr << "An error occurred while opening the specified "
                             "openPMD series!\n";
                std::cerr << e.what() << std::endl;
                return 2;
            }

            return 0;
        }
    } // namespace ls
} // namespace cli
} // namespace openPMD
