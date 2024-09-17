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
#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/DerefDynamicCast.hpp"
#include <stdexcept>

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
        IOTask(this, Parameter<Operation::DEREGISTER>(parent)));
}

template <bool flush_entire_series>
void Writable::seriesFlush(std::string backendConfig)
{
    seriesFlush<flush_entire_series>(
        internal::FlushParams{FlushLevel::UserFlush, std::move(backendConfig)});
}
template void Writable::seriesFlush<true>(std::string backendConfig);
template void Writable::seriesFlush<false>(std::string backendConfig);

template <bool flush_entire_series>
void Writable::seriesFlush(internal::FlushParams const &flushParams)
{
    Attributable impl;
    impl.setData({attributable, [](auto const *) {}});
    auto [iteration_internal, series_internal] = impl.containingIteration();
    if (iteration_internal)
    {
        (*iteration_internal)->asInternalCopyOf<Iteration>().touch();
    }
    auto series = series_internal->asInternalCopyOf<Series>();
    auto [begin, end] = [&, &iteration_internal_lambda = iteration_internal]()
        -> std::pair<Series::iterations_iterator, Series::iterations_iterator> {
        if (!flush_entire_series)
        {
            if (!iteration_internal_lambda.has_value())
            {
                throw std::runtime_error(
                    "[Writable::seriesFlush()] Requested flushing the "
                    "containing Iteration, but no Iteration was found?");
            }
            auto it = series.iterations.begin();
            auto end_lambda = series.iterations.end();
            for (; it != end_lambda; ++it)
            {
                if (&it->second.Iteration::get() == *iteration_internal_lambda)
                {
                    auto next = it;
                    ++next;
                    return {it, next};
                }
            }
            throw std::runtime_error(
                "[Writable::seriesFlush()] Found a containing Iteration that "
                "seems to not be part of the containing Series?? You might try "
                "running this with `flushing_entire_series=false` as a "
                "workaround, but something is still wrong.");
        }
        else
        {
            return {series.iterations.begin(), series.iterations.end()};
        }
    }();
    series.flush_impl(begin, end, flushParams);
}
template void
Writable::seriesFlush<true>(internal::FlushParams const &flushParams);
template void
Writable::seriesFlush<false>(internal::FlushParams const &flushParams);
} // namespace openPMD
