#include <openPMD/binding/c/ReadIterations.h>

#include <openPMD/ReadIterations.hpp>
#include <openPMD/Series.hpp>

// SeriesIterator

openPMD_SeriesIterator *openPMD_SeriesIterator_new(
    openPMD_Series *series, openPMD_ParsePreference parsePreference)
{
    const auto cxx_series = (openPMD::Series *)series;
    std::optional<openPMD::internal::ParsePreference> cxx_parsePreference;
    if (parsePreference != openPMD_ParsePreference_None)
        cxx_parsePreference = {
            openPMD::internal::ParsePreference(parsePreference)};
    const auto cxx_seriesIterator =
        new openPMD::SeriesIterator(*cxx_series, cxx_parsePreference);
    const auto seriesIterator = (openPMD_SeriesIterator *)cxx_seriesIterator;
    return seriesIterator;
}

void openPMD_SeriesIterator_delete(openPMD_SeriesIterator *seriesIterator)
{
    const auto cxx_seriesIterator = (openPMD::SeriesIterator *)seriesIterator;
    delete cxx_seriesIterator;
}

bool openPMD_SeriesIterator_done(const openPMD_SeriesIterator *seriesIterator)
{
    const auto cxx_seriesIterator =
        (const openPMD::SeriesIterator *)seriesIterator;
    return *cxx_seriesIterator == openPMD::SeriesIterator::end();
}

void openPMD_SeriesIterator_advance(openPMD_SeriesIterator *seriesIterator)
{
    const auto cxx_seriesIterator = (openPMD::SeriesIterator *)seriesIterator;
    ++*cxx_seriesIterator;
}

openPMD_IndexedIteration *
openPMD_SeriesIterator_get(openPMD_SeriesIterator *seriesIterator)
{
    const auto cxx_seriesIterator = (openPMD::SeriesIterator *)seriesIterator;
    const auto cxx_indexedIteration =
        new openPMD::IndexedIteration(**cxx_seriesIterator);
    const auto indexedIteration =
        (openPMD_IndexedIteration *)cxx_indexedIteration;
    return indexedIteration;
}

// ReadIterations

void openPMD_ReadIterations_delete(openPMD_ReadIterations *readIterations)
{
    const auto cxx_readIterations = (openPMD::ReadIterations *)readIterations;
    delete cxx_readIterations;
}

openPMD_SeriesIterator *
openPMD_ReadIterations_iterate(const openPMD_ReadIterations *readIterations)
{
    const auto cxx_readIterations = (openPMD::ReadIterations *)readIterations;
    const auto cxx_seriesIterator =
        new openPMD::SeriesIterator(cxx_readIterations->begin());
    const auto seriesIterator = (openPMD_SeriesIterator *)cxx_seriesIterator;
    return seriesIterator;
}
