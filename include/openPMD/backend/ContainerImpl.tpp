/* Copyright 2017-2025 Fabian Koller and Franz Poeschel
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

#include "openPMD/backend/Container.hpp"

/*
 * Instantiations in src/backend/Container.cpp
 * This file exists so that our tests can include the Container class with
 * custom instantiations.
 */

namespace openPMD
{
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::at(key_type const &key) -> mapped_type &
{
    return container().at(key);
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::at(key_type const &key) const
    -> mapped_type const &
{
    return container().at(key);
}

template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::operator[](key_type const &key)
    -> mapped_type &
{
    auto it = container().find(key);
    if (it != container().end())
        return it->second;
    else
    {
        if (IOHandler()->m_seriesStatus != internal::SeriesStatus::Parsing &&
            access::readOnly(IOHandler()->m_frontendAccess))
        {
            auxiliary::OutOfRangeMsg const out_of_range_msg;
            throw std::out_of_range(out_of_range_msg(key));
        }

        T t = T();
        t.linkHierarchy(writable());
        auto &ret = container().insert({key, std::move(t)}).first->second;
        if constexpr (std::is_same_v<T_key, std::string>)
        {
            ret.writable().ownKeyWithinParent = key;
        }
        else
        {
            ret.writable().ownKeyWithinParent = std::to_string(key);
        }
        traits::GenerationPolicy<T> gen;
        gen(ret);
        return ret;
    }
}
template <typename T, typename T_key, typename T_container>
auto Container<T, T_key, T_container>::operator[](key_type &&key)
    -> mapped_type &
{
    auto it = container().find(key);
    if (it != container().end())
        return it->second;
    else
    {
        if (IOHandler()->m_seriesStatus != internal::SeriesStatus::Parsing &&
            access::readOnly(IOHandler()->m_frontendAccess))
        {
            auxiliary::OutOfRangeMsg out_of_range_msg;
            throw std::out_of_range(out_of_range_msg(key));
        }

        T t = T();
        t.linkHierarchy(writable());
        auto &ret = container().insert({key, std::move(t)}).first->second;
        if constexpr (std::is_same_v<T_key, std::string>)
        {
            ret.writable().ownKeyWithinParent = std::move(key);
        }
        else
        {
            ret.writable().ownKeyWithinParent = std::to_string(std::move(key));
        }
        traits::GenerationPolicy<T> gen;
        gen(ret);
        return ret;
    }
}
} // namespace openPMD
