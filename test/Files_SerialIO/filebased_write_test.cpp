#include "SerialIOTests.hpp"
#include "openPMD/IO/Access.hpp"

namespace filebased_write_test
{
using namespace openPMD;

void close_and_reopen_iterations(
    const std::string &filename,
    openPMD::Access access,
    std::string const &json_config,
    bool need_to_explitly_open_iterations)
{
    Series list(filename, access, json_config);

    auto test_read = [](Iteration &iteration) {
        auto component = iteration.particles["e"]["position"]["x"];
        auto chunk = component.loadChunkVariant();
        iteration.seriesFlush();
        auto num_particles = component.getExtent()[0];
        std::cout << "Particles: ";
        if (num_particles > 0)
        {
            std::visit(
                [&](auto const &shared_ptr) {
                    auto it = shared_ptr.get();
                    auto end = it + num_particles;
                    std::cout << '[' << *it++;
                    for (; it != end; ++it)
                    {
                        std::cout << ", " << *it;
                    }
                },
                chunk);
            std::cout << "]";
        }
        else
        {
            std::cout << "[]";
        }
        std::cout << std::endl;
    };

    for (auto &[idx, iteration] : list.snapshots())
    {
        std::cout << "Seeing iteration " << idx << std::endl;
        if (need_to_explitly_open_iterations)
        {
            iteration.open();
        }
        if (iteration.particles.contains("e"))
        {
            test_read(iteration);
        }
        std::cout << "Closing iteration " << idx << std::endl;
        iteration.close();
    }
    std::cout << "Trying to read iteration 3 out of line" << std::endl;
    if (need_to_explitly_open_iterations || access == Access::READ_ONLY)
    {
        list.snapshots()[3].open();
    }
    test_read(list.snapshots()[3]);

    std::cout << "----------\nGoing again\n----------" << std::endl;
    for (auto &[idx, iteration] : list.snapshots())
    {
        std::cout << "Seeing iteration " << idx << std::endl;
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

void close_and_reopen_iterations(std::string const &filename)
{
    close_and_reopen_iterations(
        filename, Access::READ_LINEAR, "defer_iteration_parsing=false", false);
    close_and_reopen_iterations(
        filename, Access::READ_LINEAR, "defer_iteration_parsing=true", false);
    close_and_reopen_iterations(
        filename, Access::READ_ONLY, "defer_iteration_parsing=false", false);
    close_and_reopen_iterations(
        filename, Access::READ_ONLY, "defer_iteration_parsing=true", true);
}
} // namespace filebased_write_test
