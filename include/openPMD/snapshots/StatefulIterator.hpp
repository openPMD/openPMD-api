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
#include <unordered_map>
#include <variant>
#include <vector>

/*
 * Private header not included in user code.
 * Implements the Iterator interface for the stateful/synchronous workflow.
 */

namespace openPMD
{
namespace internal
{
    class SeriesData;
}

namespace detail
{
    /* The iterator status is either of the following:
     */
    namespace step_status_types
    {
        /* No step was opened yet, the Series was just opened.
         */
        struct Before_t
        {};
        /* A step is currently active
         */
        struct During_t
        {
            // The index of the current Step.
            size_t step_count;
            // The current Iteration within the Step.
            // Empty optional indicates that no Iteration is left in the current
            // step for processing, i.e. a new step must be opened or the Series
            // is over.
            std::optional<Iteration::IterationIndex_t> iteration_idx;
            // Iteration indexes that are accessible within the current step.
            // These are not modified when closing an Iteration as long as the
            // current IO step stays active.
            std::vector<Iteration::IterationIndex_t>
                available_iterations_in_step;

            During_t(
                size_t step_count,
                std::optional<Iteration::IterationIndex_t> iteration_idx,
                std::vector<Iteration::IterationIndex_t>
                    available_iterations_in_step);
        };
        /* No further data available in the Series.
         */
        struct After_t
        {};
    } // namespace step_status_types

    /* This class unifies the current step status as described above into a
     * std::variant with some helper functions.
     */
    struct CurrentStep
        : std::variant<
              step_status_types::Before_t,
              step_status_types::During_t,
              step_status_types::After_t>
    {
        using Before_t = step_status_types::Before_t;
        constexpr static Before_t Before{};
        using During_t = step_status_types::During_t;
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
            -> std::optional<Iteration::IterationIndex_t>;

        /* Passed as first param of create_new lambda in map_during_t, so the
         * lambda can make an appropriate case distinction.
         */
        enum class AtTheEdge : bool
        {
            Begin,
            End
        };

        /*
         * Helper for a common way to access the underlying variant.
         * `map` has type `auto (During_t &) -> void`, i.e. it can modify the
         * `During_t` struct if the variant holds it. In other cases,
         * `create_new` is called, it has the type
         * `auto (AtTheEdge) -> std::optional<T>`.
         * `AtTheEdge` is used for specifying if the variant status is Begin or
         * End. If the returned optional contains a value, that value is swapped
         * with the current variant.
         */
        template <typename F, typename G>
        void map_during_t(F &&map, G &&create_new);

        /*
         * Overload where `create_new` is a no-op.
         */
        template <typename F>
        void map_during_t(F &&map);

        // casts needed because of
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90943
        inline auto as_base() const -> variant_t const &
        {
            return *this;
        }
        inline auto as_base() -> variant_t &
        {
            return *this;
        }
    };

    /*
     * Types for telling the Iterator where to go next.
     */
    namespace seek_types
    {
        /* Just give me the next Iteration */
        struct Next_t
        {};
        /* Give me some specific Iteration */
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

        using variant_t =
            std::variant<seek_types::Next_t, seek_types::Seek_Iteration_t>;
        // casts needed because of
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90943
        inline auto as_base() const -> variant_t const &
        {
            return *this;
        }
        inline auto as_base() -> variant_t &
        {
            return *this;
        }
    };
} // namespace detail

/** Based on the logic of the former class ReadIterations, integrating into
 *  itself the logic of former WriteIterations.
 */
class StatefulIterator
    : public AbstractSeriesIterator<
          StatefulIterator,
          Container<Iteration, Iteration::IterationIndex_t>::value_type>
{
    friend class StatefulSnapshotsContainer;
    friend class Series;
    friend class internal::SeriesData;

    using iteration_index_t = IndexedIteration::index_t;

    using CurrentStep = detail::CurrentStep;

    struct SharedData
    {
        SharedData() = default;
        SharedData(SharedData const &) = delete;
        SharedData(SharedData &&) = delete;
        SharedData &operator=(SharedData const &) = delete;
        SharedData &operator=(SharedData &&) = delete;

        ~SharedData();

        using step_index = size_t;

        /*
         * This must be a non-owning internal handle to break reference cycles.
         * A non-owning handle is fine due to the usual semantics for iterator
         * invalidation.
         */
        Series series;
        /*
         * No step opened yet, so initialize this with CurrentStep::Before. Look
         * to the documentation of Before_t, During_t, After_t and CurrentStep
         * classes above for more info.
         */
        CurrentStep currentStep = {CurrentStep::Before};
        /*
         * Stores the parse preference optionally passed in the constructor.
         * Decides if IO step logic is actually used.
         */
        std::optional<internal::ParsePreference> parsePreference;
        /*
         * Store which Iterations we already saw and in which IO step we did.
         * Currently used for eliminating repetitions when (e.g. due to
         * checkpoint-restart workflows) Iterations repeat in different steps.
         *
         * Possible future uses:
         *
         * 1. Support jumping back to a previous step in order to reopen an
         *    Iteration previously seen. (Would require reopening files in
         *    ADIOS2, but so be it.)
         * 2. Pre-parsing a variable-based file for repeating Iterations and
         *    eliminating the earlier instances of repeated Iterations (instead
         *    of the later instances as is done now).
         */
        std::unordered_map<iteration_index_t, step_index> seen_iterations;

        /*
         * This returns the current value of `During_t::iteration_idx` if that
         * exists.
         */
        auto
        currentIteration() const -> std::optional<Iteration::IterationIndex_t>;
    };

    /*
     * The shared pointer is never empty,
     * emptiness is indicated by std::optional.
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
    ~StatefulIterator() override;

    StatefulIterator(StatefulIterator const &other);
    StatefulIterator(StatefulIterator &&other) noexcept;

    StatefulIterator &operator=(StatefulIterator const &other);
    StatefulIterator &operator=(StatefulIterator &&other) noexcept;

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
    auto operator*() -> value_type &;
    auto operator*() const -> value_type const &;

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

    /*
     * Try moving this Iterator to the location specified by Seek, i.e.:
     *
     * 1. Either the next available Iteration
     * 2. Or a specific Iteration specified by an index.
     *
     * A new step will be opened for this purpose if needed.
     */
    auto seek(Seek const &) -> StatefulIterator &;

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

    /*
     * Called when an Iteration was just opened but entirely fails parsing.
     */
    void deactivateDeadIteration(iteration_index_t);

    void initSeriesInLinearReadMode();

    void close();

    /* When not using IO steps, the status should not be set to After_t, but be
     * kept as During_t. This way, Iterations can still be opened without the
     * Iterator thinking it's from a past step.
     */
    enum class TypeOfEndIterator : char
    {
        NoMoreSteps,
        NoMoreIterationsInStep
    };
    auto turn_into_end_iterator(TypeOfEndIterator) -> void;
    auto assert_end_iterator() const -> void;

    auto resetCurrentIterationToBegin(
        size_t num_skipped_iterations,
        std::vector<iteration_index_t> current_iterations) -> void;
    auto
    peekCurrentlyOpenIteration() const -> std::optional<value_type const *>;
    auto peekCurrentlyOpenIteration() -> std::optional<value_type *>;

    auto reparse_possibly_deleted_iteration(iteration_index_t) -> void;
};
} // namespace openPMD

// Template definitions

namespace openPMD::detail
{
template <typename F, typename G>
void CurrentStep::map_during_t(F &&map, G &&create_new)
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
        this->as_base());
}

template <typename F>
void CurrentStep::map_during_t(F &&map)
{
    map_during_t(
        std::forward<F>(map), [](auto const &) { return std::nullopt; });
}
} // namespace openPMD::detail
