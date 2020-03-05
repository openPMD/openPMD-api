/* Copyright 2019-2020 Axel Huebl
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

#include "openPMD/openPMD.hpp"

#include <iostream>
#include <string>
#include <exception>


inline void
print_help( std::string const program_name )
{
    std::cout << "Usage: " << program_name << " openPMD-series\n";
    std::cout << "List information about an openPMD data series.\n\n";
    std::cout << "Options:\n    -h, --help    display this help and exit\n\n";
    std::cout << "Examples:\n";
    std::cout << "    " << program_name << " ./samples/git-sample/data%T.h5\n";
    std::cout << "    " << program_name << " ./samples/git-sample/data%08T.h5\n";
    std::cout << "    " << program_name << " ./samples/serial_write.json\n";
    std::cout << "    " << program_name << " ./samples/serial_patch.bp\n";
}

int main(
    int argc,
    char * argv[]
)
{
    using namespace openPMD;

    if( argc < 2 )
    {
        print_help( argv[0] );
        return 0;
    }
    if( std::string("--help") == argv[1] || std::string("-h") == argv[1] )
    {
        print_help( argv[0] );
        return 0;
    }
    if( argc > 2 )
    {
        std::cerr << "Too many arguments! See: " << argv[0] << " --help\n";
        return 1;
    }

    try
    {
        auto s = Series(
                argv[1],
                AccessType::READ_ONLY
        );

        helper::listSeries(s, true, std::cout);
    }
    catch( std::exception const  & e )
    {
        std::cerr << "An error occurred while opening the specified openPMD series!\n";
        std::cerr << e.what() << std::endl;
        return 2;
    }

    return 0;
}
