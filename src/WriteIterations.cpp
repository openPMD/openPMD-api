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

#include "openPMD/Series.hpp"

namespace openPMD
{
WriteIterations::SharedResources::SharedResources(iterations_t _iterations)
    : iterations(std::move(_iterations))
{}

WriteIterations::SharedResources::~SharedResources()
{
    if (currentlyOpen.has_value() &&
        iterations.retrieveSeries().get().m_lastFlushSuccessful)
    {
        auto lastIterationIndex = currentlyOpen.get();
        auto &lastIteration = iterations.at(lastIterationIndex);
        if (!lastIteration.closed())
        {
            lastIteration.close();
        }
    }
}

WriteIterations::WriteIterations(iterations_t iterations)
    : shared{std::make_shared<SharedResources>(std::move(iterations))}
{}

WriteIterations::mapped_type &WriteIterations::operator[](key_type const &key)
{
    // make a copy
    // explicit cast so MSVC can figure out how to do it correctly
    return operator[](static_cast<key_type &&>(key_type{key}));
}
WriteIterations::mapped_type &WriteIterations::operator[](key_type &&key)
{
    if (shared->currentlyOpen.has_value())
    {
        auto lastIterationIndex = shared->currentlyOpen.get();
        auto &lastIteration = shared->iterations.at(lastIterationIndex);
        if (lastIterationIndex != key && !lastIteration.closed())
        {
            lastIteration.close();
        }
    }
    shared->currentlyOpen = key;
    auto &res = shared->iterations[std::move(key)];
    if (res.getStepStatus() == StepStatus::NoStep)
    {
        res.beginStep(/* reread = */ false);
        res.setStepStatus(StepStatus::DuringStep);
    }
    return res;
}
} // namespace openPMD
