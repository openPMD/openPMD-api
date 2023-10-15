#include <openPMD/binding/c/WriteIterations.h>

#include <openPMD/WriteIterations.hpp>

openPMD_Iteration *
openPMD_WriteIterations_get(openPMD_WriteIterations *iterations, uint64_t key)
{
    const auto cxx_iterations = (openPMD::WriteIterations *)iterations;
    const auto cxx_iteration = (*cxx_iterations)[key];
    const auto cxx_new_iteration = new openPMD::Iteration(cxx_iteration);
    const auto iteration = (openPMD_Iteration *)cxx_new_iteration;
    return iteration;
}

openPMD_IndexedIteration *
openPMD_WriteIterations_currentIteration(openPMD_WriteIterations *iterations)
{
    const auto cxx_iterations = (openPMD::WriteIterations *)iterations;
    const auto cxx_indexed_iteration = cxx_iterations->currentIteration();
    if (!cxx_indexed_iteration)
        return nullptr;
    const auto cxx_new_indexed_iteration =
        new openPMD::IndexedIteration(*cxx_indexed_iteration);
    const auto indexed_iteration =
        (openPMD_IndexedIteration *)cxx_new_indexed_iteration;
    return indexed_iteration;
}
