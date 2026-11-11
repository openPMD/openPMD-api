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
#include "AuxiliaryTests.hpp"

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/ChildrenCache.hpp"
#include "openPMD/backend/ChildrenCacheImpl.tpp"
#include "openPMD/backend/Writable.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace openPMD::internal
{
template <>
auto container_type_id<int, std::string>() -> std::uint8_t
{
    return 240;
}
template <>
auto container_type_id<double, std::string>() -> std::uint8_t
{
    return 241;
}
template <>
auto container_type_id<int, std::uint64_t>() -> std::uint8_t
{
    return 242;
}
} // namespace openPMD::internal

namespace children_cache
{
using namespace openPMD;

namespace
{
    auto make_child() -> std::shared_ptr<internal::SharedAttributableData>
    {
        return std::make_shared<internal::SharedAttributableData>(nullptr);
    }
} // namespace

auto children_cache() -> void
{
    SECTION("get_or_create_returns_same_map")
    {
        internal::ChildrenCache cache;
        cache.try_emplace<int, std::string>("a", make_child(), 1);

        auto second = cache.get_or_create_cache<int, std::string>();
        REQUIRE(second.contains("a"));
        REQUIRE(second.at("a") == 1);
        REQUIRE(cache.num_maps() == 1);

        cache.assert_consistent();
    }

    SECTION("different_types_are_independent")
    {
        internal::ChildrenCache cache;
        auto child = make_child();
        cache.try_emplace<int, std::string>("a", child, 1);
        cache.try_emplace<double, std::string>("a", child, 2.5);
        auto ints = cache.get_or_create_cache<int, std::string>();
        auto doubles = cache.get_or_create_cache<double, std::string>();

        REQUIRE(cache.num_maps() == 2);
        REQUIRE(ints.at("a") == 1);
        REQUIRE(doubles.at("a") == 2.5);

        cache.assert_consistent();
    }

    SECTION("erase_removes_from_every_map")
    {
        internal::ChildrenCache cache;
        cache.try_emplace<int, std::string>("a", make_child(), 1);
        cache.try_emplace<int, std::string>("b", make_child(), 2);
        cache.try_emplace<int, std::uint64_t>(42, make_child(), 3);
        auto strings = cache.get_or_create_cache<int, std::string>();
        auto numbers = cache.get_or_create_cache<int, std::uint64_t>();

        cache.erase("a");
        REQUIRE(!strings.contains("a"));
        REQUIRE(strings.contains("b"));

        cache.erase("42");
        REQUIRE(!numbers.contains(42));
        REQUIRE(numbers.empty());

        cache.assert_consistent();
    }

    SECTION("clear_empties_every_map")
    {
        internal::ChildrenCache cache;
        cache.try_emplace<int, std::string>("a", make_child(), 1);
        cache.try_emplace<int, std::uint64_t>(42, make_child(), 3);
        auto strings = cache.get_or_create_cache<int, std::string>();
        auto numbers = cache.get_or_create_cache<int, std::uint64_t>();
        REQUIRE(!cache.empty());

        cache.clear();
        REQUIRE(cache.empty());
        REQUIRE(strings.empty());
        REQUIRE(numbers.empty());

        cache.assert_consistent();
    }

    SECTION("erased_keys_do_not_resurrect")
    {
        internal::ChildrenCache cache;
        cache.try_emplace<int, std::string>("a", make_child(), 1);
        cache.try_emplace<int, std::uint64_t>(42, make_child(), 1);
        auto strings = cache.get_or_create_cache<int, std::string>();
        auto numbers = cache.get_or_create_cache<int, std::uint64_t>();

        cache.erase("a");
        cache.erase("42");

        // Reinserting the same keys must create fresh entries, not revive
        // stale handles from the caches.
        cache.try_emplace<int, std::string>("a", make_child(), 2);
        cache.try_emplace<int, std::uint64_t>(42, make_child(), 2);

        REQUIRE(strings.at("a") == 2);
        REQUIRE(numbers.at(42) == 2);
        cache.assert_consistent();
    }

    SECTION("find_and_contains")
    {
        internal::ChildrenCache cache;
        REQUIRE(!cache.contains_cache<int, std::string>());
        REQUIRE(!cache.find_cache<int, std::string>().has_value());

        cache.try_emplace<int, std::string>("a", make_child(), 1);

        REQUIRE(cache.contains_cache<int, std::string>());
        auto found = cache.find_cache<int, std::string>();
        REQUIRE(found.has_value());
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        REQUIRE(found->at("a") == 1);
    }

    SECTION("typed_insert_registers_authoritative_child")
    {
        internal::ChildrenCache cache;
        auto child = make_child();

        auto inserted = cache.try_emplace<int, std::string>("a", child, 1);
        REQUIRE(inserted.second);
        REQUIRE(cache.children().size() == 1);
        REQUIRE(cache.children().at("a") == child);

        auto duplicate = cache.try_emplace<int, std::string>("a", child, 2);
        REQUIRE(!duplicate.second);
        REQUIRE(cache.children().size() == 1);
        REQUIRE(cache.get_or_create_cache<int, std::string>().at("a") == 1);
        cache.assert_consistent();
    }

    SECTION("integral_keys_must_be_canonical")
    {
        using namespace internal::children_cache_detail;
        using U = std::uint64_t;
        using I = int;

        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        REQUIRE(key_from_string<U>("42").value() == 42);
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        REQUIRE(key_from_string<I>("-42").value() == -42);
        REQUIRE(!key_from_string<U>("042"));
        REQUIRE(!key_from_string<U>("+42"));
        REQUIRE(!key_from_string<U>(" 42"));
        REQUIRE(!key_from_string<U>("-1"));
        REQUIRE(!key_from_string<I>("042"));
        REQUIRE(!key_from_string<I>("+42"));
        REQUIRE(!key_from_string<I>(" 42"));
        REQUIRE(!key_from_string<I>("-0"));
    }

    SECTION("group_metadata_erase_clears_every_cache")
    {
        internal::object_type::GroupMetaData group;
        auto child = make_child();
        group.m_children_cache.try_emplace<int, std::string>("x", child, 1);
        group.m_children_cache.try_emplace<double, std::string>(
            "x", child, 2.0);
        auto ints =
            group.m_children_cache.get_or_create_cache<int, std::string>();
        auto doubles =
            group.m_children_cache.get_or_create_cache<double, std::string>();

        group.erase_child("x");
        REQUIRE(!ints.contains("x"));
        REQUIRE(!doubles.contains("x"));

        auto child_y = make_child();
        group.m_children_cache.try_emplace<int, std::string>("y", child_y, 3);
        group.m_children_cache.try_emplace<double, std::string>(
            "y", child_y, 4.0);
        group.clear_children();
        REQUIRE(ints.empty());
        REQUIRE(doubles.empty());
    }
}
} // namespace children_cache
