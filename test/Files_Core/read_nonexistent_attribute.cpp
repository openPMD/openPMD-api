
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

#if !openPMD_USE_INVASIVE_TESTS

namespace read_nonexistent_attribute
{
void read_nonexistent_attribute()
{}
} // namespace read_nonexistent_attribute

#else

#define OPENPMD_private public:
#define OPENPMD_protected public:

#include "CoreTests.hpp"

#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/IOTask.hpp"

#include <catch2/catch_test_macros.hpp>
namespace read_nonexistent_attribute
{
using namespace openPMD;
static auto testedFileExtensions() -> std::vector<std::string>
{
    auto allExtensions = getFileExtensions();
    auto newEnd = std::remove_if(
        allExtensions.begin(),
        allExtensions.end(),
        []([[maybe_unused]] std::string const &ext) {
            // sst and ssc need a receiver for testing
            // bp5 is already tested via bp
            // toml parsing is very slow and its implementation is equivalent to
            // the json backend, so it is only activated for selected tests
            return ext == "sst" || ext == "ssc" || ext == "bp5" ||
                ext == "toml";
        });
    return {allExtensions.begin(), newEnd};
}

void run(std::string const &ext)
{
    auto const filename = "../samples/read_nonexistent_attribute." + ext;

    auto do_create = [&filename]() {
        Series write(filename, Access::CREATE);
        write.close();
    };

    do_create();

    // Try reading an attribute from this Series which does not actually exist.
    // This tests the bugs fixed in
    // https://github.com/openPMD/openPMD-api/pull/1866:
    //
    // 1. The HDF5 backend should verify that the attribute exists before trying
    //    to read it. Otherwise, the read failure will print ugly backtraces.
    // 2. The HDF5 backend should clean up resources also in case an operations
    //    returns early. Otherwise the second call to do_create() will fail,
    //    since the first HDF5 file will remain open (resource leak).
    {
        Series read(filename, Access::READ_ONLY);
        Parameter<Operation::READ_ATT> readAttr;
        readAttr.name = "this_attribute_does_hopefully_not_exist";
        read.IOHandler()->enqueue(IOTask(&read, readAttr));
        REQUIRE_THROWS_AS(
            read.IOHandler()->flush(internal::defaultFlushParams),
            error::ReadError);
    }

    do_create();
}

void read_nonexistent_attribute()
{
    for (auto const &ext : testedFileExtensions())
    {
        run(ext);
    }
}
} // namespace read_nonexistent_attribute
#endif
