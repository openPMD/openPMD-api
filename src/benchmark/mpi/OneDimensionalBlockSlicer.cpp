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

std::pair<size_t, size_t> OneDimensionalBlockSlicer::n_th_block_inside(
    size_t length, size_t rank, size_t size)
{
    if (rank >= size)
    {
        return {length, 0};
    }

    // for more equal balancing, we want the start index
    // at the upper gaussian bracket of (N/n*rank)
    // where N the size of the dataset in dimension dim
    // and n the MPI size
    // for avoiding integer overflow, this is the same as:
    // (N div n)*rank + round((N%n)/n*rank)
    auto f = [length, size](size_t rank_lambda) {
        auto res = (length / size) * rank_lambda;
        auto padDivident = (length % size) * rank_lambda;
        auto pad = padDivident / size;
        if (pad * size < padDivident)
        {
            pad += 1;
        }
        return res + pad;
    };

    size_t offset = f(rank);
    size_t extent = [&]() {
        if (rank >= size - 1)
        {
            return length - offset;
        }
        else
        {
            return f(rank + 1) - offset;
        }
    }();
    return {offset, extent};
}

std::pair<Offset, Extent>
OneDimensionalBlockSlicer::sliceBlock(Extent &totalExtent, int size, int rank)
{
    Offset localOffset(totalExtent.size(), 0);
    Extent localExtent{totalExtent};

    auto [offset_dim, extent_dim] =
        n_th_block_inside(totalExtent.at(this->m_dim), rank, size);
    localOffset[m_dim] = offset_dim;
    localExtent[m_dim] = extent_dim;
    return std::make_pair(std::move(localOffset), std::move(localExtent));
}

std::unique_ptr<BlockSlicer> OneDimensionalBlockSlicer::clone() const
{
    return std::unique_ptr<BlockSlicer>(new OneDimensionalBlockSlicer(m_dim));
}
} // namespace openPMD
