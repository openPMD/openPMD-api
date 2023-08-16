/* Copyright 2021 Franz Poeschel
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

#include "openPMD/WriteIterations.hpp"
#include "openPMD/Error.hpp"

#include "openPMD/Series.hpp"

namespace openPMD
{
WriteIterations::SharedResources::SharedResources(
    IterationsContainer_t _iterations)
    : iterations(std::move(_iterations))
{}

WriteIterations::SharedResources::~SharedResources()
{
    if (auto IOHandler = iterations.IOHandler(); currentlyOpen.has_value() &&
        IOHandler && IOHandler->m_lastFlushSuccessful)
    {
        auto lastIterationIndex = currentlyOpen.value();
        auto &lastIteration = iterations.at(lastIterationIndex);
        if (!lastIteration.closed())
        {
            lastIteration.close();
        }
    }
}

WriteIterations::WriteIterations(IterationsContainer_t iterations)
    : shared{std::make_shared<std::optional<SharedResources>>(
          std::move(iterations))}
{}

void WriteIterations::close()
{
    *shared = std::nullopt;
}

WriteIterations::mapped_type &WriteIterations::operator[](key_type const &key)
{
    // make a copy
    // explicit cast so MSVC can figure out how to do it correctly
    return operator[](static_cast<key_type &&>(key_type{key}));
}
WriteIterations::mapped_type &WriteIterations::operator[](key_type &&key)
{
    if (!shared || !shared->has_value())
    {
        throw error::WrongAPIUsage(
            "[WriteIterations] Trying to access after closing Series.");
    }
    auto &s = shared->value();
    if (s.currentlyOpen.has_value())
    {
        auto lastIterationIndex = s.currentlyOpen.value();
        auto &lastIteration = s.iterations.at(lastIterationIndex);
        if (lastIterationIndex != key && !lastIteration.closed())
        {
            lastIteration.close();
        }
    }
    s.currentlyOpen = key;
    auto &res = s.iterations[std::move(key)];
    if (res.getStepStatus() == StepStatus::NoStep)
    {
        try
        {
            res.beginStep(/* reread = */ false);
        }
        catch (error::OperationUnsupportedInBackend const &)
        {
            s.iterations.retrieveSeries()
                .get()
                .m_currentlyActiveIterations.clear();
            throw;
        }
        res.setStepStatus(StepStatus::DuringStep);
    }
    return res;
}
} // namespace openPMD
