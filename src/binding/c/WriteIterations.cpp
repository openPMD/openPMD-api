#include <openPMD/binding/c/WriteIterations.h>

#include <openPMD/WriteIterations.hpp>

void openPMD_WriteIterations_delete(openPMD_WriteIterations *writeIterations)
{
    const auto cxx_writeIterations =
        (openPMD::WriteIterations *)writeIterations;
    delete cxx_writeIterations;
}

openPMD_Iteration *openPMD_WriteIterations_get(
    openPMD_WriteIterations *writeIterations, uint64_t key)
{
    const auto cxx_writeIterations =
        (openPMD::WriteIterations *)writeIterations;
    const auto cxx_iteration = (*cxx_writeIterations)[key];
    const auto cxx_new_iteration = new openPMD::Iteration(cxx_iteration);
    const auto iteration = (openPMD_Iteration *)cxx_new_iteration;
    return iteration;
}

openPMD_IndexedIteration *openPMD_WriteIterations_currentIteration(
    openPMD_WriteIterations *writeIterations)
{
    const auto cxx_writeIterations =
        (openPMD::WriteIterations *)writeIterations;
    const auto cxx_indexedIteration = cxx_writeIterations->currentIteration();
    if (!cxx_indexedIteration)
        return nullptr;
    const auto cxx_new_indexedIteration =
        new openPMD::IndexedIteration(*cxx_indexedIteration);
    const auto indexedIteration =
        (openPMD_IndexedIteration *)cxx_new_indexedIteration;
    return indexedIteration;
}
