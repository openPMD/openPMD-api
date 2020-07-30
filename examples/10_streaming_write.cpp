#include <openPMD/openPMD.hpp>

#include <iostream>
#include <memory>
#include <numeric> // std::iota

using std::cout;
using namespace openPMD;

int
main()
{
#if openPMD_HAVE_ADIOS2
    using position_t = double;

    std::string options = R"(
        {
          "adios2": {
            "engine": {
              "type": "sst"
            }
          }
        }
    )";

    // open file for writing
    Series series = Series( "electrons.bp", Access::CREATE, options );

    Datatype datatype = determineDatatype< position_t >();
    constexpr unsigned long length = 10ul;
    Extent global_extent = { length };
    Dataset dataset = Dataset( datatype, global_extent );
    std::shared_ptr< position_t > local_data(
        new position_t[ length ], []( position_t * ptr ) { delete[] ptr; } );

    WriteIterations iterations = series.writeIterations();
    for( size_t i = 0; i < 100; ++i )
    {
        Iteration iteration = iterations[ i ];
        Record electronPositions = iteration.particles[ "e" ][ "position" ];

        std::iota( local_data.get(), local_data.get() + length, i * length );
        for( auto const & dim : { "x", "y", "z" } )
        {
            RecordComponent pos = electronPositions[ dim ];
            pos.resetDataset( dataset );
            pos.storeChunk( local_data, Offset{ 0 }, global_extent );
        }
        iteration.close();
    }

    return 0;
#else
    std::cout << "The streaming example requires that openPMD has been built "
                 "with ADIOS2."
              << std::endl;
#endif
}
