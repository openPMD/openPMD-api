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
#include <catch2/catch_session.hpp>

#if openPMD_HAVE_MPI
#include <mpi.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    Catch::Session session;
    // session.configData().runOrder = Catch::TestRunOrder::Declared;
    MPI_Bcast(
        &session.configData().rngSeed, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
    int result = session.applyCommandLine(argc, argv);
    if (result == 0)
    {
        result = session.run();
    }
    MPI_Finalize();
    return result;
}
#else
int main()
{
    throw std::runtime_error("Serial tests use Catch2's provided main.");
}
#endif
