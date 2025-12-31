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
#include "openPMD/UnitDimension.hpp"
#include <algorithm>
#include <iterator>

namespace openPMD::unit_representations
{
auto asArray(AsMap const &udim) -> AsArray
{
    AsArray res{};
    auxiliary::fromMapOfUnitDimension(res.data(), udim);
    return res;
}
auto asMap(AsArray const &array, bool skip_zeros) -> AsMap
{
    AsMap udim;
    for (size_t i = 0; i < array.size(); ++i)
    {
        if (!skip_zeros || array[i] != 0)
        {
            udim[static_cast<UnitDimension>(i)] = array[i];
        }
    }
    return udim;
}

auto asArrays(AsMaps const &vec) -> AsArrays
{
    AsArrays res;
    res.reserve(vec.size());
    std::transform(
        vec.begin(), vec.end(), std::back_inserter(res), [](auto const &map) {
            return asArray(map);
        });
    return res;
}
auto asMaps(AsArrays const &vec, bool skip_zeros) -> AsMaps
{
    AsMaps res;
    res.reserve(vec.size());
    std::transform(
        vec.begin(),
        vec.end(),
        std::back_inserter(res),
        [&](auto const &array) { return asMap(array, skip_zeros); });
    return res;
}

namespace auxiliary
{
    void fromMapOfUnitDimension(
        double *cursor, std::map<UnitDimension, double> const &udim)
    {
        for (auto [unit, exponent] : udim)
        {
            cursor[static_cast<uint8_t>(unit)] = exponent;
        }
    }
} // namespace auxiliary
} // namespace openPMD::unit_representations
