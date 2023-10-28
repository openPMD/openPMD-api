#include <openPMD/binding/c/ReadIterations.h>

#include <openPMD/ReadIterations.hpp>

void openPMD_ReadIterations_delete(openPMD_ReadIterations *iterations)
{
    const auto cxx_iterations = (openPMD::ReadIterations *)iterations;
    delete cxx_iterations;
}
