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

#include <array>
#include <cstdint>
#include <map>
#include <vector>

namespace openPMD
{

using UnitDimensionExponent = double;

/** Physical dimension of a record
 *
 * Dimensional base quantities of the international system of quantities
 */
enum class UnitDimension : uint8_t
{
    L = 0, //!< length
    M, //!< mass
    T, //!< time
    I, //!< electric current
    theta, //!< thermodynamic temperature
    N, //!< amount of substance
    J //!< luminous intensity
};

namespace unit_representations
{
    using AsMap = std::map<UnitDimension, UnitDimensionExponent>;
    using AsArray = std::array<UnitDimensionExponent, 7>;

    using AsMaps = std::vector<AsMap>;
    using AsArrays = std::vector<AsArray>;

    auto asArray(AsMap const &) -> AsArray;
    auto asMap(AsArray const &, bool skip_zeros = true) -> AsMap;

    auto asArrays(AsMaps const &) -> AsArrays;
    auto asMaps(AsArrays const &, bool skip_zeros = true) -> AsMaps;

    namespace auxiliary
    {
        void fromMapOfUnitDimension(
            double *cursor, std::map<UnitDimension, double> const &udim);
    } // namespace auxiliary
} // namespace unit_representations
} // namespace openPMD
