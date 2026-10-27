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
#include "openPMD/backend/Writable.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace children_cache
{
using namespace openPMD;

namespace
{
    void add_child(internal::ChildrenCache &cache, std::string key)
    {
        cache.children().emplace(
            std::move(key),
            std::make_shared<internal::SharedAttributableData>(nullptr));
    }
} // namespace

auto children_cache() -> void
{
    SECTION("get_or_create_returns_same_map")
    {
        internal::ChildrenCache cache;
        auto first = cache.get_or_create_cache<int, std::string>();
        add_child(cache, "a");
        first.unsafe_try_emplace("a", 1);

        auto second = cache.get_or_create_cache<int, std::string>();
        REQUIRE(second.contains("a"));
        REQUIRE(second.at("a") == 1);
        REQUIRE(cache.num_maps() == 1);

        cache.assert_consistent();
    }

    SECTION("different_types_are_independent")
    {
        internal::ChildrenCache cache;
        auto ints = cache.get_or_create_cache<int, std::string>();
        auto doubles = cache.get_or_create_cache<double, std::string>();
        add_child(cache, "a");
        ints.unsafe_try_emplace("a", 1);
        doubles.unsafe_try_emplace("a", 2.5);

        REQUIRE(cache.num_maps() == 2);
        REQUIRE(ints.at("a") == 1);
        REQUIRE(doubles.at("a") == 2.5);

        cache.assert_consistent();
    }

    SECTION("erase_removes_from_every_map")
    {
        internal::ChildrenCache cache;
        auto strings = cache.get_or_create_cache<int, std::string>();
        auto numbers = cache.get_or_create_cache<int, std::uint64_t>();
        add_child(cache, "a");
        add_child(cache, "b");
        add_child(cache, "42");
        strings.unsafe_try_emplace("a", 1);
        strings.unsafe_try_emplace("b", 2);
        numbers.unsafe_try_emplace(42, 3);

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
        auto strings = cache.get_or_create_cache<int, std::string>();
        auto numbers = cache.get_or_create_cache<int, std::uint64_t>();
        add_child(cache, "a");
        add_child(cache, "42");
        strings.unsafe_try_emplace("a", 1);
        numbers.unsafe_try_emplace(42, 3);
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
        auto strings = cache.get_or_create_cache<int, std::string>();
        auto numbers = cache.get_or_create_cache<int, std::uint64_t>();
        add_child(cache, "a");
        add_child(cache, "42");
        strings.unsafe_try_emplace("a", 1);
        numbers.unsafe_try_emplace(42, 1);

        cache.erase("a");
        cache.erase("42");

        // Reinserting the same keys must create fresh entries, not revive
        // stale handles from the caches.
        add_child(cache, "a");
        add_child(cache, "42");
        strings.unsafe_try_emplace("a", 2);
        numbers.unsafe_try_emplace(42, 2);

        REQUIRE(strings.at("a") == 2);
        REQUIRE(numbers.at(42) == 2);
        cache.assert_consistent();
    }

    SECTION("find_and_contains")
    {
        internal::ChildrenCache cache;
        REQUIRE(!cache.contains_cache<int, std::string>());
        REQUIRE(!cache.find_cache<int, std::string>().has_value());

        auto strings = cache.get_or_create_cache<int, std::string>();
        add_child(cache, "a");
        strings.unsafe_try_emplace("a", 1);

        REQUIRE(cache.contains_cache<int, std::string>());
        auto found = cache.find_cache<int, std::string>();
        REQUIRE(found.has_value());
        REQUIRE(found->at("a") == 1);
    }

    SECTION("integral_keys_must_be_canonical")
    {
        using namespace internal::children_cache_detail;
        using U = std::uint64_t;
        using I = int;

        REQUIRE(key_from_string<U>("42").value() == 42);
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
        auto ints =
            group.m_children_cache.get_or_create_cache<int, std::string>();
        auto doubles =
            group.m_children_cache.get_or_create_cache<double, std::string>();
        add_child(group.m_children_cache, "x");
        ints.unsafe_try_emplace("x", 1);
        doubles.unsafe_try_emplace("x", 2.0);

        group.erase_child("x");
        REQUIRE(!ints.contains("x"));
        REQUIRE(!doubles.contains("x"));

        add_child(group.m_children_cache, "y");
        ints.unsafe_try_emplace("y", 3);
        doubles.unsafe_try_emplace("y", 4.0);
        group.clear_children();
        REQUIRE(ints.empty());
        REQUIRE(doubles.empty());
    }
}
} // namespace children_cache
