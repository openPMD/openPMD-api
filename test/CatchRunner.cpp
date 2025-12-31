/* Copyright 2025 Axel Huebl
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
#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#if openPMD_HAVE_MPI
#include <mpi.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    Catch::Session session;
    int result = 0;
    {
        // Indicates a command line parsing
        result = session.applyCommandLine(argc, argv);
        // RT tests
        if (result == 0)
            result = session.run();
    }
    MPI_Finalize();
    return result;
}
#else
int main(int argc, char *argv[])
{
    Catch::Session session;
    int result = 0;
    {
        // Indicates a command line parsing
        result = session.applyCommandLine(argc, argv);
        // RT tests
        if (result == 0)
            result = session.run();
    }
    return result;
}
#endif
