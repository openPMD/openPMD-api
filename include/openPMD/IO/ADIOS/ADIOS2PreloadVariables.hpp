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
    // For which step were the above variables buffered?
    size_t currentStep;
    // Optimization: If variable definitions do not vary across steps, no need
    // to recompute them
    bool variables_are_static = false;

    // Preparsed step data
    struct RandomAccessPreparsed_t
    {
        // Variable only defined in these steps
        std::map<std::string, std::vector<size_t>> m_partialVariables;

        AttributeMap_t m_allVariables;
    };
    std::optional<RandomAccessPreparsed_t> m_preparsed;

    /*
     * If use_step_selection is false, but preparsed step data is available,
     * this means that Advance(stepSelection = null) was executed previously.
     * So, we can return m_preparsed->m_allVariables.
     */
    auto
    availableVariables(size_t step, bool use_step_selection, adios2::IO &IO)
        -> AttributeMap_t const &;
};
} // namespace openPMD::detail

#endif // openPMD_HAVE_ADIOS2
