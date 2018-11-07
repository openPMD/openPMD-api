#include <openPMD/openPMD.hpp>
#include <openPMD/Series.hpp>
#include <openPMD/benchmark/mpi/MPIBenchmark.hpp>
#include <openPMD/benchmark/mpi/RandomDatasetFiller.hpp>
#include <openPMD/benchmark/mpi/OneDimensionalBlockSlicer.hpp>

#if openPMD_HAVE_MPI
#include <mpi.h>
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

    using type = long int;
    openPMD::Datatype dt = openPMD::determineDatatype<type>();

    openPMD::Extent total{
        100,
        100,
        100,
        10
    };
    auto blockSlicer = std::make_shared<openPMD::OneDimensionalBlockSlicer>(0);
    std::uniform_int_distribution<type> distr(
        0,
        200000000
    );

    openPMD::RandomDatasetFiller<decltype(distr)> df{distr};
    openPMD::SimpleDatasetFillerProvider<decltype(df)> dfp{df};

    // Since we use a SimpleDatasetFillerProvider, we may only configure
    // benchmark runs of the type fitting decltype(df)::resultType.
    // Otherwise, the DatasetFillerProvider will throw a runtime error.

    openPMD::MPIBenchmark<decltype(dfp)> benchmark{
        "../benchmarks/benchmark",
        total,
        std::dynamic_pointer_cast<openPMD::BlockSlicer>(blockSlicer),
        dfp,
    };

#if openPMD_HAVE_ADIOS1
    benchmark.addConfiguration("", 0, "bp", dt, 10);
#endif
#if openPMD_HAVE_HDF5
    benchmark.addConfiguration("", 0, "h5", dt, 10);
#endif

    auto
        res =
        benchmark.runBenchmark<std::chrono::high_resolution_clock>();

    int rank;
    MPI_Comm_rank(
        MPI_COMM_WORLD,
        &rank
    );
    if (rank == 0)
    {
        for (auto
                 it =
                 res.durations
                     .begin();
             it !=
             res.durations
                 .end();
             it++)
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
