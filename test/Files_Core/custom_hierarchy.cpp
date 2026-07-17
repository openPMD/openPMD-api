
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

#include "openPMD/IterationEncoding.hpp"
#define OPENPMD_private public:
#define OPENPMD_protected public:

#include "CoreTests.hpp"

#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/IOTask.hpp"

#include <catch2/catch_test_macros.hpp>
namespace custom_hierarchy
{
using namespace openPMD;

void write(
    std::string const &filename,
    std::string const &json_params,
    IterationEncoding)
{
    Series series(filename, Access::CREATE_LINEAR, json_params);
    auto add_custom_hierarchy = [](auto &&attr) {
        attr.customHierarchies()["rabimmel"].setAttribute("rabammel", "rabumm");
    };
    auto iteration = series.snapshots()[0];

    add_custom_hierarchy(series);
    add_custom_hierarchy(series.snapshots());
    add_custom_hierarchy(iteration);

    auto Ex = iteration.meshes["E"]["x"];
    Ex.resetDataset({Datatype::INT, {3}, R"(
                    resizable = true
                    hdf5.dataset.chunks = "auto")"});
    std::vector<int> Exdata{1, 2, 3};
    Ex.storeChunk(Exdata, {0}, {3});

    auto Ex_ =
        iteration.customHierarchies()["meshes"]["E"]["x"].as<RecordComponent>();
    REQUIRE(Ex_.getDatatype() == Ex.getDatatype());
    REQUIRE(Ex_.getExtent() == Ex.getExtent());
    iteration.seriesFlush();

    // Test resizing
    Ex_.resetDataset({{6}});
    Ex.storeChunk(Exdata, {3}, {3});
    iteration.close();
}

void read(
    std::string const &filename,
    std::string const &json_params,
    IterationEncoding)
{
    Series series(filename, Access::READ_LINEAR, json_params);
    auto require_custom_hierarchy = [](auto &&attr) {
        CustomHierarchy ch = attr.customHierarchies();
        REQUIRE(ch.find("rabimmel") == ch.end());
        ch.read(0);
        REQUIRE(
            ch["rabimmel"].getAttribute("rabammel").get<std::string>() ==
            "rabumm");
    };
    auto iteration = series.snapshots()[0];

    require_custom_hierarchy(series);
    require_custom_hierarchy(series.snapshots());
    require_custom_hierarchy(iteration);
    iteration.close();
}

struct test_config
{
    char const *filename;
    char const *json_params;
    IterationEncoding encoding;
};

void custom_hierarchy()
{
    test_config configs[] = {
        {"groupbased.%E",
         R"({"iteration_encoding": "group_based"})",
         IterationEncoding::groupBased},
        {"filebased_%T.%E",
         R"({"iteration_encoding": "file_based"})",
         IterationEncoding::fileBased},
        {"variablebased.%E",
         R"({"iteration_encoding": "variable_based"})",
         IterationEncoding::variableBased}};

    for (auto const &backend : {"adios2", "hdf5", "json", "toml"})
    {
        for (auto const &[filename, json_params, encoding] : configs)
        {
            auto json_params_ = json::merge(
                json_params,
                std::string(
                    R"(
                    {
                      "adios2": {
                        "engine": {
                          "type": "file"
                        }
                      },
                      "backend": ")") +
                    backend + R"("})");
            auto filename_ = std::string("../samples/custom_hierarchy/") +
                backend + "/" + filename;
            write(filename_, json_params_, encoding);
            read(filename_, json_params_, encoding);
        }
    }
}
} // namespace custom_hierarchy
