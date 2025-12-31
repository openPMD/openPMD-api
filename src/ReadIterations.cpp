/* Copyright 2025 Franz Poeschel, Luca Fedeli
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
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
    if (access == Access::READ_LINEAR && !data.m_sharedStatefulIterator)
    {
        // Open the iterator now already, so that metadata may already be read
        data.m_sharedStatefulIterator = std::make_unique<StatefulIterator>(
            StatefulIterator::tag_read, m_series, m_parsePreference);
    }
}

auto ReadIterations::begin() -> iterator_t
{
    auto &series = m_series.get();
    if (!series.m_sharedStatefulIterator)
    {
        series.m_sharedStatefulIterator = std::make_unique<StatefulIterator>(
            StatefulIterator::tag_read, m_series, m_parsePreference);
    }
    return stateful_to_opaque(*series.m_sharedStatefulIterator);
}

auto ReadIterations::end() -> iterator_t
{
    return stateful_to_opaque(StatefulIterator::end());
}
} // namespace openPMD
