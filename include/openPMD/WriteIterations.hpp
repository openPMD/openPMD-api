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
#pragma once

#include "openPMD/Iteration.hpp"
#include "openPMD/backend/Container.hpp"

#include <optional>

namespace openPMD
{
class Series;

/** Writing side of the streaming API.
 *
 * Create instance via Series::writeIterations().
 * For use via WriteIterations::operator[]().
 * Designed to allow reading any kind of Series, streaming and non-
 * streaming alike. Calling Iteration::close() manually before opening
 * the next iteration is encouraged and will implicitly flush all
 * deferred IO actions. Otherwise, Iteration::close() will be implicitly
 * called upon SeriesIterator::operator++(), i.e. upon going to the next
 * iteration in the foreach loop.
 *
 * Since this is designed for streaming mode, reopening an iteration is
 * not possible once it has been closed.
 *
 */

namespace internal
{
    class SeriesData;
}

class WriteIterations
{
    friend class Series;
    friend class internal::SeriesData;

private:
    using IterationsContainer_t =
        Container<Iteration, Iteration::IterationIndex_t>;

public:
    using key_type = IterationsContainer_t::key_type;
    using mapped_type = IterationsContainer_t::mapped_type;
    using value_type = IterationsContainer_t::value_type;
    using reference = IterationsContainer_t::reference;

private:
    struct SharedResources
    {
        IterationsContainer_t iterations;
        //! Index of the last opened iteration
        std::optional<Iteration::IterationIndex_t> currentlyOpen;

        SharedResources(IterationsContainer_t);
        ~SharedResources();
    };

    WriteIterations(IterationsContainer_t);
    explicit WriteIterations() = default;
    // std::optional so that a single instance is able to close this without
    // needing to wait for all instances to deallocate
    std::shared_ptr<std::optional<SharedResources>> shared;

    void close();

public:
    mapped_type &operator[](key_type const &key);
    mapped_type &operator[](key_type &&key);
};
} // namespace openPMD
