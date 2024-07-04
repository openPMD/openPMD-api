#pragma once

/*
 * Legacy header.
 */

#include "openPMD/Iteration.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/snapshots/Snapshots.hpp"

namespace openPMD
{
/** @brief Legacy Iterator type for `Series::readIterations()`
 *
 * Wraps the Iterator type of `Series::snapshots()`, but has `IndexedIteration`
 * as value_type instead of `std::pair<uint64_t, Iteration>`.
 */
class LegacyIteratorAdaptor
{
    using value_type = IndexedIteration;
    using parent_t = Snapshots::iterator;

private:
    friend class ReadIterations;
    Snapshots::iterator m_iterator;
    LegacyIteratorAdaptor(Snapshots::iterator iterator);

public:
    value_type operator*() const;
    LegacyIteratorAdaptor &operator++();
    bool operator==(LegacyIteratorAdaptor const &other) const;
    bool operator!=(LegacyIteratorAdaptor const &other) const;
};

/**
 * @brief Legacy class as return type for `Series::readIterations()`.
 *
 * This is a feature-restricted subset for the functionality of
 * `Series::snapshots()`, prefer using that. The compatibility layer is needed
 * due to the different value_type for `Series::readIterations()`-based
 * iteration (`IterationIndex` instead of `std::pair<uint64_t, Iteration>`).
 *
 * Create instance via Series::readIterations().
 * For use in a C++11-style foreach loop over iterations.
 * Designed to allow reading any kind of Series, streaming and non-
 * streaming alike.
 * Calling Iteration::close() manually before opening the next iteration is
 * encouraged and will implicitly flush all deferred IO actions.
 * Otherwise, Iteration::close() will be implicitly called upon
 * StatefulIterator::operator++(), i.e. upon going to the next iteration in
 * the foreach loop.
 *
 */
class ReadIterations
{
    friend class Series;

private:
    using iterations_t = decltype(internal::SeriesData::iterations);
    using iterator_t = LegacyIteratorAdaptor;

    Series m_series;
    std::optional<internal::ParsePreference> m_parsePreference;

    ReadIterations(
        Series,
        Access,
        std::optional<internal::ParsePreference> parsePreference);

public:
    auto begin() -> iterator_t;
    static auto end() -> iterator_t;
};
} // namespace openPMD
