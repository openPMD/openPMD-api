#include "openPMD/ReadIterations.hpp"
#include "openPMD/snapshots/IteratorHelpers.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"

namespace openPMD
{
LegacyIteratorAdaptor::LegacyIteratorAdaptor(Snapshots::iterator iterator)
    : m_iterator(std::move(iterator))
{}

auto LegacyIteratorAdaptor::operator*() const -> value_type
{
    return m_iterator.operator*();
}

auto LegacyIteratorAdaptor::operator++() -> LegacyIteratorAdaptor &
{
    ++m_iterator;
    return *this;
}

auto LegacyIteratorAdaptor::operator==(LegacyIteratorAdaptor const &other) const
    -> bool
{
    return m_iterator == other.m_iterator;
}

auto LegacyIteratorAdaptor::operator!=(LegacyIteratorAdaptor const &other) const
    -> bool
{
    return m_iterator != other.m_iterator;
}

ReadIterations::ReadIterations(
    Series series,
    Access access,
    std::optional<internal::ParsePreference> parsePreference)
    : m_series(std::move(series)), m_parsePreference(parsePreference)
{
    auto &data = m_series.get();
    if (access == Access::READ_LINEAR && !data.m_sharedReadIterations)
    {
        // Open the iterator now already, so that metadata may already be read
        data.m_sharedReadIterations = std::make_unique<StatefulIterator>(
            StatefulIterator::tag_read, m_series, m_parsePreference);
    }
}

auto ReadIterations::begin() -> iterator_t
{
    auto &series = m_series.get();
    if (!series.m_sharedReadIterations)
    {
        series.m_sharedReadIterations = std::make_unique<StatefulIterator>(
            StatefulIterator::tag_read, m_series, m_parsePreference);
    }
    return stateful_to_opaque(*series.m_sharedReadIterations);
}

auto ReadIterations::end() -> iterator_t
{
    return stateful_to_opaque(StatefulIterator::end());
}
} // namespace openPMD
