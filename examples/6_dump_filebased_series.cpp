/* Copyright 2025 Axel Huebl, Fabian Koller, Franz Poeschel
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
#include <openPMD/openPMD.hpp>

#include <iostream>
#include <numeric>

using namespace openPMD;

int main()
{
    Series o = Series(
        "../samples/git-sample/data%T.h5",
        Access::READ_RANDOM_ACCESS,
        R"({"defer_iteration_parsing": true})");

    std::cout << "Read iterations ";
    for (auto const &val : o.snapshots())
        std::cout << '\t' << val.first;
    std::cout << "Read attributes in the root:\n";
    for (auto const &val : o.attributes())
        std::cout << '\t' << val << '\n';
    std::cout << '\n';

    std::cout << "basePath - " << o.basePath() << '\n'
              << "iterationEncoding - " << o.iterationEncoding() << '\n'
              << "iterationFormat - " << o.iterationFormat() << '\n'
              << "meshesPath - " << o.meshesPath() << '\n'
              << "openPMD - " << o.openPMD() << '\n'
              << "openPMDextension - " << o.openPMDextension() << '\n'
              << "particlesPath - " << o.particlesPath() << '\n'
              << '\n';

    std::cout << "Read attributes in basePath:\n";
    for (auto const &a : o.snapshots().attributes())
        std::cout << '\t' << a << '\n';
    std::cout << '\n';

    std::cout << "Read iterations in basePath:\n";
    for (auto const &i : o.snapshots())
        std::cout << '\t' << i.first << '\n';
    std::cout << '\n';

    for (auto &[index, i] : o.snapshots())
    {
        // with defer_iteration_parsing, open() must be called explicitly
        i.open();
        std::cout << "Read attributes in iteration " << index << ":\n";
        for (auto const &val : i.attributes())
            std::cout << '\t' << val << '\n';
        std::cout << '\n';

        std::cout << index << ".time - " << i.time<float>() << '\n'
                  << index << ".dt - " << i.dt<float>() << '\n'
                  << index << ".timeUnitSI - " << i.timeUnitSI() << '\n'
                  << '\n';

        std::cout << "Read attributes in meshesPath in iteration " << index
                  << ":\n";
        for (auto const &a : i.meshes.attributes())
            std::cout << '\t' << a << '\n';
        std::cout << '\n';

        std::cout << "Read meshes in iteration " << index << ":\n";
        for (auto const &m : i.meshes)
            std::cout << '\t' << m.first << '\n';
        std::cout << '\n';

        for (auto const &m : i.meshes)
        {
            std::cout << "Read attributes for mesh " << m.first
                      << " in iteration " << index << ":\n";
            for (auto const &val : m.second.attributes())
                std::cout << '\t' << val << '\n';
            std::cout << '\n';

            std::string meshPrefix = std::to_string(index) + '.' + m.first;
            std::string axisLabels = "";
            for (auto const &val : m.second.axisLabels())
                axisLabels += val + ", ";
            std::string gridSpacing = "";
            for (auto const &val : m.second.gridSpacing<float>())
                gridSpacing += std::to_string(val) + ", ";
            std::string gridGlobalOffset = "";
            for (auto const &val : m.second.gridGlobalOffset())
                gridGlobalOffset += std::to_string(val) + ", ";
            std::string unitDimension = "";
            for (auto const &val : m.second.unitDimension())
                unitDimension += std::to_string(val) + ", ";
            std::cout << meshPrefix << ".geometry - " << m.second.geometry()
                      << '\n'
                      << meshPrefix << ".dataOrder - " << m.second.dataOrder()
                      << '\n'
                      << meshPrefix << ".axisLabels - " << axisLabels << '\n'
                      << meshPrefix << ".gridSpacing - " << gridSpacing << '\n'
                      << meshPrefix << ".gridGlobalOffset - "
                      << gridGlobalOffset << '\n'
                      << meshPrefix << ".gridUnitSI - " << m.second.gridUnitSI()
                      << '\n'
                      << meshPrefix << ".unitDimension - " << unitDimension
                      << '\n'
                      << meshPrefix << ".timeOffset - "
                      << m.second.timeOffset<float>() << '\n'
                      << '\n';

            std::cout << "Read recordComponents for mesh " << m.first << ":\n";
            for (auto const &rc : m.second)
                std::cout << '\t' << rc.first << '\n';
            std::cout << '\n';

            for (auto const &rc : m.second)
            {
                std::cout << "Read attributes for recordComponent " << rc.first
                          << " for mesh " << m.first << '\n';
                for (auto const &val : rc.second.attributes())
                    std::cout << '\t' << val << '\n';
                std::cout << '\n';

                std::string componentPrefix =
                    std::to_string(index) + '.' + m.first + '.' + rc.first;
                std::string position = "";
                for (auto const &val : rc.second.position<double>())
                    position += std::to_string(val) + ", ";
                std::cout << componentPrefix << ".unitSI - "
                          << rc.second.unitSI() << '\n'
                          << componentPrefix << ".position - " << position
                          << '\n'
                          << '\n';
            }
        }

        std::cout << "Read attributes in particlesPath in iteration " << index
                  << ":\n";
        for (auto const &a : i.particles.attributes())
            std::cout << '\t' << a << '\n';
        std::cout << '\n';

        std::cout << "Read particleSpecies in iteration " << index << ":\n";
        for (auto const &val : i.particles)
            std::cout << '\t' << val.first << '\n';
        std::cout << '\n';

        for (auto const &p : i.particles)
        {
            std::cout << "Read attributes for particle species " << p.first
                      << " in iteration " << index << ":\n";
            for (auto const &val : p.second.attributes())
                std::cout << '\t' << val << '\n';
            std::cout << '\n';

            std::cout << "Read particle records for particle species "
                      << p.first << " in iteration " << index << ":\n";
            for (auto const &r : p.second)
                std::cout << '\t' << r.first << '\n';
            std::cout << '\n';

            for (auto const &r : p.second)
            {
                std::cout << "Read recordComponents for particle record "
                          << r.first << ":\n";
                for (auto const &rc : r.second)
                    std::cout << '\t' << rc.first << '\n';
                std::cout << '\n';

                for (auto const &rc : r.second)
                {
                    std::cout << "Read attributes for recordComponent "
                              << rc.first << " for particle record " << r.first
                              << '\n';
                    for (auto const &val : rc.second.attributes())
                        std::cout << '\t' << val << '\n';
                    std::cout << '\n';
                }
            }
        }

        // The iteration can be closed in order to help free up resources.
        // The iteration's content will be flushed automatically.
        i.close();
    }

    /* The files in 'o' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
     * Alternatively, one can call `series.close()` to the same effect as
     * calling the destructor, including the release of file handles.
     */
    return 0;
}
