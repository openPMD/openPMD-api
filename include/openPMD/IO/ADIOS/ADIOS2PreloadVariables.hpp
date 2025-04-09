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
#pragma once

#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/config.hpp"
#include <deque>
#include <optional>
#include <variant>
#if openPMD_HAVE_ADIOS2

#include <adios2.h>
#include <map>

namespace openPMD::detail
{
struct AdiosVariables
{
    // Buffered map for current step
    using AttributeMap_t = std::map<std::string, adios2::Params>;
    std::optional<AttributeMap_t> m_availableVariables;
    size_t currentStep;

    // Preparsed step data
    struct RandomAccessPreparsed_t
    {
        // Variable only defined in these steps
        using PartialVariable = std::optional<std::deque<size_t>>;
        std::map<std::string, std::deque<size_t>> m_partialVariables;

        AttributeMap_t m_allVariables;
    };
    std::optional<RandomAccessPreparsed_t> m_preparsed;

    auto availableAttributes(size_t step, adios2::IO &IO)
        -> AttributeMap_t const &;
};
} // namespace openPMD::detail

#endif // openPMD_HAVE_ADIOS2
