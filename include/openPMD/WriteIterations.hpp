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
#include "openPMD/auxiliary/Option.hpp"
#include "openPMD/backend/Container.hpp"

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
class WriteIterations : private Container<Iteration, uint64_t>
{
    friend class Series;

private:
    using iterations_t = Container<Iteration, uint64_t>;

public:
    using key_type = typename iterations_t::key_type;
    using mapped_type = typename iterations_t::mapped_type;
    using value_type = typename iterations_t::value_type;
    using reference = typename iterations_t::reference;

private:
    struct SharedResources
    {
        iterations_t iterations;
        auxiliary::Option<uint64_t> currentlyOpen;

        SharedResources(iterations_t);
        ~SharedResources();
    };

    WriteIterations(iterations_t);
    explicit WriteIterations() = default;
    //! Index of the last opened iteration
    std::shared_ptr<SharedResources> shared;

public:
    mapped_type &operator[](key_type const &key) override;
    mapped_type &operator[](key_type &&key) override;
};
} // namespace openPMD
