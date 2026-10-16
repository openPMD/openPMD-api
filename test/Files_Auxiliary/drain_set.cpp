/* Copyright 2026 Franz Poeschel
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

#include "openPMD/auxiliary/DrainSet.hpp"

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <string>
#include <vector>

namespace drain_set
{
using namespace openPMD;

auto drain_set() -> void
{
    SECTION("empty_set")
    {
        std::set<int> set;
        size_t visited = 0;
        for ([[maybe_unused]] int const value : auxiliary::drain(set))
        {
            ++visited;
        }
        REQUIRE(visited == 0);
        REQUIRE(set.empty());
    }

    SECTION("full_drain_visits_sorted_and_empties_set")
    {
        std::set<int> set{3, 1, 4, 2};
        std::vector<int> visited;
        for (int const value : auxiliary::drain(set))
        {
            visited.push_back(value);
        }
        REQUIRE(visited == std::vector<int>{1, 2, 3, 4});
        REQUIRE(set.empty());
    }

    SECTION("elements_are_mutable_during_drain")
    {
        std::set<int> set{1, 2, 3};
        std::vector<int> observed;
        for (int &value : auxiliary::drain(set))
        {
            value += 100;
            observed.push_back(value);
        }
        REQUIRE(observed == std::vector<int>{101, 102, 103});
        REQUIRE(set.empty());
    }

    SECTION("drain_can_be_repeated")
    {
        std::set<std::string> set{"a", "c"};
        auto first = [&set]() {
            std::vector<std::string> visited;
            for (std::string const &value : auxiliary::drain(set))
            {
                visited.push_back(value);
            }
            return visited;
        };
        REQUIRE(first() == std::vector<std::string>{"a", "c"});
        REQUIRE(set.empty());
        set.emplace("d");
        set.emplace("b");
        REQUIRE(first() == std::vector<std::string>{"b", "d"});
        REQUIRE(set.empty());
    }

    SECTION("insertion_during_drain_is_safe")
    {
        // Extracting elements one by one means that the set is not iterated in
        // the usual way, so modifying it while draining is allowed. Elements
        // that are inserted before the drain has run out of elements are picked
        // up as well.
        std::set<int> set{1, 2, 3};
        std::vector<int> visited;
        for (int const value : auxiliary::drain(set))
        {
            visited.push_back(value);
            if (value == 1)
            {
                set.insert(4);
            }
        }
        REQUIRE(visited == std::vector<int>{1, 2, 3, 4});
        REQUIRE(set.empty());
    }

    SECTION("iterator_comparison")
    {
        // An empty set: begin() immediately yields an empty node, so it
        // compares equal to the sentinel end().
        std::set<int> empty_set;
        auto view = auxiliary::drain(empty_set);
        REQUIRE(view.begin() == view.end());

        // A non-empty set: begin() extracts the first element, so it does NOT
        // equal the sentinel. Note: we use one drain_view here so that begin()
        // is called only once and we do not consume an element with each call.
        std::set<int> set{1, 2};
        auto nonempty_view = auxiliary::drain(set);
        REQUIRE(nonempty_view.begin() != nonempty_view.end());
        // One element was consumed by begin(); the set still has the rest.
        REQUIRE(set == std::set<int>{2});
    }

    SECTION("draining_a_subset_leaves_rest_in_set")
    {
        // Breaking out of the loop consumes the element that was current at
        // that point, but leaves the not-yet-visited elements in the set.
        std::set<int> set{1, 2, 3};
        for (int const value : auxiliary::drain(set))
        {
            REQUIRE(value == 1);
            // if condition needed for warnings to disappear
            if (value == 1)
            {
                break;
            }
        }
        REQUIRE(set == std::set<int>{2, 3});
    }
}
} // namespace drain_set
