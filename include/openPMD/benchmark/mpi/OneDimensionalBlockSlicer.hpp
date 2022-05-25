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

#pragma once

#include "openPMD/Dataset.hpp"
#include "openPMD/benchmark/mpi/BlockSlicer.hpp"

namespace openPMD
{
class OneDimensionalBlockSlicer : public BlockSlicer
{
public:
    Extent::value_type m_dim;

    explicit OneDimensionalBlockSlicer(Extent::value_type dim = 0);

    std::pair<Offset, Extent>
    sliceBlock(Extent &totalExtent, int size, int rank) override;
};
} // namespace openPMD
