#define BUILD_STREAMING_EXAMPLE false
#if BUILD_STREAMING_EXAMPLE
#include <openPMD/openPMD.hpp>

#include <array>
#include <iostream>
#include <memory>

using std::cout;
using namespace openPMD;

int
run()
{
#if openPMD_HAVE_ADIOS2
    using position_t = double;

    std::string options = R"(
        {
          "adios2": {
            "engine": {
              "type": "sst",
              "parameters": {
                  "OpenTimeoutSecs": "5"
              }
            }
          }
        }
    )";

    // open file for reading
    Series series( "electrons.bp", Access::READ_ONLY, options );

    for( IndexedIteration iteration : series.readIterations() )
    {
        std::cout << "Current iteration: " << iteration.iterationIndex
                  << std::endl;
        Record electronPositions = iteration.particles[ "e" ][ "position" ];
        std::array< std::shared_ptr< position_t >, 3 > loadedChunks;
        std::array< Extent, 3 > extents;
        std::array< std::string, 3 > const dimensions{ { "x", "y", "z" } };

        for( size_t i = 0; i < 3; ++i )
        {
            std::string dim = dimensions[ i ];
            RecordComponent rc = electronPositions[ dim ];
            loadedChunks[ i ] = rc.loadChunk< position_t >(
                Offset( rc.getDimensionality(), 0 ), rc.getExtent() );
            extents[ i ] = rc.getExtent();
        }

        iteration.close();

        for( size_t i = 0; i < 3; ++i )
        {
            std::string dim = dimensions[ i ];
            Extent const & extent = extents[ i ];
            std::cout << "\ndim: " << dim << "\n" << std::endl;
            auto chunk = loadedChunks[ i ];
            for( size_t j = 0; j < extent[ 0 ]; ++j )
            {
                std::cout << chunk.get()[ j ] << ", ";
            }
            std::cout << "\n----------\n" << std::endl;
        }
    }

    return 0;
#else
    std::cout << "The streaming example requires that openPMD has been built "
                 "with ADIOS2."
              << std::endl;
    return 0;
#endif
}

int
main()
{
    try
    {
        run();
    }
    catch( std::runtime_error & e )
    {
        /*
         * This will catch a timeout error if no writer has been found
         */
        std::cout << "Reading end of the streaming example failed:\n"
                  << e.what() << std::endl;
    }
    return 0;
}
#else
int main(){ return 0; }
#endif