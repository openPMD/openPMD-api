#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#if openPMD_HAVE_MPI
#include <mpi.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    Catch::Session session;
    int result = 0;
    {
        // Indicates a command line parsing
        result = session.applyCommandLine(argc, argv);
        // RT tests
        if (result == 0)
            result = session.run();
    }
    MPI_Finalize();
    return result;
}
#else
int main(int argc, char *argv[])
{
    Catch::Session session;
    int result = 0;
    {
        // Indicates a command line parsing
        result = session.applyCommandLine(argc, argv);
        // RT tests
        if (result == 0)
            result = session.run();
    }
    return result;
}
#endif
