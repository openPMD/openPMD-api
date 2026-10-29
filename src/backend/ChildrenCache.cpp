/* Copyright 2026 The openPMD Community
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
#include "openPMD/backend/ChildrenCache.hpp"

#include "ContainerTypeMacros.hpp"

#include "openPMD/CustomHierarchy.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticlePatches.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Record.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/backend/ChildrenCacheImpl.tpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"

#include <cstdint>

namespace openPMD::internal
{
namespace
{
    enum class ContainerTypeId : std::uint8_t
    {
#define OPENPMD_ENUMERATE_CONTAINER_TYPE(type, key) type,
        OPENPMD_FOREACH_CONTAINER_TYPE(OPENPMD_ENUMERATE_CONTAINER_TYPE)
#undef OPENPMD_ENUMERATE_CONTAINER_TYPE
    };
} // namespace

#define OPENPMD_DEFINE_CONTAINER_TYPE_ID(type, key)                            \
    template <>                                                                \
    auto container_type_id<type, key>() -> std::uint8_t                        \
    {                                                                          \
        return static_cast<std::uint8_t>(ContainerTypeId::type);               \
    }
OPENPMD_FOREACH_CONTAINER_TYPE(OPENPMD_DEFINE_CONTAINER_TYPE_ID)
#undef OPENPMD_DEFINE_CONTAINER_TYPE_ID

#define OPENPMD_INSTANTIATE_CONTAINER_CACHE(type, key)                         \
    template ChildrenCacheMap<type, key>                                       \
    ChildrenCache::get_or_create_cache<type, key>() const;                     \
    template std::optional<ChildrenCacheMap<type, key>>                        \
    ChildrenCache::find_cache<type, key>() const;                              \
    template bool ChildrenCache::contains_cache<type, key>() const noexcept;   \
    template std::pair<typename std::map<key, type>::iterator, bool>           \
    ChildrenCache::try_emplace<type, key>(                                     \
        key const &,                                                           \
        std::shared_ptr<SharedAttributableData> const &,                       \
        type &&);                                                              \
    template std::pair<typename std::map<key, type>::iterator, bool>           \
    ChildrenCache::try_emplace<type, key>(                                     \
        key const &, std::shared_ptr<SharedAttributableData> const &, type &&) \
        const;

OPENPMD_FOREACH_CONTAINER_TYPE(OPENPMD_INSTANTIATE_CONTAINER_CACHE)
#undef OPENPMD_INSTANTIATE_CONTAINER_CACHE

ChildrenCache::children_map_t &ChildrenCache::children()
{
    return m_children;
}

ChildrenCache::children_map_t const &ChildrenCache::children() const
{
    return m_children;
}

void ChildrenCache::erase(std::string const &canonicalKey)
{
    for (auto &[type, entry] : m_entries)
    {
        (void)type;
        entry->erase(canonicalKey);
    }

    for (auto const &[type, entry] : m_entries)
    {
        (void)type;
        assert(!entry->contains(canonicalKey));
    }
    m_children.erase(canonicalKey);
}

void ChildrenCache::clear()
{
    for (auto &[type, entry] : m_entries)
    {
        (void)type;
        entry->clear();
    }

    for (auto const &[type, entry] : m_entries)
    {
        (void)type;
        assert(entry->size() == 0);
    }
    m_children.clear();

    assert_consistent();
}

bool ChildrenCache::empty() const noexcept
{
    for (auto const &[type, entry] : m_entries)
    {
        (void)type;
        if (entry->size() != 0)
        {
            return false;
        }
    }
    return true;
}

std::size_t ChildrenCache::num_maps() const noexcept
{
    return m_entries.size();
}

void ChildrenCache::assert_consistent() const
{
#ifndef NDEBUG
    for (auto const &pair : m_entries)
    {
        auto &entry = pair.second;

        std::size_t const registered_size = entry->size();
        std::size_t counted_size = 0;
        entry->for_each_key([&](std::string const &key) {
            ++counted_size;
            assert(entry->contains(key));
            assert(m_children.find(key) != m_children.end());
        });
        assert(counted_size == registered_size);
    }
#endif
}
} // namespace openPMD::internal
