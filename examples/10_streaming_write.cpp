#define BUILD_STREAMING_EXAMPLE false
#if BUILD_STREAMING_EXAMPLE
#include <openPMD/openPMD.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric> // std::iota
#include <thread>

#include <condition_variable>

using std::cout;
using namespace openPMD;

enum class timeoutStatus
{
    running,
    timeout,
    finished
};

struct timeout
{
    timeoutStatus status{ timeoutStatus::running };
    std::condition_variable condvar;
    std::mutex mutex;

    bool
    wait()
    {
        std::unique_lock< std::mutex > lk( mutex );
        condvar.wait(
            lk, [ this ] { return status != timeoutStatus::running; } );
        return status == timeoutStatus::finished;
    }

    void
    success()
    {
        {
            std::unique_lock< std::mutex > lk( mutex );
            status = timeoutStatus::finished;
        }
        condvar.notify_one();
    }

    void
    cancel()
    {
        {
            std::unique_lock< std::mutex > lk( mutex );
            if( status == timeoutStatus::running )
            {
                status = timeoutStatus::timeout;
            }
        }
        condvar.notify_one();
    }
};

int
run( timeout & timeout )
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
    Series series( "electrons.bp", Access::CREATE, options );

    Datatype datatype = determineDatatype< position_t >();
    constexpr unsigned long length = 10ul;
    Extent global_extent = { length };
    Dataset dataset = Dataset( datatype, global_extent );
    std::shared_ptr< position_t > local_data(
        new position_t[ length ],
        []( position_t const * ptr ) { delete[] ptr; } );

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
        // a reader has been found, let's report back to timeout thing
        timeout.success();
    }

    return 0;
#else
    timeout.success();
    std::cout << "The streaming example requires that openPMD has been built "
                 "with ADIOS2."
              << std::endl;
    return 0;
#endif
}

int
main()
{
    /*
     * ADIOS2 SST engine does not support timeouts for the writing end,
     * let's do it manually for now.
     */
    timeout to;
    std::thread worker( &run, std::ref( to ) );
    std::thread timer( [ &to ] {
        std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
        to.cancel();
    } );

    if( !to.wait() )
    {
        std::cerr << "No reader has been found within five seconds."
                  << std::endl;
        exit( 0 );
    }
    worker.join();
    timer.join();
    return 0;
}
#else
int main(){ return 0; }
#endif