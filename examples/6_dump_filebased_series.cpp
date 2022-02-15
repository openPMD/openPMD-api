#include <openPMD/openPMD.hpp>

#include <iostream>
#include <numeric>

using namespace openPMD;

int main()
{
    Series o = Series("../samples/git-sample/data%T.h5", Access::READ_ONLY);

    std::cout << "Read iterations ";
    for (auto const &val : o.iterations)
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
    for (auto const &a : o.iterations.attributes())
        std::cout << '\t' << a << '\n';
    std::cout << '\n';

    std::cout << "Read iterations in basePath:\n";
    for (auto const &i : o.iterations)
        std::cout << '\t' << i.first << '\n';
    std::cout << '\n';

    for (auto const &i : o.iterations)
    {
        std::cout << "Read attributes in iteration " << i.first << ":\n";
        for (auto const &val : i.second.attributes())
            std::cout << '\t' << val << '\n';
        std::cout << '\n';

        std::cout << i.first << ".time - " << i.second.time<float>() << '\n'
                  << i.first << ".dt - " << i.second.dt<float>() << '\n'
                  << i.first << ".timeUnitSI - " << i.second.timeUnitSI()
                  << '\n'
                  << '\n';

        std::cout << "Read attributes in meshesPath in iteration " << i.first
                  << ":\n";
        for (auto const &a : i.second.meshes.attributes())
            std::cout << '\t' << a << '\n';
        std::cout << '\n';

        std::cout << "Read meshes in iteration " << i.first << ":\n";
        for (auto const &m : i.second.meshes)
            std::cout << '\t' << m.first << '\n';
        std::cout << '\n';

        for (auto const &m : i.second.meshes)
        {
            std::cout << "Read attributes for mesh " << m.first
                      << " in iteration " << i.first << ":\n";
            for (auto const &val : m.second.attributes())
                std::cout << '\t' << val << '\n';
            std::cout << '\n';

            std::string meshPrefix = std::to_string(i.first) + '.' + m.first;
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
                    std::to_string(i.first) + '.' + m.first + '.' + rc.first;
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

        std::cout << "Read attributes in particlesPath in iteration " << i.first
                  << ":\n";
        for (auto const &a : i.second.particles.attributes())
            std::cout << '\t' << a << '\n';
        std::cout << '\n';

        std::cout << "Read particleSpecies in iteration " << i.first << ":\n";
        for (auto const &val : i.second.particles)
            std::cout << '\t' << val.first << '\n';
        std::cout << '\n';

        for (auto const &p : i.second.particles)
        {
            std::cout << "Read attributes for particle species " << p.first
                      << " in iteration " << i.first << ":\n";
            for (auto const &val : p.second.attributes())
                std::cout << '\t' << val << '\n';
            std::cout << '\n';

            std::cout << "Read particle records for particle species "
                      << p.first << " in iteration " << i.first << ":\n";
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
    }

    /* The files in 'o' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
     */
    return 0;
}
