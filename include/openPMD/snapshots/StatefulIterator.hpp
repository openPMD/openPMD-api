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
#include <vector>

namespace openPMD
{
namespace internal
{
    class SeriesData;
}

namespace detail
{
    namespace seek_types
    {
        struct InitNonFileBased_t
        {};
        struct Next_t
        {};
        using seek_impl = std::variant<InitNonFileBased_t, Next_t>;
    } // namespace seek_types
    struct Seek : seek_types::seek_impl
    {
        using InitNonFileBased_t = seek_types::InitNonFileBased_t;
        using Next_t = seek_types::Next_t;

        constexpr static InitNonFileBased_t InitNonFileBased{};
        constexpr static Next_t Next{};
    };
} // namespace detail

class StatefulIterator
    : public AbstractSeriesIterator<
          StatefulIterator,
          Container<Iteration, Iteration::IterationIndex_t>::value_type>
{
    friend class StatefulSnapshotsContainer;
    friend class Series;
    friend class internal::SeriesData;

    using iteration_index_t = IndexedIteration::index_t;

    using maybe_series_t = std::optional<Series>;

    struct SharedData
    {
        SharedData() = default;
        SharedData(SharedData const &) = delete;
        SharedData(SharedData &&) = delete;
        SharedData &operator=(SharedData const &) = delete;
        SharedData &operator=(SharedData &&) = delete;

        ~SharedData();

        Series series;
        std::vector<iteration_index_t> iterationsInCurrentStep;
        // nullopt <-> currently out of step
        std::optional<iteration_index_t> currentIteration{};
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

    auto get() -> SharedData &;
    auto get() const -> SharedData const &;

    using parent_t = AbstractSeriesIterator<StatefulIterator>;

public:
    using value_type =
        typename Container<Iteration, Iteration::IterationIndex_t>::value_type;
    using typename parent_t ::difference_type;
    using Seek = detail::Seek;
    //! construct the end() iterator
    explicit StatefulIterator();

    class tag_write_t
    {};
    static constexpr tag_write_t const tag_write{};
    class tag_read_t
    {};
    static constexpr tag_read_t const tag_read{};

    StatefulIterator(
        tag_read_t,
        Series const &,
        std::optional<internal::ParsePreference> const &parsePreference);

    StatefulIterator(tag_write_t, Series const &);

    // dereference
    using parent_t::operator*;
    value_type const &operator*() const;

    // increment/decrement
    auto operator++() -> StatefulIterator &;
    auto operator--() -> StatefulIterator &;
    auto operator--(int) -> StatefulIterator;
    auto operator++(int) -> StatefulIterator;

    // comparison
    auto operator-(StatefulIterator const &) const -> difference_type;
    bool operator==(StatefulIterator const &other) const;
    auto operator<(StatefulIterator const &) const -> bool;

    static auto end() -> StatefulIterator;

    operator bool() const;

private:
    std::optional<StatefulIterator *> nextIterationInStep();

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
    std::optional<StatefulIterator *> nextStep(Seek const &);

    std::optional<StatefulIterator *> loopBody(Seek const &);

    void deactivateDeadIteration(iteration_index_t);

    void initSeriesInLinearReadMode();

    void close();

    auto resetCurrentIterationToBegin() -> bool;
    auto peekCurrentlyOpenIteration() const
        -> std::optional<value_type const *>;
    auto peekCurrentlyOpenIteration() -> std::optional<value_type *>;
};

class LegacyIteratorAdaptor
{
    using value_type = IndexedIteration;
    using parent_t = StatefulIterator;

private:
    friend class ReadIterations;
    StatefulIterator m_iterator;
    LegacyIteratorAdaptor(StatefulIterator iterator);

public:
    value_type operator*() const;
    LegacyIteratorAdaptor &operator++();
    static LegacyIteratorAdaptor end();
    bool operator==(LegacyIteratorAdaptor const &other) const;
    bool operator!=(LegacyIteratorAdaptor const &other) const;
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
 * StatefulIterator::operator++(), i.e. upon going to the next iteration in
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
    auto begin() -> iterator_t;
    auto end() -> iterator_t;
};
} // namespace openPMD
