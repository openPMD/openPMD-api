#include <iostream>

#include "include/Output.hpp"

int main()
{
    Output o = Output("./working/directory/",
                      "3D_simData.h5",
                      Output::IterationEncoding::groupBased,
                      Format::HDF5,
                      AccessType::READ_ONLY);

    std::cout << "Read attributes in the root:\n";
    for( auto const& val : o.attributes() )
        std::cout << '\t' << val << '\n';
    std::cout << '\n';

    /*
    std::cout << "basePath - " << o.basePath() << '\n'
              << "iterationEncoding - " << o.iterationEncoding() << '\n'
              << "iterationFormat - " << o.iterationFormat() << '\n'
              << "meshesPath - " << o.meshesPath() << '\n'
              << "openPMD - " << o.openPMD() << '\n'
              << "openPMDextension - " << o.openPMDextension() << '\n'
              << "particlesPath - " << o.particlesPath() << '\n';
              */

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
    }

    return 0;
}
