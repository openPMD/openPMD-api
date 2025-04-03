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
