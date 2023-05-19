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
#include "openPMD/backend/Writable.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/DerefDynamicCast.hpp"

namespace openPMD
{
Writable::Writable(internal::AttributableData *a) : attributable{a}
{}

Writable::~Writable()
{
    if (!IOHandler || !IOHandler->has_value())
    {
        return;
    }
    /*
     * Enqueueing a pointer to this object, which is now being deleted.
     * The DEREGISTER task must not dereference the pointer, but only use it to
     * remove references to this object from internal data structures.
     */
    IOHandler->value()->enqueue(
        IOTask(this, Parameter<Operation::DEREGISTER>()));
}

void Writable::seriesFlush(std::string backendConfig)
{
    seriesFlush({FlushLevel::UserFlush, std::move(backendConfig)});
}

void Writable::seriesFlush(internal::FlushParams flushParams)
{
    auto series =
        Attributable({attributable, [](auto const *) {}}).retrieveSeries();
    series.flush_impl(
        series.iterations.begin(), series.iterations.end(), flushParams);
}

} // namespace openPMD
