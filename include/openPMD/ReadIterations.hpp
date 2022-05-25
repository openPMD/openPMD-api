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
#include "openPMD/Series.hpp"

namespace openPMD
{
/**
 * @brief Subclass of Iteration that knows its own index withing the containing
 *        Series.
 */
class IndexedIteration : public Iteration
{
    friend class SeriesIterator;

public:
    using iterations_t = decltype(internal::SeriesData::iterations);
    using index_t = iterations_t::key_type;
    index_t const iterationIndex;

private:
    template <typename Iteration_t>
    IndexedIteration(Iteration_t &&it, index_t index)
        : Iteration(std::forward<Iteration_t>(it)), iterationIndex(index)
    {}
};

class SeriesIterator
{
    using iteration_index_t = IndexedIteration::index_t;

    using maybe_series_t = auxiliary::Option<Series>;

    maybe_series_t m_series;
    iteration_index_t m_currentIteration = 0;

public:
    //! construct the end() iterator
    explicit SeriesIterator();

    SeriesIterator(Series);

    SeriesIterator &operator++();

    IndexedIteration operator*();

    bool operator==(SeriesIterator const &other) const;

    bool operator!=(SeriesIterator const &other) const;

    static SeriesIterator end();
};

/**
 * @brief Reading side of the streaming API.
 *
 * Create instance via Series::readIterations().
 * For use in a C++11-style foreach loop over iterations.
 * Designed to allow reading any kind of Series, streaming and non-
 * streaming alike.
 * Calling Iteration::close() manually before opening the next iteration is
 * encouraged and will implicitly flush all deferred IO actions.
 * Otherwise, Iteration::close() will be implicitly called upon
 * SeriesIterator::operator++(), i.e. upon going to the next iteration in
 * the foreach loop.
 * Since this is designed for streaming mode, reopening an iteration is
 * not possible once it has been closed.
 *
 */
class ReadIterations
{
    friend class Series;

private:
    using iterations_t = decltype(internal::SeriesData::iterations);
    using iterator_t = SeriesIterator;

    Series m_series;

    ReadIterations(Series);

public:
    iterator_t begin();

    iterator_t end();
};
} // namespace openPMD
