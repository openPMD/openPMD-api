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
#include "openPMD/Streaming.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/ParsePreference.hpp"
#include "openPMD/snapshots/IteratorTraits.hpp"

#include <deque>
#include <iostream>
#include <optional>
#include <set>
#include <variant>
#include <vector>

namespace openPMD
{
namespace internal
{
    class SeriesData;
}

namespace detail
{
    namespace step_status_types
    {
        struct Before_t
        {};
        struct During_t
        {
            size_t idx;
            std::optional<Iteration::IterationIndex_t> iteration_idx;
        };
        struct After_t
        {};
    } // namespace step_status_types
    struct CurrentStep
        : std::variant<
              step_status_types::Before_t,
              step_status_types::During_t,
              step_status_types::After_t>
    {
        using Before_t = step_status_types::Before_t;
        constexpr static Before_t Before{};
        using During_t = step_status_types::During_t;
        constexpr static During_t During{};
        using After_t = step_status_types::After_t;
        constexpr static After_t After{};

        using variant_t = std::variant<
            step_status_types::Before_t,
            step_status_types::During_t,
            step_status_types::After_t>;

        using variant_t::operator=;

        template <typename V>
        auto get_variant() -> std::optional<V *>;
        template <typename V>
        auto get_variant() const -> std::optional<V const *>;

        auto get_iteration_index() const
            -> std::optional<Iteration::IterationIndex_t const *>;
        auto get_iteration_index()
            -> std::optional<Iteration::IterationIndex_t *>;

        enum class AtTheEdge : bool
        {
            Begin,
            End
        };

        template <typename F, typename G>
        auto map_during_t(F &&map, G &&create_new);
        template <typename F>
        auto map_during_t(F &&map);
    };

    namespace seek_types
    {
        struct Next_t
        {};
        struct Seek_Iteration_t
        {
            Iteration::IterationIndex_t iteration_idx;
        };
    } // namespace seek_types

    struct Seek : std::variant<seek_types::Next_t, seek_types::Seek_Iteration_t>
    {
        using Next_t = seek_types::Next_t;
        using Seek_Iteration_t = seek_types::Seek_Iteration_t;

        static constexpr Next_t const Next{};
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

    using CurrentStep = detail::CurrentStep;

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
        CurrentStep currentStep = {CurrentStep::Before};
        std::optional<internal::ParsePreference> parsePreference;
        /*
         * Necessary because in the old ADIOS2 schema, old iterations' metadata
         * will leak into new steps, making the frontend think that the groups
         * are still there and the iterations can be parsed again.
         */
        std::set<Iteration::IterationIndex_t> ignoreIterations;

        auto currentIteration() -> std::optional<Iteration::IterationIndex_t *>;
        auto currentIteration() const
            -> std::optional<Iteration::IterationIndex_t const *>;
    };

    /*
     * The shared data is never empty, emptiness is indicated by std::optional
     */
    std::shared_ptr<std::optional<SharedData>> m_data =
        std::make_shared<std::optional<SharedData>>(std::nullopt);

    auto get() -> SharedData &;
    auto get() const -> SharedData const &;

    using parent_t = AbstractSeriesIterator<
        StatefulIterator,
        Container<Iteration, Iteration::IterationIndex_t>::value_type>;

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
    /*
     * This is considered an end iterator if:
     *
     * 1. The iterator has no state at all
     *    (generic statically created end iterator)
     * 2. The state is During_t with no iteration index
     *    (finished reading iterations in a randomly-accessible Series)
     * 3. The state is After_t
     *    (closed the last step in a step-wise Series)
     */
    auto is_end() const -> bool;

    operator bool() const;

    // Custom non-iterator methods
    auto seek(Seek const &) -> StatefulIterator *;

private:
    std::optional<StatefulIterator *> nextIterationInStep();
    std::optional<StatefulIterator *> skipToIterationInStep(iteration_index_t);

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
    std::optional<StatefulIterator *> nextStep(size_t recursion_depth);

    std::optional<StatefulIterator *> loopBody(Seek const &);

    void initIteratorFilebased();

    void deactivateDeadIteration(iteration_index_t);

    void initSeriesInLinearReadMode();

    void close();
    enum class TypeOfEndIterator : char
    {
        NoMoreSteps,
        NoMoreIterationsInStep
    };
    auto turn_into_end_iterator(TypeOfEndIterator) -> void;
    auto assert_end_iterator() const -> void;

    auto resetCurrentIterationToBegin(size_t num_skipped_iterations) -> void;
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

// Template definitions

namespace openPMD::detail
{
template <typename F, typename G>
auto CurrentStep::map_during_t(F &&map, G &&create_new)
{
    std::visit(
        auxiliary::overloaded{
            [&](During_t &during) { std::forward<F>(map)(during); },
            [&](Before_t const &) {
                std::optional<variant_t> res =
                    std::forward<G>(create_new)(AtTheEdge::Begin);
                if (res.has_value())
                {
                    this->swap(*res);
                }
            },
            [&](After_t const &) {
                std::optional<variant_t> res =
                    std::forward<G>(create_new)(AtTheEdge::End);
                if (res.has_value())
                {
                    this->swap(*res);
                }
            }},
        *this);
}

template <typename F>
auto CurrentStep::map_during_t(F &&map)
{
    map_during_t(
        std::forward<F>(map), [](auto const &) { return std::nullopt; });
}
} // namespace openPMD::detail
