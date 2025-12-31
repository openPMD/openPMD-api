/* Copyright 2025 Franz Poeschel
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
#include "SerialIOTests.hpp"

#include "openPMD/Dataset.hpp"
#include "openPMD/openPMD.hpp"

#include <memory>
#include <numeric>

// clang-format off
/*
 * Tests regression introduced with #1743:
 * terminate called after throwing an instance of 'openPMD::error::Internal'
 *   what():  Internal error: [ADIOS2 backend] Orphaned unique-ptr put operations found when closing file.
 * This is a bug. Please report at ' https://github.com/openPMD/openPMD-api/issues'.
 */
// clang-format on

namespace issue_1744_unique_ptrs_at_close_time
{
auto issue_1744_unique_ptrs_at_close_time() -> void
{
    openPMD::Series write(
        "../samples/issue_1744_unique_ptrs_at_close_time.bp4",
        openPMD::Access::CREATE,
        R"({"iteration_encoding": "group_based"})");
    std::unique_ptr<int[]> data_unique(new int[10]);
    std::iota(data_unique.get(), data_unique.get() + 10, 0);
    auto E_x = write.snapshots()[0].meshes["E"]["x"];
    E_x.resetDataset({openPMD::Datatype::INT, {10}});
    E_x.storeChunk(std::move(data_unique), {0}, {10});
    write.close();
}
} // namespace issue_1744_unique_ptrs_at_close_time
