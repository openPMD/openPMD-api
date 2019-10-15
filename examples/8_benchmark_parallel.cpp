#include <openPMD/openPMD.hpp>
#include <openPMD/benchmark/mpi/MPIBenchmark.hpp>
#include <openPMD/benchmark/mpi/RandomDatasetFiller.hpp>
#include <openPMD/benchmark/mpi/OneDimensionalBlockSlicer.hpp>

#if openPMD_HAVE_MPI
#   include <mpi.h>
#endif

#include <iostream>


#if openPMD_HAVE_MPI
int main(
    int argc,
    char *argv[]
)
{
    using namespace std;
    MPI_Init(
        &argc,
        &argv
    );

    // For simplicity, use only one datatype in this benchmark.
    // Note that a single Benchmark object can be used to configure
    // multiple different benchmark runs with different datatypes,
    // given that you provide it with an appropriate DatasetFillerProvider
    // (template parameter of the Benchmark class).
    using type = long int;
#if openPMD_HAVE_ADIOS1 || openPMD_HAVE_ADIOS2 || openPMD_HAVE_HDF5
    openPMD::Datatype dt = openPMD::determineDatatype<type>();
#endif

    // Total (in this case 4D) dataset across all MPI ranks.
    // Will be the same for all configured benchmarks.
    openPMD::Extent total{
        100,
        100,
        100,
        10
    };

    // The blockslicer assigns to each rank its part of the dataset. The rank will
    // write to and read from that part. OneDimensionalBlockSlicer is a simple
    // implementation of the BlockSlicer abstract class that will divide the
    // dataset into hyperslab along one given dimension.
    // If you wish to partition your dataset in a different manner, you can
    // replace this with your own implementation of BlockSlicer.
    auto blockSlicer = std::make_shared<openPMD::OneDimensionalBlockSlicer>(0);

    // Set up the DatasetFiller. The benchmarks will later inquire the DatasetFiller
    // to get data for writing.
    std::uniform_int_distribution<type> distr(
        0,
        200000000
    );
    openPMD::RandomDatasetFiller<decltype(distr)> df{distr};

    // The Benchmark class will in principle allow a user to configure
    // runs that write and read different datatypes.
    // For this, the class is templated with a type called DatasetFillerProvider.
    // This class serves as a factory for DatasetFillers for concrete types and
    // should have a templated operator()<T>() returning a value
    // that can be dynamically casted to a std::shared_ptr<openPMD::DatasetFiller<T>>
    // The openPMD API provides only one implementation of a DatasetFillerProvider,
    // namely the SimpleDatasetFillerProvider being used in this example.
    // Its purpose is to leverage a DatasetFiller for a concrete type (df in this example)
    // to a DatasetFillerProvider whose operator()<T>() will fail during runtime if T does
    // not correspond with the underlying DatasetFiller.
    // Use this implementation if you only wish to run the benchmark for one Datatype,
    // otherwise provide your own implementation of DatasetFillerProvider.
    openPMD::SimpleDatasetFillerProvider<decltype(df)> dfp{df};

    // Create the Benchmark object. The file name (first argument) will be extended
    // with the backends' file extensions.
    openPMD::MPIBenchmark<decltype(dfp)> benchmark{
        "../benchmarks/benchmark",
        total,
        std::dynamic_pointer_cast<openPMD::BlockSlicer>(blockSlicer),
        dfp,
    };

    // Add benchmark runs to be executed. This will only store the configuration and not
    // run the benchmark yet. Each run is configured by:
    // * The compression scheme to use (first two parameters). The first parameter chooses
    //   the compression scheme, the second parameter is the compression level.
    // * The backend (by file extension).
    // * The datatype to use for this run.
    // * The number of iterations. Effectively, the benchmark will be repeated for this many
    //   times.
#if openPMD_HAVE_ADIOS1 || openPMD_HAVE_ADIOS2
    benchmark.addConfiguration("", 0, "bp", dt, 10);
#endif
#if openPMD_HAVE_HDF5
    benchmark.addConfiguration("", 0, "h5", dt, 10);
#endif

    // Execute all previously configured benchmarks. Will return a MPIBenchmarkReport object
    // with write and read times for each configured run.
    // Take notice that results will be collected into the root rank's report object, the other
    // ranks' reports will be empty. The root rank is specified by the first parameter of runBenchmark,
    // the default being 0.
    auto res =
        benchmark.runBenchmark<std::chrono::high_resolution_clock>();

    int rank;
    MPI_Comm_rank(
        MPI_COMM_WORLD,
        &rank
    );
    if( rank == 0 )
    {
        for( auto it = res.durations.begin();
             it != res.durations.end();
             it++ )
        {
            auto time = it->second;
            std::cout << "on rank " << std::get<res.RANK>(it->first)
                      << "\t with backend "
                      << std::get<res.BACKEND>(it->first)
                      << "\twrite time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(
                          time.first
                      ).count() << "\tread time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(
                          time.second
                      ).count() << std::endl;
        }
    }

    MPI_Finalize();
}
#else
int main(void)
{
    return 0;
}
#endif
