#include <iostream>

#include "include/Output.hpp"

int main()
{
    Output o = Output("./working/directory/",
                      "3D_simData.h5",
                      Output::IterationEncoding::groupBased,
                      Format::HDF5,
                      AccessType::READ_ONLY);

    for( auto const& val : o.attributes() )
        std::cout << val << std::endl;

    return 0;
}
