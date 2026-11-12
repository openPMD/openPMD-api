/* Copyright 2025 Franz Poeschel
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

/*
 * Compile-time check that the public headers do not pull in <mpi.h>, even
 * when the library was built with MPI support. Non-MPI consumers can include
 * the regular public headers directly; the MPI communicators are passed
 * type-erased.
 */

#include "openPMD/openPMD.hpp"

#ifdef MPI_VERSION
#error "A public openPMD header pulled in <mpi.h>"
#endif

int main()
{
    return 0;
}
