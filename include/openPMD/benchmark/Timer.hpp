/* Copyright 2020-2021 Junmin Gu
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

#include "MemoryProfiler.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

namespace openPMD
{
namespace benchmark
{
    /** The Timer class for profiling purpose
     *
     *  Simple Timer that measures time consumption btw constructor and
     * destructor Reports at rank 0 at the console, for immediate convenience
     */
    class Timer
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = std::chrono::time_point<Clock>;

        /** Simple Timer
         *
         * @param tag       item name to measure
         * @param rank      MPI rank
         * @param progStart time point at program start
         */
        Timer(const std::string &tag, int rank, TimePoint progStart)
            : m_ProgStart(progStart)
            , m_Start(std::chrono::system_clock::now())
            , m_Tag(tag)
            , m_Rank(rank)
        {
            MemoryProfiler(rank, tag);
        }

        ~Timer()
        {
            std::string tt = "~" + m_Tag;
            MemoryProfiler(m_Rank, tt.c_str());
            m_End = Clock::now();

            double millis =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    m_End - m_Start)
                    .count();
            double secs = millis / 1000.0;
            if (m_Rank > 0)
                return;

            std::cout << "  [" << m_Tag << "] took:" << secs << " seconds\n";
            std::cout << "     " << m_Tag << "  From ProgStart in seconds "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(
                             m_End - m_ProgStart)
                             .count() /
                    1000.0
                      << std::endl;

            std::cout << std::endl;
        }

    private:
        TimePoint m_ProgStart;
        TimePoint m_Start;
        TimePoint m_End;

        std::string m_Tag;
        int m_Rank = 0;
    };
} // namespace benchmark
} // namespace openPMD
