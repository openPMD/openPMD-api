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

#include "openPMD/Error.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/backend/ParsePreference.hpp"
#include "openPMD/snapshots/IteratorTraits.hpp"

#include <deque>
#include <iostream>
#include <optional>
#include <set>

namespace openPMD
{
class SeriesIterator
    : public AbstractSeriesIterator<
          SeriesIterator,
          Container<Iteration, Iteration::IterationIndex_t>::value_type>
{
    using iteration_index_t = IndexedIteration::index_t;

    using maybe_series_t = std::optional<Series>;

    struct SharedData
    {
        SharedData() = default;
        SharedData(SharedData const &) = delete;
        SharedData(SharedData &&) = delete;
        SharedData &operator=(SharedData const &) = delete;
        SharedData &operator=(SharedData &&) = delete;

        maybe_series_t series;
        std::deque<iteration_index_t> iterationsInCurrentStep;
        uint64_t currentIteration{};
        std::optional<internal::ParsePreference> parsePreference;
        /*
         * Necessary because in the old ADIOS2 schema, old iterations' metadata
         * will leak into new steps, making the frontend think that the groups
         * are still there and the iterations can be parsed again.
         */
        std::set<Iteration::IterationIndex_t> ignoreIterations;
    };

    /*
     * The shared data is never empty, emptiness is indicated by std::optional
     */
    std::shared_ptr<std::optional<SharedData>> m_data =
        std::make_shared<std::optional<SharedData>>(std::nullopt);

    SharedData &get()
    {
        return m_data->value();
    }
    SharedData const &get() const
    {
        return m_data->value();
    }

    using parent_t = AbstractSeriesIterator<SeriesIterator>;

public:
    using value_type =
        typename Container<Iteration, Iteration::IterationIndex_t>::value_type;
    using typename parent_t ::difference_type;
    //! construct the end() iterator
    explicit SeriesIterator();

    SeriesIterator(
        Series const &,
        std::optional<internal::ParsePreference> const &parsePreference);

    // dereference
    using parent_t::operator*;
    value_type const &operator*() const;

    // increment/decrement
    SeriesIterator &operator++();
    using parent_t::operator--;
    inline SeriesIterator &operator--()
    {
        throw error::WrongAPIUsage(
            "Global stateful iterator supports no decrement (yet).");
    }
    SeriesIterator operator++(int)
    {
        throw error::WrongAPIUsage(
            "Global stateful iterator supports no post-increment.");
    }

    // comparison
    difference_type operator-(SeriesIterator const &) const
    {
        throw error::WrongAPIUsage(
            "Global stateful iterator supports no relative comparison.");
    }
    bool operator==(SeriesIterator const &other) const;
    inline bool operator<(SeriesIterator const &) const
    {
        throw error::WrongAPIUsage(
            "Global stateful iterator supports no relative comparison.");
    }

    static SeriesIterator end();

    operator bool();

private:
    inline bool setCurrentIteration()
    {
        auto &data = get();
        if (data.iterationsInCurrentStep.empty())
        {
            std::cerr << "[ReadIterations] Encountered a step without "
                         "iterations. Closing the Series."
                      << std::endl;
            *this = end();
            return false;
        }
        data.currentIteration = *data.iterationsInCurrentStep.begin();
        return true;
    }

    inline std::optional<uint64_t> peekCurrentIteration()
    {
        auto &data = get();
        if (data.iterationsInCurrentStep.empty())
        {
            return std::nullopt;
        }
        else
        {
            return {*data.iterationsInCurrentStep.begin()};
        }
    }

    std::optional<SeriesIterator *> nextIterationInStep();

    /*
     * When a step cannot successfully be opened, the method nextStep() calls
     * itself again recursively.
     * (Recursion massively simplifies the logic here, and it only happens
     * in case of error.)
     * After successfully beginning a step, this methods needs to remember, how
     * many broken steps have been skipped. In case the Series does not use
     * the /data/snapshot attribute, this helps figuring out which iteration
     * is now active. Hence, recursion_depth.
     */
    std::optional<SeriesIterator *> nextStep(size_t recursion_depth);

    std::optional<SeriesIterator *> loopBody();

    void deactivateDeadIteration(iteration_index_t);

    void initSeriesInLinearReadMode();

    void close();
};

class LegacyIteratorAdaptor
{
    using value_type = IndexedIteration;
    using parent_t = SeriesIterator;

private:
    friend class ReadIterations;
    SeriesIterator m_iterator;
    LegacyIteratorAdaptor(SeriesIterator iterator)
        : m_iterator(std::move(iterator))
    {}

public:
    inline value_type operator*() const
    {
        return m_iterator.operator*();
    }

    inline LegacyIteratorAdaptor &operator++()
    {
        ++m_iterator;
        return *this;
    }

    static LegacyIteratorAdaptor end()
    {
        return SeriesIterator::end();
    }

    inline bool operator==(LegacyIteratorAdaptor const &other) const
    {
        return m_iterator == other.m_iterator;
    };

    inline bool operator!=(LegacyIteratorAdaptor const &other) const
    {
        return m_iterator != other.m_iterator;
    };
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
    using iterator_t = LegacyIteratorAdaptor;

    Series m_series;
    std::optional<internal::ParsePreference> m_parsePreference;

    ReadIterations(
        Series,
        Access,
        std::optional<internal::ParsePreference> parsePreference);

public:
    iterator_t begin();

    iterator_t end();
};
} // namespace openPMD
