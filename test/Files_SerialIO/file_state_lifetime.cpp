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

namespace file_state_lifetime
{
// This test needs access to Writable::fileState, so it only does something
// when invasive tests are enabled.
auto file_state_lifetime() -> void
{}
} // namespace file_state_lifetime

#else

#define OPENPMD_private public:

#include "SerialIOTests.hpp"

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerHelper.hpp"
#include "openPMD/IO/Format.hpp"
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/IO/InvalidatableFile.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Writable.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <optional>
#include <string>

namespace file_state_lifetime
{
using namespace openPMD;

/*
 * The file-state lifetime is managed by the IO handlers: A Writable keeps a
 * SharedFileState that is shared with the handler's `m_files` bookkeeping. This
 * test drives an IO handler directly in order to exercise the three lifetime
 * scenarios that matter: reopening after CLOSE_FILE, re-creating a file that is
 * still open, and a failed OPEN_FILE.
 */

/*
 * A minimal Attributable in order to get access to a root-level Writable
 * without going through the public Series API.
 */
struct RootWritable : public Attributable
{
    Writable *root()
    {
        return &writable();
    }
};

struct Fixture
{
    std::string directory;
    std::unique_ptr<AbstractIOHandler> handler;
    RootWritable root;
    Writable *writable;

    Fixture(
        std::string directory_in,
        Access access,
        Format format,
        std::string originalExtension)
        : directory(std::move(directory_in))
        , handler(createIOHandler(
              std::nullopt,
              directory,
              access,
              format,
              std::move(originalExtension)))
        , writable(root.root())
    {}
};

void create_file(
    AbstractIOHandler &handler, Writable *writable, std::string name)
{
    Parameter<Operation::CREATE_FILE> create;
    create.name = std::move(name);
    handler.enqueue(IOTask(writable, std::move(create)));
    handler.flush(internal::defaultFlushParams);
}

void close_file(AbstractIOHandler &handler, Writable *writable)
{
    Parameter<Operation::CLOSE_FILE> close;
    handler.enqueue(IOTask(writable, std::move(close)));
    handler.flush(internal::defaultFlushParams);
}

void reopen_after_close_file(std::string const &ext, Format format)
{
    std::string const directory =
        "../samples/file_state_lifetime/reopen_" + ext;
    auxiliary::remove_directory(directory);
    Fixture fixture(directory, Access::CREATE, format, "." + ext);

    create_file(*fixture.handler, fixture.writable, "reopen");
    REQUIRE(fixture.writable->fileState.has_value());

    close_file(*fixture.handler, fixture.writable);
    REQUIRE(!fixture.writable->fileState.has_value());

    // After closing, the same file must be re-openable, obtaining a fresh
    // file state.
    Parameter<Operation::OPEN_FILE> open;
    open.name = "reopen";
    open.reopen = Parameter<Operation::OPEN_FILE>::Reopen::WasCreatedByUs;
    fixture.handler->enqueue(IOTask(fixture.writable, std::move(open)));
    fixture.handler->flush(internal::defaultFlushParams);
    REQUIRE(fixture.writable->fileState.has_value());

    // The freshly opened file can be used, e.g. by creating a path in it.
    Parameter<Operation::CREATE_PATH> createPath;
    createPath.path = "some_path";
    fixture.handler->enqueue(IOTask(fixture.writable, std::move(createPath)));
    fixture.handler->flush(internal::defaultFlushParams);

    close_file(*fixture.handler, fixture.writable);
    REQUIRE(!fixture.writable->fileState.has_value());
}

void recreate_still_open_file(std::string const &ext, Format format)
{
    std::string const directory =
        "../samples/file_state_lifetime/recreate_" + ext;
    auxiliary::remove_directory(directory);
    Fixture fixture(directory, Access::CREATE, format, "." + ext);

    create_file(*fixture.handler, fixture.writable, "recreate");
    REQUIRE(fixture.writable->fileState.has_value());
    auto *const old_state = fixture.writable->fileState.get();

    // Creating the same file again while it is still open must replace the old
    // file state instead of leaving two competing states behind.
    RootWritable second_root;
    Writable *second = second_root.root();
    create_file(*fixture.handler, second, "recreate");
    REQUIRE(second->fileState.has_value());
    REQUIRE(second->fileState.get() != old_state);
    /*
     * The first Writable shares its state with the handler's bookkeeping, so
     * replacing the file invalidates the old Writable's view of the file.
     */
    REQUIRE(!fixture.writable->fileState.has_value());

    close_file(*fixture.handler, second);
    REQUIRE(!second->fileState.has_value());
}

/*
 * A failed open must not leave the file registered in the handler, so that a
 * retry obtains a fresh file state afterwards. This holds for all backends.
 */
void failed_open(std::string const &ext, Format format)
{
    std::string const directory =
        "../samples/file_state_lifetime/failed_open_" + ext;
    auxiliary::remove_directory(directory);
    auxiliary::create_directories(directory);

    Fixture fixture(directory, Access::READ_ONLY, format, "." + ext);

    /*
     * Open the file and then read from it. Reading is not required for the
     * failure itself, but it makes the test exercise backends that open files
     * lazily as well.
     */
    auto open_and_read = [&fixture](std::string const &name) {
        Parameter<Operation::OPEN_FILE> open;
        open.name = name;
        open.reopen = Parameter<Operation::OPEN_FILE>::Reopen::NoReopen;
        fixture.handler->enqueue(IOTask(fixture.writable, std::move(open)));

        Parameter<Operation::LIST_PATHS> list;
        fixture.handler->enqueue(IOTask(fixture.writable, std::move(list)));
        fixture.handler->flush(internal::defaultFlushParams);
    };

    // Opening and reading a file that does not exist must fail ...
    REQUIRE_THROWS(open_and_read("does_not_exist"));

    /*
     * The failed attempt must not leave a registered file behind in the
     * handler's bookkeeping. If it did, the retry below would reuse that stale
     * state instead of creating a fresh one.
     */
    auto *const state_after_failure = fixture.writable->fileState.get();

    // ... and must not corrupt the handler's bookkeeping. Write the file now
    // using a separate handler.
    {
        Fixture writer(directory, Access::CREATE, format, "." + ext);
        create_file(*writer.handler, writer.writable, "does_not_exist");
        close_file(*writer.handler, writer.writable);
    }

    // The same handler must afterwards be able to open and use the file.
    open_and_read("does_not_exist");
    REQUIRE(fixture.writable->fileState.has_value());
    REQUIRE(fixture.writable->fileState.get() != state_after_failure);

    close_file(*fixture.handler, fixture.writable);
    REQUIRE(!fixture.writable->fileState.has_value());
}

auto file_state_lifetime() -> void
{
    reopen_after_close_file("json", Format::JSON);
    recreate_still_open_file("json", Format::JSON);
    failed_open("json", Format::JSON);
#if openPMD_HAVE_ADIOS2
    reopen_after_close_file("bp", Format::ADIOS2_BP);
    recreate_still_open_file("bp", Format::ADIOS2_BP);
    failed_open("bp", Format::ADIOS2_BP);
#endif
}
} // namespace file_state_lifetime

#endif
