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

#include <chrono>
#include <fstream>
#include <iostream>

namespace openPMD
{
namespace benchmark
{
    /** The Memory profiler class for profiling purpose
     *
     *  Simple Memory usage report that works on linux system
     */
    class MemoryProfiler
    {
    public:
        /** Simple Memory profiler for linux
         *
         * @param[in] rank     MPI rank
         * @param[in] tag      item name to measure
         */
        MemoryProfiler(int rank, const std::string &tag)
            : m_Rank(rank), m_Name("")
        {
#if defined(__linux)
            // m_Name = "/proc/meminfo";
            m_Name = "/proc/self/status";
            Display(tag);
#endif
        }

        /** Display virtual memory info
         *
         * Read from /proc/self/status and display the Virtual Memory info at
         * rank 0 to stdout
         *
         * @param tag      item name to measure
         */
        void Display(const std::string &tag)
        {
            if (0 == m_Name.size())
                return;

            if (m_Rank > 0)
                return;

            std::cout << " memory at:  " << tag;
            std::ifstream input(m_Name.c_str());

            if (input.is_open())
            {
                for (std::string line; getline(input, line);)
                {
                    if (line.find("VmRSS") == 0)
                        std::cout << line << " ";
                    if (line.find("VmSize") == 0)
                        std::cout << line << " ";
                    if (line.find("VmSwap") == 0)
                        std::cout << line;
                }
                std::cout << std::endl;
                input.close();
            }
        }

    private:
        int m_Rank;
        std::string m_Name;
    };
} // namespace benchmark
} // namespace openPMD
