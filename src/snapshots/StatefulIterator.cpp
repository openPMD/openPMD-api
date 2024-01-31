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

#include "openPMD/Series.hpp"

#include <iostream>
#include <optional>
#include <stdexcept>

namespace openPMD
{
StatefulIterator::SharedData::~SharedData()
{
    if (auto IOHandler = series.IOHandler(); currentIteration.has_value() &&
        IOHandler && IOHandler->m_lastFlushSuccessful)
    {
        auto lastIterationIndex = currentIteration.value();
        auto &lastIteration = series.iterations.at(lastIterationIndex);
        if (!lastIteration.closed())
        {
            lastIteration.close();
        }
    }
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
            series.readFileBased();
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
                    /* do_always_throw_errors = */ false, /* init = */ true);
                break;
            case PP::UpFront:
                series.readGorVBased(
                    /* do_always_throw_errors = */ false, /* init = */ true);
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

auto StatefulIterator::setCurrentIteration() -> bool
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

auto StatefulIterator::peekCurrentIteration() -> std::optional<uint64_t>
{
    if (!m_data || !m_data->has_value())
    {
        return std::nullopt;
    }
    auto &data = m_data->value();
    if (data.iterationsInCurrentStep.empty())
    {
        return std::nullopt;
    }
    else
    {
        return {*data.iterationsInCurrentStep.begin()};
    }
}
auto StatefulIterator::peekCurrentlyOpenIteration() const
    -> std::optional<value_type const *>
{
    if (!m_data || !m_data->has_value())
    {
        return std::nullopt;
    }
    auto &s = m_data->value();
    if (!s.currentIteration.has_value())
    {
        return std::nullopt;
    }
    // Iteration &currentIteration =
    // s.series.iterations.at(*s.currentIteration);
    auto currentIteration = s.series.iterations.find(*s.currentIteration);
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

    auto it = series.get().iterations.begin();
    if (it == series.get().iterations.end())
    {
        this->close();
        return;
    }
    else if (
        it->second.get().m_closed == internal::CloseStatus::ClosedInBackend)
    {
        throw error::WrongAPIUsage(
            "Trying to call Series::readIterations() on a (partially) read "
            "Series.");
    }
    else
    {
        auto openIteration = [](Iteration &iteration) {
            /*
             * @todo
             * Is that really clean?
             * Use case: See Python ApiTest testListSeries:
             * Call listSeries twice.
             */
            if (iteration.get().m_closed !=
                internal::CloseStatus::ClosedInBackend)
            {
                iteration.open();
            }
        };
        AdvanceStatus status{};
        switch (series.iterationEncoding())
        {
        case IterationEncoding::fileBased:
            /*
             * The file needs to be accessed before beginning a step upon it.
             * In file-based iteration layout it maybe is not accessed yet,
             * so do that now. There is only one step per file, so beginning
             * the step after parsing the file is ok.
             */

            openIteration(series.iterations.begin()->second);
            status = it->second.beginStep(/* reread = */ true);
            for (auto const &pair : series.iterations)
            {
                data.iterationsInCurrentStep.push_back(pair.first);
            }
            break;
        case IterationEncoding::groupBased:
        case IterationEncoding::variableBased: {
            /*
             * In group-based iteration layout, we have definitely already had
             * access to the file until now. Better to begin a step right away,
             * otherwise we might get another step's data.
             */
            Iteration::BeginStepStatus::AvailableIterations_t
                availableIterations;
            std::tie(status, availableIterations) = it->second.beginStep(
                /* reread = */ reread(data.parsePreference));
            /*
             * In random-access mode, do not use the information read in the
             * `snapshot` attribute, instead simply go through iterations
             * one by one in ascending order (fallback implementation in the
             * second if branch).
             */
            data.iterationsInCurrentStep = availableIterations;
            if (!data.iterationsInCurrentStep.empty())
            {
                openIteration(
                    series.iterations.at(data.iterationsInCurrentStep.at(0)));
            }
            break;
        }
        }

        if (status == AdvanceStatus::OVER)
        {
            this->close();
            return;
        }
        if (!setCurrentIteration())
        {
            this->close();
            return;
        }
        it->second.setStepStatus(StepStatus::DuringStep);
    }
}

std::optional<StatefulIterator *> StatefulIterator::nextIterationInStep()
{
    auto &data = get();
    using ret_t = std::optional<StatefulIterator *>;

    if (data.iterationsInCurrentStep.empty())
    {
        return ret_t{};
    }
    data.iterationsInCurrentStep.pop_front();
    if (data.iterationsInCurrentStep.empty())
    {
        return ret_t{};
    }
    auto oldIterationIndex = data.currentIteration;
    data.currentIteration = *data.iterationsInCurrentStep.begin();
    auto &series = data.series;

    switch (series.iterationEncoding())
    {
    case IterationEncoding::groupBased:
    case IterationEncoding::variableBased: {
        if (oldIterationIndex.has_value())
        {
            auto begin = series.iterations.find(*oldIterationIndex);
            auto end = begin;
            ++end;
            series.flush_impl(
                begin,
                end,
                {FlushLevel::UserFlush},
                /* flushIOHandler = */ true);
        }
        try
        {
            series.iterations[*data.currentIteration].open();
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read iteration '" << *data.currentIteration
                      << "' and will skip it due to read error:\n"
                      << err.what() << std::endl;
            return nextIterationInStep();
        }

        return {this};
    }
    case IterationEncoding::fileBased:
        try
        {
            /*
             * Errors in here might appear due to deferred iteration parsing.
             */
            series.iterations[*data.currentIteration].open();
            /*
             * Errors in here might appear due to reparsing after opening a
             * new step.
             */
            series.iterations[*data.currentIteration].beginStep(
                /* reread = */ true);
        }
        catch (error::ReadError const &err)
        {
            std::cerr
                << "[StatefulIterator] Cannot read iteration due to error "
                   "below, will skip it.\n"
                << err.what() << std::endl;
            return nextIterationInStep();
        }

        return {this};
    }
    throw std::runtime_error("Unreachable!");
}

std::optional<StatefulIterator *>
StatefulIterator::nextStep(size_t recursion_depth)
{
    auto &data = get();
    // since we are in group-based iteration layout, it does not
    // matter which iteration we begin a step upon
    AdvanceStatus status{};
    Iteration::BeginStepStatus::AvailableIterations_t availableIterations;
    try
    {
        std::tie(status, availableIterations) = Iteration::beginStep(
            {},
            data.series,
            /* reread = */ reread(data.parsePreference),
            data.ignoreIterations);
    }
    catch (error::ReadError const &err)
    {
        std::cerr << "[StatefulIterator] Cannot read iteration due to error "
                     "below, will skip it.\n"
                  << err.what() << std::endl;
        data.series.advance(AdvanceMode::ENDSTEP);
        return nextStep(recursion_depth + 1);
    }

    if (status != AdvanceStatus::RANDOMACCESS)
    {
        data.iterationsInCurrentStep = availableIterations;
    }
    else
    {
        /*
         * Fallback implementation: Assume that each step corresponds
         * with an iteration in ascending order.
         */
        if (!data.currentIteration.has_value())
        {
            throw std::runtime_error(
                "CATASTROPHE. need to think how to resolve this one.");
        }
        auto &series = data.series;
        auto it = series.iterations.find(*data.currentIteration);
        auto itEnd = series.iterations.end();
        if (it == itEnd)
        {
            if (status == AdvanceStatus::RANDOMACCESS ||
                status == AdvanceStatus::OVER)
            {
                this->close();
                return {this};
            }
            else
            {
                /*
                 * Stream still going but there was no iteration found in the
                 * current IO step?
                 * Might be a duplicate iteration resulting from appending,
                 * will skip such iterations and hope to find something in a
                 * later IO step. No need to finish right now.
                 */
                data.iterationsInCurrentStep = {};
            }
        }
        else
        {
            for (size_t i = 0; i < recursion_depth && it != itEnd; ++i)
            {
                ++it;
            }

            if (it == itEnd)
            {
                if (status == AdvanceStatus::RANDOMACCESS ||
                    status == AdvanceStatus::OVER)
                {
                    this->close();
                    return {this};
                }
                else
                {
                    /*
                     * Stream still going but there was no iteration found in
                     * the current IO step? Might be a duplicate iteration
                     * resulting from appending, will skip such iterations and
                     * hope to find something in a later IO step. No need to
                     * finish right now.
                     */
                    data.iterationsInCurrentStep = {};
                }
            }
            else
            {
                data.iterationsInCurrentStep = {it->first};
            }
        }
    }

    if (status == AdvanceStatus::OVER)
    {
        this->close();
        return {this};
    }

    return {this};
}

std::optional<StatefulIterator *> StatefulIterator::loopBody()
{
    auto &data = get();
    Series &series = data.series;
    auto &iterations = series.iterations;

    /*
     * Might not be present because parsing might have failed in previous step
     */
    if (data.currentIteration.has_value() &&
        iterations.contains(*data.currentIteration))
    {
        auto &currentIteration = iterations[*data.currentIteration];
        if (!currentIteration.closed())
        {
            currentIteration.close();
        }
    }

    auto guardReturn =
        [&series, &iterations](
            auto const &option) -> std::optional<openPMD::StatefulIterator *> {
        if (!option.has_value() || *option.value() == end())
        {
            return option;
        }
        auto currentIterationIndex = option.value()->peekCurrentIteration();
        if (!currentIterationIndex.has_value())
        {
            series.advance(AdvanceMode::ENDSTEP);
            return std::nullopt;
        }
        // If we had the iteration already, then it's either not there at all
        // (because old iterations are deleted in linear access mode),
        // or it's still there but closed in random-access mode
        auto index = currentIterationIndex.value();

        if (iterations.contains(index))
        {
            auto iteration = iterations.at(index);
            if (iteration.get().m_closed !=
                internal::CloseStatus::ClosedInBackend)
            {
                try
                {
                    iterations.at(index).open();
                    option.value()->setCurrentIteration();
                    return option;
                }
                catch (error::ReadError const &err)
                {
                    std::cerr << "Cannot read iteration '"
                              << currentIterationIndex.value()
                              << "' and will skip it due to read error:\n"
                              << err.what() << std::endl;
                    option.value()->deactivateDeadIteration(
                        currentIterationIndex.value());
                    return std::nullopt;
                }
            }
            else
            {
                // we had this iteration already, skip it
                iteration.endStep();
                return std::nullopt; // empty, go into next iteration
            }
        }
        else
        {
            // we had this iteration already, skip it
            series.advance(AdvanceMode::ENDSTEP);
            return std::nullopt;
        }
    };

    {
        auto optionallyAStep = nextIterationInStep();
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
        this->close();
        return {this};
    }

    auto option = nextStep(/*recursion_depth = */ 1);
    return guardReturn(option);
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
    auto &data = get();
    auto oldIterationIndex = data.currentIteration;
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
        res = loopBody();
    } while (!res.has_value());

    auto resvalue = res.value();
    if (*resvalue != end())
    {
        auto &series = data.series;
        auto index = data.currentIteration;
        auto &iteration = series.iterations[index.value()];
        iteration.setStepStatus(StepStatus::DuringStep);

        if (series.IOHandler()->m_frontendAccess == Access::READ_LINEAR &&
            oldIterationIndex.has_value())
        {
            /*
             * Linear read mode: Any data outside the current iteration is
             * inaccessible. Delete the iteration. This has two effects:
             *
             * 1) Avoid confusion.
             * 2) Avoid memory buildup in long-running workflows with many
             *    iterations.
             *
             * @todo Also delete data in the backends upon doing this.
             */
            auto &container = series.iterations.container();
            container.erase(*oldIterationIndex);
            data.ignoreIterations.emplace(*oldIterationIndex);
        }
    }
    return *resvalue;
}

auto StatefulIterator::operator*() const -> value_type const &
{
    auto &data = get();
    if (data.currentIteration.has_value())
    {

        auto iterator = static_cast<Series::IterationsContainer_t const &>(
                            data.series.iterations)
                            .find(*data.currentIteration);
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
         (this->get().currentIteration == other.get().currentIteration)) ||
        // or both are empty
        (!this->m_data->has_value() && !other.m_data->has_value());
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
