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

#include "openPMD/snapshots/StatefulIterator.hpp"
#include "openPMD/Error.hpp"

#include "openPMD/Iteration.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/Variant.hpp"

#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <variant>
#include <vector>

namespace openPMD
{

namespace detail
{
    step_status_types::During_t::During_t(
        size_t idx_in,
        std::optional<Iteration::IterationIndex_t> iteration_idx_in,
        std::vector<Iteration::IterationIndex_t>
            available_iterations_in_step_in)
        : idx(idx_in)
        , iteration_idx(iteration_idx_in)
        , available_iterations_in_step(
              std::move(available_iterations_in_step_in))
    {}

    template <typename V>
    auto CurrentStep::get_variant() -> std::optional<V *>
    {
        auto res = std::get_if<V>(this);
        if (res)
        {
            return std::make_optional<V *>(res);
        }
        else
        {
            return std::nullopt;
        }
    }

    template <typename V>
    auto CurrentStep::get_variant() const -> std::optional<V const *>
    {
        auto res = std::get_if<V>(*this);
        if (res)
        {
            return {res};
        }
        else
        {
            return std::nullopt;
        }
    }

    auto CurrentStep::get_iteration_index() const
        -> std::optional<Iteration::IterationIndex_t const *>
    {
        using res_t = std::optional<Iteration::IterationIndex_t const *>;
        return std::visit(
            auxiliary::overloaded{
                [](auto const &) -> res_t { return std::nullopt; },
                [](During_t const &during) -> res_t {
                    if (during.iteration_idx.has_value())
                    {
                        return std::make_optional<
                            Iteration::IterationIndex_t const *>(
                            &*during.iteration_idx);
                    }
                    else
                    {
                        return std::nullopt;
                    }
                }},
            *this);
    }
    auto CurrentStep::get_iteration_index()
        -> std::optional<Iteration::IterationIndex_t *>
    {
        auto res =
            static_cast<CurrentStep const *>(this)->get_iteration_index();
        if (res.has_value())
        {
            return const_cast<Iteration::IterationIndex_t *>(*res);
        }
        else
        {
            return std::nullopt;
        }
    }
} // namespace detail

StatefulIterator::SharedData::~SharedData()
{
    auto IOHandler = series.IOHandler();
    auto current_iteration = currentIteration();
    if (IOHandler && current_iteration.has_value() && IOHandler &&
        IOHandler->m_lastFlushSuccessful)
    {
        auto lastIterationIndex = **current_iteration;
        if (!series.iterations.contains(**current_iteration))
        {
            return;
        }
        auto &lastIteration = series.iterations.at(lastIterationIndex);
        if (!lastIteration.closed())
        {
            lastIteration.close();
        }
    }
}

auto StatefulIterator::SharedData::currentIteration()
    -> std::optional<Iteration::IterationIndex_t *>
{
    return currentStep.get_iteration_index();
}
auto StatefulIterator::SharedData::currentIteration() const
    -> std::optional<Iteration::IterationIndex_t const *>
{
    return currentStep.get_iteration_index();
}

namespace
{
    bool reread(std::optional<internal::ParsePreference> parsePreference)
    {
        if (parsePreference.has_value())
        {
            using PP = Parameter<Operation::OPEN_FILE>::ParsePreference;

            switch (parsePreference.value())
            {
            case PP::PerStep:
                return true;
            case PP::UpFront:
                return false;
            }
            return false;
        }
        else
        {
            throw error::Internal(
                "Group/Variable-based encoding: Parse preference must be set.");
        }
    }
} // namespace

StatefulIterator::StatefulIterator() = default;

auto StatefulIterator::get() -> SharedData &
{
    return m_data->value();
}
auto StatefulIterator::get() const -> SharedData const &
{
    return m_data->value();
}

void StatefulIterator::initSeriesInLinearReadMode()
{
    auto &data = get();
    auto &series = data.series;
    series.IOHandler()->m_seriesStatus = internal::SeriesStatus::Parsing;
    try
    {
        switch (series.iterationEncoding())
        {
            using IE = IterationEncoding;
        case IE::fileBased:
            series.readFileBased(std::nullopt);
            break;
        case IE::groupBased:
        case IE::variableBased: {
            Parameter<Operation::OPEN_FILE> fOpen;
            fOpen.name = series.get().m_name;
            series.IOHandler()->enqueue(IOTask(&series, fOpen));
            series.IOHandler()->flush(internal::defaultFlushParams);
            using PP = Parameter<Operation::OPEN_FILE>::ParsePreference;
            switch (*fOpen.out_parsePreference)
            {
            case PP::PerStep:
                series.advance(AdvanceMode::BEGINSTEP);
                series.readGorVBased(
                    /* do_always_throw_errors = */ false,
                    /* init = */ true,
                    std::nullopt);
                break;
            case PP::UpFront:
                series.readGorVBased(
                    /* do_always_throw_errors = */ false,
                    /* init = */ true,
                    std::nullopt);
                /*
                 * In linear read mode (where no parsing at all is done upon
                 * constructing the Series), it might turn out after parsing
                 * that what we expected to be a group-based Series was in fact
                 * a single file of a file-based Series.
                 * (E.g. when opening "data00000100.h5" directly instead of
                 * "data%T.h5")
                 * So we need to check the current value of
                 * `iterationEncoding()` once more.
                 */
                if (series.iterationEncoding() != IterationEncoding::fileBased)
                {
                    series.advance(AdvanceMode::BEGINSTEP);
                }
                break;
            }
            data.parsePreference = *fOpen.out_parsePreference;
            break;
        }
        }
    }
    catch (...)
    {
        series.IOHandler()->m_seriesStatus = internal::SeriesStatus::Default;
        throw;
    }
    series.IOHandler()->m_seriesStatus = internal::SeriesStatus::Default;
}

void StatefulIterator::close()
{
    *m_data = std::nullopt; // turn this into end iterator
}

auto StatefulIterator::turn_into_end_iterator(TypeOfEndIterator type) -> void
{
    auto &data = get();
    switch (type)
    {
    case TypeOfEndIterator::NoMoreSteps:
        data.currentStep = CurrentStep::After;
        break;
    case TypeOfEndIterator::NoMoreIterationsInStep:
        data.currentStep.map_during_t(
            [](CurrentStep::During_t &during) {
                during.iteration_idx = std::nullopt;
            },
            [](auto const &) {
                return CurrentStep::During_t{0, std::nullopt, {}};
            }

        );
        break;
    }
}

auto StatefulIterator::resetCurrentIterationToBegin(
    size_t num_skipped_iterations, std::vector<iteration_index_t> indexes)
    -> void
{
    auto &data = get();
    data.currentStep.map_during_t(
        [&](CurrentStep::During_t &during) {
            during.idx += num_skipped_iterations;
            during.available_iterations_in_step = std::move(indexes);
            if (during.available_iterations_in_step.empty())
            {
                during.iteration_idx = std::nullopt;
            }
            else
            {
                during.iteration_idx =
                    *during.available_iterations_in_step.begin();
            }
        },
        [&](CurrentStep::AtTheEdge whereAmI)
            -> std::optional<CurrentStep::variant_t> {
            switch (whereAmI)
            {
            case detail::CurrentStep::AtTheEdge::Begin: {
                if (indexes.empty())
                {
                    return std::nullopt;
                }
                auto first_iteration = *indexes.begin();
                // Begin iterating
                return detail::CurrentStep::During_t{
                    num_skipped_iterations,
                    first_iteration,
                    std::move(indexes)};
            }
            case detail::CurrentStep::AtTheEdge::End:
                return std::nullopt;
            }
            throw std::runtime_error("Unreachable!");
        });
}

auto StatefulIterator::peekCurrentlyOpenIteration() const
    -> std::optional<value_type const *>
{
    if (!m_data || !m_data->has_value())
    {
        return std::nullopt;
    }
    auto &s = m_data->value();
    auto const &maybeCurrentIteration = s.currentIteration();
    if (!maybeCurrentIteration.has_value())
    {
        return std::nullopt;
    }
    // Iteration &currentIteration =
    // s.series.iterations.at(*s.currentIteration);
    auto currentIteration = s.series.iterations.find(**maybeCurrentIteration);
    if (currentIteration == s.series.iterations.end())
    {
        return std::nullopt;
    }
    if (currentIteration->second.closed())
    {
        return std::nullopt;
    }
    return std::make_optional<value_type const *>(&*currentIteration);
}
auto StatefulIterator::peekCurrentlyOpenIteration()
    -> std::optional<value_type *>
{
    if (auto res = static_cast<StatefulIterator const *>(this)
                       ->peekCurrentlyOpenIteration();
        res.has_value())
    {
        return {const_cast<value_type *>(*res)};
    }
    else
    {
        return std::nullopt;
    }
}

auto StatefulIterator::reparse_possibly_deleted_iteration(iteration_index_t idx)
    -> void
{
    auto &data = get();
    if (!data.series.iterations.contains(idx))
    {
        withRWAccess(data.series.IOHandler()->m_seriesStatus, [&]() {
            switch (data.series.iterationEncoding())
            {

            case IterationEncoding::fileBased:
                data.series.readFileBased({idx});
                break;
            case IterationEncoding::groupBased:
            case IterationEncoding::variableBased:
                data.series.readGorVBased(
                    /* do_always_throw_errors = */ true,
                    /* init = */ false,
                    {idx});
                break;
            }
        });
    }
}

StatefulIterator::StatefulIterator(tag_write_t, Series const &series_in)
    : m_data{std::make_shared<std::optional<SharedData>>(std::in_place)}
{
    auto &data = get();
    /*
     * Since the iterator is stored in
     * internal::SeriesData::m_sharedReadIterations,
     * we need to use a non-owning Series instance here for tie-breaking
     * purposes.
     * This is ok due to the usual C++ iterator invalidation workflows
     * (deleting the original container invalidates the iterator).
     */
    data.series = Series();
    data.series.setData(std::shared_ptr<internal::SeriesData>(
        series_in.m_series.get(), [](auto const *) {}));
}

StatefulIterator::StatefulIterator(
    tag_read_t,
    Series const &series_in,
    std::optional<internal::ParsePreference> const &parsePreference)
    : m_data{std::make_shared<std::optional<SharedData>>(std::in_place)}
{
    auto &data = get();
    data.parsePreference = parsePreference;
    /*
     * Since the iterator is stored in
     * internal::SeriesData::m_sharedReadIterations,
     * we need to use a non-owning Series instance here for tie-breaking
     * purposes.
     * This is ok due to the usual C++ iterator invalidation workflows
     * (deleting the original container invalidates the iterator).
     */
    data.series = Series();
    data.series.setData(std::shared_ptr<internal::SeriesData>(
        series_in.m_series.get(), [](auto const *) {}));
    auto &series = data.series;
    if (series.IOHandler()->m_frontendAccess == Access::READ_LINEAR &&
        series.iterations.empty())
    {
        initSeriesInLinearReadMode();
    }

    switch (series.iterationEncoding())
    {

    case IterationEncoding::fileBased: {
        initIteratorFilebased();
        break;
    }
    case IterationEncoding::groupBased:
    case IterationEncoding::variableBased:
        if (!seek({Seek::Next}))
        {
            this->assert_end_iterator();
        }
        break;
    }
}

std::optional<StatefulIterator *> StatefulIterator::nextIterationInStep()
{
    auto &data = get();
    auto maybeCurrentIteration =
        data.currentStep.get_variant<CurrentStep::During_t>();

    if (!maybeCurrentIteration.has_value())
    {
        return std::nullopt;
    }
    CurrentStep::During_t &currentIteration = **maybeCurrentIteration;

    auto no_result = [&]() {
        currentIteration.iteration_idx = std::nullopt;
        return std::nullopt;
    };

    if (!currentIteration.iteration_idx.has_value())
    {
        return no_result();
    }

    auto &current_iteration_idx = *currentIteration.iteration_idx;

    if (auto it = std::find(
            currentIteration.available_iterations_in_step.begin(),
            currentIteration.available_iterations_in_step.end(),
            current_iteration_idx);
        it != currentIteration.available_iterations_in_step.end())
    {
        ++it;
        if (it == currentIteration.available_iterations_in_step.end())
        {
            return no_result();
        }
        current_iteration_idx = *it;
    }
    else
    {
        return no_result();
    }

    auto &series = data.series;

    try
    {
        reparse_possibly_deleted_iteration(current_iteration_idx);
        series.iterations.at(current_iteration_idx).open();
    }
    catch (error::ReadError const &err)
    {
        std::cerr << "[StatefulIterator] Cannot read iteration '"
                  << *maybeCurrentIteration
                  << "' and will skip it due to read error:\n"
                  << err.what() << std::endl;
        return nextIterationInStep();
    }

    return {this};
}

auto StatefulIterator::skipToIterationInStep(Iteration::IterationIndex_t idx)
    -> std::optional<StatefulIterator *>
{
    auto &data = get();
    auto maybeCurrentIteration =
        data.currentStep.get_variant<CurrentStep::During_t>();

    if (!maybeCurrentIteration.has_value())
    {
        return std::nullopt;
    }
    CurrentStep::During_t &currentIteration = **maybeCurrentIteration;

    if (std::find(
            currentIteration.available_iterations_in_step.begin(),
            currentIteration.available_iterations_in_step.end(),
            idx) == currentIteration.available_iterations_in_step.end())
    {
        return std::nullopt;
    }

    // This is called upon user request, i.e. ReadErrors should be caught in
    // user code
    reparse_possibly_deleted_iteration(idx);
    data.series.iterations.at(idx).open();
    currentIteration.iteration_idx = idx;
    return {this};
}

std::optional<StatefulIterator *>
StatefulIterator::nextStep(size_t recursion_depth)
{
    auto &data = get();
    std::vector<iteration_index_t> current_iterations;
    // since we are in group-based iteration layout, it does not
    // matter which iteration we begin a step upon
    AdvanceStatus status{};
    try
    {
        std::tie(status, current_iterations) = Iteration::beginStep(
            {},
            data.series,
            /* reread = */ reread(data.parsePreference));
    }
    catch (error::ReadError const &err)
    {
        std::cerr << "[StatefulIterator] Cannot read iteration due to error "
                     "below, will skip it.\n"
                  << err.what() << std::endl;
        data.series.advance(AdvanceMode::ENDSTEP);
        return nextStep(recursion_depth + 1);
    }

    bool close = [&]() {
        switch (status)
        {

        case AdvanceStatus::OK:
            return false;
        case AdvanceStatus::OVER:
            return true;
        case AdvanceStatus::RANDOMACCESS:
            return std::visit(
                auxiliary::overloaded{
                    [](CurrentStep::Before_t const &) { return false; },
                    [](auto const &) { return true; }},
                data.currentStep);
        }
        throw std::runtime_error("Unreachable!");
    }();

    if (close)
    {
        this->turn_into_end_iterator(TypeOfEndIterator::NoMoreSteps);
    }
    else
    {
        if (auto during = data.currentStep.get_variant<CurrentStep::During_t>();
            during.has_value())
        {
            ++(*during)->idx;
        }
        resetCurrentIterationToBegin(
            recursion_depth, std::move(current_iterations));
    }
    return {this};
}

std::optional<StatefulIterator *> StatefulIterator::loopBody(Seek const &seek)
{
    auto &data = get();
    Series &series = data.series;
    auto &iterations = series.iterations;

    { /*
       * Might not be present because parsing might have failed in previous step
       */
        auto maybe_current_iteration = data.currentStep.get_iteration_index();
        if (maybe_current_iteration.has_value() &&
            iterations.contains(**maybe_current_iteration))
        {
            auto &currentIteration = iterations.at(**maybe_current_iteration);
            if (!currentIteration.closed())
            {
                currentIteration.close();
            }
            if (series.IOHandler()->m_frontendAccess == Access::READ_LINEAR)
            {
                data.series.iterations.container().erase(
                    **maybe_current_iteration);
            }
        }
    }

    auto guardReturn =
        [&series, &iterations, &data](
            auto const &option) -> std::optional<openPMD::StatefulIterator *> {
        if (!option.has_value() ||
            std::holds_alternative<CurrentStep::After_t>(
                (*option)->get().currentStep))
        {
            return option;
        }
        /*
         * A step was successfully opened, but no iterations are contained.
         * This might happen when iterations from the step are ignored, e.g.
         * a duplicate iteration has been written by Append mode.
         */
        auto maybe_current_iteration = data.currentStep.get_iteration_index();
        if (!maybe_current_iteration.has_value())
        {
            series.advance(AdvanceMode::ENDSTEP);
            return std::nullopt;
        }
        auto &current_iteration = **maybe_current_iteration;
        // If we had the iteration already, then it's either not there at all
        // (because old iterations are deleted in linear access mode),
        // or it's still there but closed in random-access mode

        if (iterations.contains(current_iteration))
        {
            auto iteration = iterations.at(current_iteration);
            switch (iteration.get().m_closed)
            {
            case internal::CloseStatus::ParseAccessDeferred:
                try
                {
                    iterations.at(current_iteration).open();
                }
                catch (error::ReadError const &err)
                {
                    std::cerr << "Cannot read iteration '" << current_iteration
                              << "' and will skip it due to read error:\n"
                              << err.what() << std::endl;
                    option.value()->deactivateDeadIteration(current_iteration);
                    return std::nullopt;
                }
                [[fallthrough]];
            case internal::CloseStatus::Open:
                return option;
            case internal::CloseStatus::Closed:
            case internal::CloseStatus::ClosedInFrontend:
                throw error::Internal(
                    "[StatefulIterator] Next step returned an iteration that "
                    "is already closed, should have opened it.");
            }
            throw std::runtime_error("Unreachable!");
        }
        else
        {
            // we had this iteration already, skip it
            series.advance(AdvanceMode::ENDSTEP);
            return std::nullopt;
        }
    };

    {
        std::optional<StatefulIterator *> optionallyAStep = std::visit(
            auxiliary::overloaded{
                [&](Seek::Next_t) { return nextIterationInStep(); },
                [&](Seek::Seek_Iteration_t const &skip_to_iteration) {
                    return skipToIterationInStep(
                        skip_to_iteration.iteration_idx);
                }},
            seek);
        if (optionallyAStep.has_value())
        {
            return guardReturn(optionallyAStep);
        }
    }

    // The currently active iterations have been exhausted.
    // Now see if there are further iterations to be found.

    if (series.iterationEncoding() == IterationEncoding::fileBased)
    {
        // this one is handled above, stream is over once it proceeds to here
        this->turn_into_end_iterator(TypeOfEndIterator::NoMoreIterationsInStep);
        return {this};
    }

    auto option = std::visit(
        auxiliary::overloaded{
            [&](Seek::Next_t) { return nextStep(/* recursion_depth = */ 0); },
            [](Seek::Seek_Iteration_t const &skip_to_iteration)
                -> std::optional<StatefulIterator *> {
                throw std::runtime_error(
                    "[StatefulIterator] Skipping to iteration " +
                    std::to_string(skip_to_iteration.iteration_idx) +
                    "from another step not supported yet");
            }},
        seek);
    return guardReturn(option);
}

void StatefulIterator::initIteratorFilebased()
{
    auto &data = get();
    auto &series = data.series;
    if (series.iterations.empty())
    {
        this->turn_into_end_iterator(TypeOfEndIterator::NoMoreIterationsInStep);
        return;
    }
    std::vector<iteration_index_t> indexes;
    indexes.reserve(series.iterations.size());
    std::transform(
        series.iterations.begin(),
        series.iterations.end(),
        std::back_inserter(indexes),
        [](auto const &pair) { return pair.first; });
    auto it = series.iterations.begin();
    auto end = series.iterations.end();
    for (; it != end; ++it)
    {
        try
        {
            it->second.open();
            break;
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "[StatefulIterator] Cannot read iteration '"
                      << it->first << "' and will skip it due to read error:\n"
                      << err.what() << std::endl;
        }
    }
    if (it != end)
    {
        data.currentStep =
            CurrentStep::During_t{0, it->first, std::move(indexes)};
    }
    else
    {
        this->turn_into_end_iterator(TypeOfEndIterator::NoMoreIterationsInStep);
    }
}

void StatefulIterator::deactivateDeadIteration(iteration_index_t index)
{
    auto &data = get();
    switch (data.series.iterationEncoding())
    {
    case IterationEncoding::fileBased: {
        Parameter<Operation::CLOSE_FILE> param;
        data.series.IOHandler()->enqueue(
            IOTask(&data.series.iterations[index], std::move(param)));
        data.series.IOHandler()->flush({FlushLevel::UserFlush});
    }
    break;
    case IterationEncoding::variableBased:
    case IterationEncoding::groupBased: {
        Parameter<Operation::ADVANCE> param;
        param.mode = AdvanceMode::ENDSTEP;
        data.series.IOHandler()->enqueue(
            IOTask(&data.series.iterations[index], std::move(param)));
        data.series.IOHandler()->flush({FlushLevel::UserFlush});
    }
    break;
    }
    data.series.iterations.container().erase(index);
}

StatefulIterator &StatefulIterator::operator++()
{
    return *seek({Seek::Next});
}

auto StatefulIterator::seek(Seek const &seek) -> StatefulIterator *
{
    auto &data = get();
    std::optional<StatefulIterator *> res;
    /*
     * loopBody() might return an empty option to indicate a skipped iteration.
     * Loop until it returns something real for us.
     * Note that this is not an infinite loop:
     * Upon end of the Series, loopBody() does not return an empty option,
     * but the end iterator.
     */
    do
    {
        res = loopBody(seek);
    } while (!res.has_value());

    auto resvalue = res.value();
    if (*resvalue != end())
    {
        auto &series = data.series;
        auto index = data.currentIteration();
        auto &iteration = series.iterations.at(*index.value());
        iteration.setStepStatus(StepStatus::DuringStep);
    }
    return resvalue;
}

auto StatefulIterator::operator*() const -> value_type const &
{
    auto &data = get();
    if (auto cur = data.currentIteration(); cur.has_value())
    {

        auto iterator = static_cast<Series::IterationsContainer_t const &>(
                            data.series.iterations)
                            .find(**cur);
        return iterator.operator*();
    }
    else
    {
        throw std::runtime_error("No iteration currently active.");
    }
}

auto StatefulIterator::operator--() -> StatefulIterator &
{
    throw error::WrongAPIUsage(
        "Global stateful iterator supports no decrement (yet).");
}
auto StatefulIterator::operator--(int) -> StatefulIterator
{
    throw error::WrongAPIUsage(
        "Global stateful iterator supports no post-decrement.");
}
auto StatefulIterator::operator++(int) -> StatefulIterator
{
    throw error::WrongAPIUsage(
        "Global stateful iterator supports no post-increment.");
}

// comparison
auto StatefulIterator::operator-(StatefulIterator const &) const
    -> difference_type
{
    throw error::WrongAPIUsage(
        "Global stateful iterator supports no relative comparison.");
}
bool StatefulIterator::operator==(StatefulIterator const &other) const
{
    return
        // either both iterators are filled
        (this->m_data->has_value() && other.m_data->has_value() &&
         (this->get().currentIteration() == other.get().currentIteration())) ||
        // or both are empty
        (this->is_end() && other.is_end());
}
auto StatefulIterator::operator<(StatefulIterator const &) const -> bool
{
    throw error::WrongAPIUsage(
        "Global stateful iterator supports no relative comparison.");
}

StatefulIterator StatefulIterator::end()
{
    return StatefulIterator{};
}

auto StatefulIterator::is_end() const -> bool
{
    return !m_data || !m_data->has_value() ||
        std::visit(
            auxiliary::overloaded{
                [](CurrentStep::Before_t const &) { return false; },
                [](CurrentStep::During_t const &during) {
                    return !during.iteration_idx.has_value();
                },
                [](CurrentStep::After_t const &) { return true; }},
            (**m_data).currentStep);
}

auto StatefulIterator::assert_end_iterator() const -> void
{
    if (!is_end())
    {
        throw error::Internal("Assertion error: Iterator is no end iterator.");
    }
}

StatefulIterator::operator bool() const
{
    return m_data->has_value();
}

LegacyIteratorAdaptor::LegacyIteratorAdaptor(StatefulIterator iterator)
    : m_iterator(std::move(iterator))
{}

auto LegacyIteratorAdaptor::operator*() const -> value_type
{
    return m_iterator.operator*();
}

auto LegacyIteratorAdaptor::operator++() -> LegacyIteratorAdaptor &
{
    ++m_iterator;
    return *this;
}

auto LegacyIteratorAdaptor::end() -> LegacyIteratorAdaptor
{
    return StatefulIterator::end();
}

auto LegacyIteratorAdaptor::operator==(LegacyIteratorAdaptor const &other) const
    -> bool
{
    return m_iterator == other.m_iterator;
};

auto LegacyIteratorAdaptor::operator!=(LegacyIteratorAdaptor const &other) const
    -> bool
{
    return m_iterator != other.m_iterator;
};

ReadIterations::ReadIterations(
    Series series,
    Access access,
    std::optional<internal::ParsePreference> parsePreference)
    : m_series(std::move(series)), m_parsePreference(parsePreference)
{
    auto &data = m_series.get();
    if (access == Access::READ_LINEAR && !data.m_sharedReadIterations)
    {
        // Open the iterator now already, so that metadata may already be read
        data.m_sharedReadIterations = std::make_unique<StatefulIterator>(
            StatefulIterator::tag_read, m_series, m_parsePreference);
    }
}

ReadIterations::iterator_t ReadIterations::begin()
{
    auto &series = m_series.get();
    if (!series.m_sharedReadIterations)
    {
        series.m_sharedReadIterations = std::make_unique<StatefulIterator>(
            StatefulIterator::tag_read, m_series, m_parsePreference);
    }
    return *series.m_sharedReadIterations;
}

ReadIterations::iterator_t ReadIterations::end()
{
    return iterator_t::end();
}
} // namespace openPMD
