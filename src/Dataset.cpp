/* Copyright 2017-2021 Fabian Koller
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
#include "openPMD/Dataset.hpp"
#include "openPMD/Error.hpp"

#include <cstddef>
#include <iostream>

namespace openPMD
{
Dataset::Dataset(Datatype d, Extent e, std::string options_in)
    : extent{std::move(e)}, dtype{d}, options{std::move(options_in)}
{
    // avoid initialization order issues
    rank = static_cast<uint8_t>(extent.size());
    // Call this in order to have early error message in case of wrong
    // specification of joined dimensions
    joinedDimension();
}

Dataset::Dataset(Extent e) : Dataset(Datatype::UNDEFINED, std::move(e))
{}

Dataset &Dataset::extend(Extent newExtents)
{
    if (newExtents.size() != rank)
        throw std::runtime_error(
            "Dimensionality of extended Dataset must match the original "
            "dimensionality");
    for (size_t i = 0; i < newExtents.size(); ++i)
        if (newExtents[i] < extent[i])
            throw std::runtime_error(
                "New Extent must be equal or greater than previous Extent");

    extent = newExtents;
    return *this;
}

bool Dataset::empty() const
{
    auto jd = joinedDimension();
    for (size_t i = 0; i < extent.size(); ++i)
    {
        if (extent[i] == 0 && (!jd.has_value() || jd.value() != i))
        {
            return true;
        }
    }
    return false;
}

std::optional<size_t> Dataset::joinedDimension() const
{
    std::optional<size_t> res;
    for (size_t i = 0; i < extent.size(); ++i)
    {
        if (extent[i] == JOINED_DIMENSION)
        {
            if (res.has_value())
            {
                throw error::WrongAPIUsage(
                    "Must specify JOINED_DIMENSION at most once (found at "
                    "indices " +
                    std::to_string(res.value()) + " and " + std::to_string(i) +
                    ")");
            }
            else
            {
                res = i;
            }
        }
    }
    return res;
}
} // namespace openPMD
