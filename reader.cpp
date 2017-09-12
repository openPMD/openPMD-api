#include <iostream>
#include <numeric>

#include "include/Output.hpp"

int main()
{
    Output o = Output("./directory/",
                      "data%T.h5");

    /*
    std::cout << "Read attributes in the root:\n";
    for( auto const& val : o.attributes() )
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
    for( auto const& a : o.iterations.attributes() )
        std::cout << '\t' << a << '\n';
    std::cout << '\n';

    std::cout << "Read iterations in basePath:\n";
    for( auto const& i : o.iterations )
        std::cout << '\t' << i.first << '\n';
    std::cout << '\n';

    for( auto const& i : o.iterations )
    {
        std::cout << "Read attributes in iteration " << i.first << ":\n";
        for( auto const& val : i.second.attributes() )
            std::cout << '\t' << val << '\n';
        std::cout << '\n';

        std::cout << i.first << ".time - " << i.second.time() << '\n'
                  << i.first << ".dt - " << i.second.dt() << '\n'
                  << i.first << ".timeUnitSI - " << i.second.timeUnitSI() << '\n'
                  << '\n';

        std::cout << "Read attributes in meshesPath in iteration " << i.first << ":\n";
        for( auto const& a : i.second.meshes.attributes() )
            std::cout << '\t' << a << '\n';
        std::cout << '\n';

        std::cout << "Read meshes in iteration " << i.first << ":\n";
        for( auto const& m : i.second.meshes )
            std::cout << '\t' << m.first << '\n';
        std::cout << '\n';

        for( auto const& m : i.second.meshes )
        {
            std::cout << "Read attributes for mesh " << m.first << " in iteration " << i.first << ":\n";
            for( auto const& val : m.second.attributes() )
                std::cout << '\t' << val << '\n';
            std::cout << '\n';

            std::string prefix = std::to_string(i.first) + '.' + m.first;
            std::string axisLabels = "";
            for( auto const& val : m.second.axisLabels() )
                axisLabels += val + ", ";
            std::string gridSpacing = "";
            for( auto const& val : m.second.gridSpacing() )
                gridSpacing += std::to_string(val) + ", ";
            std::string gridGlobalOffset = "";
            for( auto const& val : m.second.gridGlobalOffset() )
                gridGlobalOffset += std::to_string(val) + ", ";
            std::string unitDimension = "";
            for( auto const& val : m.second.unitDimension() )
                unitDimension += std::to_string(val) + ", ";
            std::cout << prefix << ".geometry - " << m.second.geometry() << '\n'
                      << prefix << ".dataOrder - " << m.second.dataOrder() << '\n'
                      << prefix << ".axisLabels - " << axisLabels << '\n'
                      << prefix << ".gridSpacing - " << gridSpacing << '\n'
                      << prefix << ".gridGlobalOffset - " << gridGlobalOffset << '\n'
                      << prefix << ".gridUnitSI - " << m.second.gridUnitSI() << '\n'
                      << prefix << ".unitDimension - " << unitDimension << '\n'
                      << prefix << ".timeOffset - " << m.second.timeOffset() << '\n'
                      << '\n';

            std::cout << "Read recordComponents for mesh " << m.first << ":\n";
            for( auto const& rc : m.second )
                std::cout << '\t' << rc.first << '\n';
            std::cout << '\n';

            for( auto const& rc : m.second )
            {
                std::cout << "Read attributes for recordComponent " << rc.first << " for mesh " << m.first << '\n';
                for( auto const& val : rc.second.attributes() )
                    std::cout << '\t' << val << '\n';
                std::cout << '\n';

                std::string prefix = std::to_string(i.first) + '.' + m.first + '.' + rc.first;
                std::string position = "";
                for( auto const& val : rc.second.position< double >() )
                    position += std::to_string(val) + ", ";
                std::cout << prefix << ".unitSI - " << rc.second.unitSI() << '\n'
                          << prefix << ".position - " << position << '\n'
                          << '\n';
            }
        }

        std::cout << "Read attributes in particlesPath in iteration " << i.first << ":\n";
        for( auto const& a : i.second.particles.attributes() )
            std::cout << '\t' << a << '\n';
        std::cout << '\n';

        std::cout << "Read particleSpecies in iteration " << i.first << ":\n";
        for( auto const& val : i.second.particles )
            std::cout << '\t' << val.first << '\n';
        std::cout << '\n';

        for( auto const& p : i.second.particles )
        {
            std::cout << "Read attributes for particle species " << p.first << " in iteration " << i.first << ":\n";
            for( auto const& val : p.second.attributes() )
                std::cout << '\t' << val << '\n';
            std::cout << '\n';

            std::cout << "Read particle records for particle species " << p.first << " in iteration " << i.first << ":\n";
            for( auto const& r : p.second )
                std::cout << '\t' << r.first << '\n';
            std::cout << '\n';

            for( auto const& r : p.second )
            {
                std::cout << "Read recordComponents for particle record " << r.first << ":\n";
                for( auto const& rc : r.second )
                    std::cout << '\t' << rc.first << '\n';
                std::cout << '\n';

                for( auto const& rc : r.second )
                {
                    std::cout << "Read attributes for recordComponent " << rc.first << " for particle record " << r.first << '\n';
                    for( auto const& val : rc.second.attributes() )
                        std::cout << '\t' << val << '\n';
                    std::cout << '\n';
                }
            }
        }
    }
     */

    return 0;
}
