#include "SerialIOTests.hpp"
#include "openPMD/IO/Access.hpp"

namespace filebased_write_test
{
using namespace openPMD;

#define OPENPMD_TEST_VERBOSE 0

namespace
{
    template <typename... Args>
    auto write_to_stdout([[maybe_unused]] Args &&...args) -> void
    {
#if OPENPMD_TEST_VERBOSE
        (std::cout << ... << args);
#endif
    }
} // namespace

auto close_and_reopen_iterations(
    const std::string &filename,
    openPMD::Access access,
    std::string const &json_config,
    bool need_to_explitly_open_iterations) -> void
{
    Series list(filename, access, json_config);

    auto test_read = [](Iteration &iteration) {
        auto component = iteration.particles["e"]["position"]["x"];
        auto chunk = component.loadChunkVariant();
        iteration.seriesFlush();
        auto num_particles = component.getExtent()[0];
        write_to_stdout("Particles: ");
        if (num_particles > 0)
        {
            std::visit(
                [&](auto const &shared_ptr) {
                    auto it = shared_ptr.get();
                    auto end = it + num_particles;
                    write_to_stdout('[', *it++);
                    for (; it != end; ++it)
                    {
                        write_to_stdout(", ", *it);
                    }
                },
                chunk);
            write_to_stdout("]");
        }
        else
        {
            write_to_stdout("[]");
        }
        write_to_stdout('\n');
    };

    for (auto &[idx, iteration] : list.snapshots())
    {
        write_to_stdout("Seeing iteration ", idx, '\n');
        if (need_to_explitly_open_iterations)
        {
            iteration.open();
        }
        if (iteration.particles.contains("e"))
        {
            test_read(iteration);
        }
        write_to_stdout("Closing iteration ", idx, '\n');
        iteration.close();
    }
    write_to_stdout("Trying to read iteration 3 out of line", '\n');
    if (need_to_explitly_open_iterations || access == Access::READ_ONLY)
    {
        list.snapshots()[3].open();
    }
    test_read(list.snapshots()[3]);

    write_to_stdout("----------\nGoing again\n----------", '\n');
    for (auto &[idx, iteration] : list.snapshots())
    {
        write_to_stdout("Seeing iteration ", idx, '\n');
        if (need_to_explitly_open_iterations || access == Access::READ_ONLY)
        {
            iteration.open();
        }
        if (iteration.particles.contains("e"))
        {
            test_read(iteration);
        }
    }
}

auto close_and_reopen_iterations(std::string const &filename) -> void
{
    close_and_reopen_iterations(
        filename,
        Access::READ_LINEAR,
        R"({"defer_iteration_parsing":false})",
        false);
    close_and_reopen_iterations(
        filename,
        Access::READ_LINEAR,
        R"({"defer_iteration_parsing":true})",
        false);
    close_and_reopen_iterations(
        filename,
        Access::READ_ONLY,
        R"({"defer_iteration_parsing":false})",
        false);
    close_and_reopen_iterations(
        filename,
        Access::READ_ONLY,
        R"({"defer_iteration_parsing":true})",
        true);
}
} // namespace filebased_write_test
