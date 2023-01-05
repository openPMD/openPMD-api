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

#include "openPMD/ReadIterations.hpp"

#include "openPMD/Series.hpp"

#include <iostream>

namespace openPMD
{

SeriesIterator::SeriesIterator() = default;

void SeriesIterator::initSeriesInLinearReadMode()
{
    auto &data = *m_data;
    auto &series = *data.series;
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
            fOpen.encoding = series.iterationEncoding();
            series.IOHandler()->enqueue(IOTask(&series, fOpen));
            series.IOHandler()->flush(internal::defaultFlushParams);
            switch (*fOpen.out_parsePreference)
            {
                using PP = Parameter<Operation::OPEN_FILE>::ParsePreference;
            case PP::PerStep:
                series.advance(AdvanceMode::BEGINSTEP);
                series.readGorVBased(
                    /* do_always_throw_errors = */ false, /* init = */ true);
                break;
            case PP::UpFront:
                series.readGorVBased(
                    /* do_always_throw_errors = */ false, /* init = */ true);
                series.advance(AdvanceMode::BEGINSTEP);
                break;
            }
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

SeriesIterator::SeriesIterator(Series series_in)
    : m_data{std::make_shared<SharedData>()}
{
    auto &data = *m_data;
    data.series = std::move(series_in);
    auto &series = data.series.value();
    if (series.IOHandler()->m_frontendAccess == Access::READ_LINEAR &&
        series.iterations.empty())
    {
        initSeriesInLinearReadMode();
    }

    auto it = series.get().iterations.begin();
    if (it == series.get().iterations.end())
    {
        *this = end();
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
            std::tie(status, availableIterations) =
                it->second.beginStep(/* reread = */ true);
            /*
             * In random-access mode, do not use the information read in the
             * `snapshot` attribute, instead simply go through iterations
             * one by one in ascending order (fallback implementation in the
             * second if branch).
             */
            if (availableIterations.has_value() &&
                status != AdvanceStatus::RANDOMACCESS)
            {
                data.iterationsInCurrentStep = availableIterations.value();
                if (!data.iterationsInCurrentStep.empty())
                {
                    openIteration(series.iterations.at(
                        data.iterationsInCurrentStep.at(0)));
                }
            }
            else if (!series.iterations.empty())
            {
                /*
                 * Fallback implementation: Assume that each step corresponds
                 * with an iteration in ascending order.
                 */
                data.iterationsInCurrentStep = {
                    series.iterations.begin()->first};
                openIteration(series.iterations.begin()->second);
            }
            else
            {
                // this is a no-op, but let's keep it explicit
                data.iterationsInCurrentStep = {};
            }

            break;
        }
        }

        if (status == AdvanceStatus::OVER)
        {
            *this = end();
            return;
        }
        if (!setCurrentIteration())
        {
            *this = end();
            return;
        }
        it->second.setStepStatus(StepStatus::DuringStep);
    }
}

std::optional<SeriesIterator *> SeriesIterator::nextIterationInStep()
{
    auto &data = *m_data;
    using ret_t = std::optional<SeriesIterator *>;

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
    auto &series = data.series.value();

    switch (series.iterationEncoding())
    {
    case IterationEncoding::groupBased:
    case IterationEncoding::variableBased: {
        auto begin = series.iterations.find(oldIterationIndex);
        auto end = begin;
        ++end;
        series.flush_impl(
            begin,
            end,
            {FlushLevel::UserFlush},
            /* flushIOHandler = */ true);

        try
        {
            series.iterations[data.currentIteration].open();
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read iteration '" << data.currentIteration
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
            series.iterations[data.currentIteration].open();
            /*
             * Errors in here might appear due to reparsing after opening a
             * new step.
             */
            series.iterations[data.currentIteration].beginStep(
                /* reread = */ true);
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "[SeriesIterator] Cannot read iteration due to error "
                         "below, will skip it.\n"
                      << err.what() << std::endl;
            return nextIterationInStep();
        }

        return {this};
    }
    throw std::runtime_error("Unreachable!");
}

std::optional<SeriesIterator *> SeriesIterator::nextStep(size_t recursion_depth)
{
    auto &data = *m_data;
    // since we are in group-based iteration layout, it does not
    // matter which iteration we begin a step upon
    AdvanceStatus status{};
    Iteration::BeginStepStatus::AvailableIterations_t availableIterations;
    try
    {
        std::tie(status, availableIterations) =
            Iteration::beginStep({}, *data.series, /* reread = */ true);
    }
    catch (error::ReadError const &err)
    {
        std::cerr << "[SeriesIterator] Cannot read iteration due to error "
                     "below, will skip it.\n"
                  << err.what() << std::endl;
        data.series->advance(AdvanceMode::ENDSTEP);
        return nextStep(recursion_depth + 1);
    }

    if (availableIterations.has_value() &&
        status != AdvanceStatus::RANDOMACCESS)
    {
        data.iterationsInCurrentStep = availableIterations.value();
    }
    else
    {
        /*
         * Fallback implementation: Assume that each step corresponds
         * with an iteration in ascending order.
         */
        auto &series = data.series.value();
        auto it = series.iterations.find(data.currentIteration);
        auto itEnd = series.iterations.end();
        if (it == itEnd)
        {
            if (status == AdvanceStatus::RANDOMACCESS ||
                status == AdvanceStatus::OVER)
            {
                *this = end();
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
                data.series->advance(AdvanceMode::ENDSTEP);
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
                    *this = end();
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
                    data.series->advance(AdvanceMode::ENDSTEP);
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
        *this = end();
        return {this};
    }

    return {this};
}

std::optional<SeriesIterator *> SeriesIterator::loopBody()
{
    auto &data = *m_data;
    Series &series = data.series.value();
    auto &iterations = series.iterations;

    /*
     * Might not be present because parsing might have failed in previous step
     */
    if (iterations.contains(data.currentIteration))
    {
        auto &currentIteration = iterations[data.currentIteration];
        if (!currentIteration.closed())
        {
            currentIteration.close();
        }
    }

    auto guardReturn =
        [&iterations](
            auto const &option) -> std::optional<openPMD::SeriesIterator *> {
        if (!option.has_value() || *option.value() == end())
        {
            return option;
        }
        auto currentIterationIndex = option.value()->peekCurrentIteration();
        if (!currentIterationIndex.has_value())
        {
            return std::nullopt;
        }
        auto iteration = iterations.at(currentIterationIndex.value());
        if (iteration.get().m_closed != internal::CloseStatus::ClosedInBackend)
        {
            try
            {
                iteration.open();
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
        *this = end();
        return {this};
    }

    auto option = nextStep(/*recursion_depth = */ 1);
    return guardReturn(option);
}

void SeriesIterator::deactivateDeadIteration(iteration_index_t index)
{
    auto &data = *m_data;
    switch (data.series->iterationEncoding())
    {
    case IterationEncoding::fileBased: {
        Parameter<Operation::CLOSE_FILE> param;
        data.series->IOHandler()->enqueue(
            IOTask(&data.series->iterations[index], std::move(param)));
        data.series->IOHandler()->flush({FlushLevel::UserFlush});
    }
    break;
    case IterationEncoding::variableBased:
    case IterationEncoding::groupBased: {
        Parameter<Operation::ADVANCE> param;
        param.mode = AdvanceMode::ENDSTEP;
        data.series->IOHandler()->enqueue(
            IOTask(&data.series->iterations[index], std::move(param)));
        data.series->IOHandler()->flush({FlushLevel::UserFlush});
    }
    break;
    }
    data.series->iterations.container().erase(index);
}

SeriesIterator &SeriesIterator::operator++()
{
    auto &data = *m_data;
    if (!data.series.has_value())
    {
        *this = end();
        return *this;
    }
    std::optional<SeriesIterator *> res;
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
        (**resvalue).setStepStatus(StepStatus::DuringStep);
    }
    return *resvalue;
}

IndexedIteration SeriesIterator::operator*()
{
    auto &data = *m_data;
    return IndexedIteration(
        data.series.value().iterations[data.currentIteration],
        data.currentIteration);
}

bool SeriesIterator::operator==(SeriesIterator const &other) const
{
    return (this->m_data.operator bool() && other.m_data.operator bool() &&
            (this->m_data->currentIteration ==
             other.m_data->currentIteration)) ||
        (!this->m_data.operator bool() && !other.m_data.operator bool());
}

bool SeriesIterator::operator!=(SeriesIterator const &other) const
{
    return !operator==(other);
}

SeriesIterator SeriesIterator::end()
{
    return SeriesIterator{};
}

ReadIterations::ReadIterations(Series series, Access access)
    : m_series(std::move(series))
{
    if (access == Access::READ_LINEAR)
    {
        // Open the iterator now already, so that metadata may already be read
        alreadyOpened = iterator_t{m_series};
    }
}

ReadIterations::iterator_t ReadIterations::begin()
{
    if (!alreadyOpened.has_value())
    {
        alreadyOpened = iterator_t{m_series};
    }
    return alreadyOpened.value();
}

ReadIterations::iterator_t ReadIterations::end()
{
    return SeriesIterator::end();
}
} // namespace openPMD
