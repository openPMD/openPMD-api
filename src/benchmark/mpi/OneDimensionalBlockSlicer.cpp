/* Copyright 2018-2021 Franz Poeschel
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

#include "openPMD/benchmark/mpi/OneDimensionalBlockSlicer.hpp"

#include <algorithm>

namespace openPMD
{
OneDimensionalBlockSlicer::OneDimensionalBlockSlicer(Extent::value_type dim)
    : m_dim{dim}
{}

std::pair<Offset, Extent>
OneDimensionalBlockSlicer::sliceBlock(Extent &totalExtent, int size, int rank)
{
    Offset offs(totalExtent.size(), 0);

    if (rank >= size)
    {
        Extent extent(totalExtent.size(), 0);
        return std::make_pair(std::move(offs), std::move(extent));
    }

    auto dim = this->m_dim;

    // for more equal balancing, we want the start index
    // at the upper gaussian bracket of (N/n*rank)
    // where N the size of the dataset in dimension dim
    // and n the MPI size
    // for avoiding integer overflow, this is the same as:
    // (N div n)*rank + round((N%n)/n*rank)
    auto f = [&totalExtent, size, dim](int threadRank) {
        auto N = totalExtent[dim];
        auto res = (N / size) * threadRank;
        auto padDivident = (N % size) * threadRank;
        auto pad = padDivident / size;
        if (pad * size < padDivident)
        {
            pad += 1;
        }
        return res + pad;
    };

    offs[dim] = f(rank);
    Extent localExtent{totalExtent};
    if (rank >= size - 1)
    {
        localExtent[dim] -= offs[dim];
    }
    else
    {
        localExtent[dim] = f(rank + 1) - offs[dim];
    }
    return std::make_pair(std::move(offs), std::move(localExtent));
}
} // namespace openPMD
