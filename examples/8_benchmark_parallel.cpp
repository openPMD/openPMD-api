#include <openPMD/benchmark/mpi/MPIBenchmark.hpp>
#include <openPMD/benchmark/mpi/OneDimensionalBlockSlicer.hpp>
#include <openPMD/benchmark/mpi/RandomDatasetFiller.hpp>
#include <openPMD/openPMD.hpp>

#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

#include <iostream>
#include <string>
#include <vector>

#if openPMD_HAVE_MPI
inline void print_help(std::string const program_name)
{
    std::cout << "Usage: " << program_name << "\n";
    std::cout << "Run a simple parallel write and read benchmark.\n\n";
    std::cout << "Options:\n";
    std::cout
        << "    -w, --weak    run a weak scaling (default: strong scaling)\n";
    std::cout << "    -h, --help    display this help and exit\n";
    std::cout << "    -v, --version output version information and exit\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "    " << program_name << " --weak  # for a weak-scaling\n";
    std::cout << "    " << program_name << "  # for a strong scaling\n";
}

inline void print_version(std::string const program_name)
{
    std::cout << program_name << " (openPMD-api) " << openPMD::getVersion()
              << "\n";
    std::cout << "Copyright 2017-2021 openPMD contributors\n";
    std::cout << "Authors: Franz Poeschel, Axel Huebl et al.\n";
    std::cout << "License: LGPLv3+\n";
    std::cout << "This is free software: you are free to change and "
                 "redistribute it.\n"
                 "There is NO WARRANTY, to the extent permitted by law.\n";
}

int main(int argc, char *argv[])
{
    using namespace std;
    MPI_Init(&argc, &argv);

    // CLI parsing
    std::vector<std::string> str_argv;
    for (int i = 0; i < argc; ++i)
        str_argv.emplace_back(argv[i]);
    bool weak_scaling = false;

    for (int c = 1; c < int(argc); c++)
    {
        if (std::string("--help") == argv[c] || std::string("-h") == argv[c])
        {
            print_help(argv[0]);
            return 0;
        }
        if (std::string("--version") == argv[c] || std::string("-v") == argv[c])
        {
            print_version(argv[0]);
            return 0;
        }
        if (std::string("--weak") == argv[c] || std::string("-w") == argv[c])
        {
            weak_scaling = true;
        }
    }

    if (argc > 2)
    {
        std::cerr << "Too many arguments! See: " << argv[0] << " --help\n";
        return 1;
    }

    // For simplicity, use only one datatype in this benchmark.
    // Note that a single Benchmark object can be used to configure
    // multiple different benchmark runs with different datatypes,
    // given that you provide it with an appropriate DatasetFillerProvider
    // (template parameter of the Benchmark class).
    using type = uint64_t;
#if openPMD_HAVE_ADIOS1 || openPMD_HAVE_ADIOS2 || openPMD_HAVE_HDF5
    openPMD::Datatype dt = openPMD::determineDatatype<type>();
#endif

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    const unsigned scale_up = weak_scaling ? unsigned(size) : 1u;

    // Total (in this case 3D) dataset across all MPI ranks.
    // Will be the same for all configured benchmarks.
    openPMD::Extent total{100 * scale_up, 100, 1000};

    // The blockslicer assigns to each rank its part of the dataset. The rank
    // will write to and read from that part. OneDimensionalBlockSlicer is a
    // simple implementation of the BlockSlicer abstract class that will divide
    // the dataset into hyperslab along one given dimension. If you wish to
    // partition your dataset in a different manner, you can replace this with
    // your own implementation of BlockSlicer.
    auto blockSlicer = std::make_shared<openPMD::OneDimensionalBlockSlicer>(0);

    // Set up the DatasetFiller. The benchmarks will later inquire the
    // DatasetFiller to get data for writing.
    std::uniform_int_distribution<type> distr(0, 200000000);
    openPMD::RandomDatasetFiller<decltype(distr)> df{distr};

    // The Benchmark class will in principle allow a user to configure
    // runs that write and read different datatypes.
    // For this, the class is templated with a type called
    // DatasetFillerProvider. This class serves as a factory for DatasetFillers
    // for concrete types and should have a templated operator()<T>() returning
    // a value that can be dynamically casted to a
    // std::shared_ptr<openPMD::DatasetFiller<T>> The openPMD API provides only
    // one implementation of a DatasetFillerProvider, namely the
    // SimpleDatasetFillerProvider being used in this example. Its purpose is to
    // leverage a DatasetFiller for a concrete type (df in this example) to a
    // DatasetFillerProvider whose operator()<T>() will fail during runtime if T
    // does not correspond with the underlying DatasetFiller. Use this
    // implementation if you only wish to run the benchmark for one Datatype,
    // otherwise provide your own implementation of DatasetFillerProvider.
    openPMD::SimpleDatasetFillerProvider<decltype(df)> dfp{df};

    // Create the Benchmark object. The file name (first argument) will be
    // extended with the backends' file extensions.
    openPMD::MPIBenchmark<decltype(dfp)> benchmark{
        "../benchmarks/benchmark",
        total,
        std::dynamic_pointer_cast<openPMD::BlockSlicer>(blockSlicer),
        dfp,
    };

    // Add benchmark runs to be executed. This will only store the configuration
    // and not run the benchmark yet. Each run is configured by:
    // * The compression scheme to use (first two parameters). The first
    // parameter chooses
    //   the compression scheme, the second parameter is the compression level.
    // * The backend (by file extension).
    // * The datatype to use for this run.
    // * The number of iterations. Effectively, the benchmark will be repeated
    // for this many
    //   times.
#if openPMD_HAVE_ADIOS1 || openPMD_HAVE_ADIOS2
    benchmark.addConfiguration("", 0, "bp", dt, 10);
#endif
#if openPMD_HAVE_HDF5
    benchmark.addConfiguration("", 0, "h5", dt, 10);
#endif

    // Execute all previously configured benchmarks. Will return a
    // MPIBenchmarkReport object with write and read times for each configured
    // run. Take notice that results will be collected into the root rank's
    // report object, the other ranks' reports will be empty. The root rank is
    // specified by the first parameter of runBenchmark, the default being 0.
    auto res = benchmark.runBenchmark<std::chrono::high_resolution_clock>();

    if (rank == 0)
    {
        for (auto it = res.durations.begin(); it != res.durations.end(); it++)
        {
            auto time = it->second;
            std::cout << "on rank " << std::get<res.RANK>(it->first)
                      << "\t with backend " << std::get<res.BACKEND>(it->first)
                      << "\twrite time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(
                             time.first)
                             .count()
                      << "\tread time: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(
                             time.second)
                             .count()
                      << std::endl;
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
