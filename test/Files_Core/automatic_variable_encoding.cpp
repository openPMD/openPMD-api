#include "CoreTests.hpp"

#include <catch2/catch.hpp>

namespace automatic_variable_encoding
{
auto automatic_variable_encoding() -> void
{
#if openPMD_HAVE_ADIOS2
    using namespace openPMD;

    size_t filename_counter = 0;
    auto filename = [&filename_counter]() {
        std::stringstream res;
        res << "../samples/automatic_variable_encoding/test_no_"
            << filename_counter << ".bp5";
        return res.str();
    };
    auto next_filename = [&filename_counter, &filename]() {
        ++filename_counter;
        return filename();
    };
    auto require_encoding = [&filename](IterationEncoding ie) {
        Series read(filename(), openPMD::Access::READ_RANDOM_ACCESS);
        REQUIRE(read.iterationEncoding() == ie);
    };

    // TESTS

    {
        Series write(next_filename(), Access::CREATE);
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    {
        Series write(next_filename(), Access::CREATE_RANDOM_ACCESS);
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    {
        Series write(next_filename(), Access::CREATE_LINEAR);
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(next_filename(), Access::CREATE_RANDOM_ACCESS);
        write.flush();
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    {
        Series write(next_filename(), Access::CREATE_LINEAR);
        write.flush();
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    // explicitly set variable encoding

    {
        Series write(
            next_filename(),
            Access::CREATE,
            R"(iteration_encoding = "variable_based")");
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(
            next_filename(),
            Access::CREATE_RANDOM_ACCESS,
            R"(iteration_encoding = "variable_based")");
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(
            next_filename(),
            Access::CREATE_LINEAR,
            R"(iteration_encoding = "variable_based")");
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(
            next_filename(),
            Access::CREATE_RANDOM_ACCESS,
            R"(iteration_encoding = "variable_based")");
        write.flush();
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(
            next_filename(),
            Access::CREATE_LINEAR,
            R"(iteration_encoding = "variable_based")");
        write.flush();
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    // explicitly set variable encoding

    {
        Series write(
            next_filename(),
            Access::CREATE,
            R"(iteration_encoding = "group_based")");
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    {
        Series write(
            next_filename(),
            Access::CREATE_RANDOM_ACCESS,
            R"(iteration_encoding = "group_based")");
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    {
        Series write(
            next_filename(),
            Access::CREATE_LINEAR,
            R"(iteration_encoding = "group_based")");
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    {
        Series write(
            next_filename(),
            Access::CREATE_RANDOM_ACCESS,
            R"(iteration_encoding = "group_based")");
        write.flush();
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    {
        Series write(
            next_filename(),
            Access::CREATE_LINEAR,
            R"(iteration_encoding = "group_based")");
        write.flush();
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    // explicitly use API call to set variable encoding

    {
        Series write(next_filename(), Access::CREATE);
        write.setIterationEncoding(IterationEncoding::variableBased);
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(next_filename(), Access::CREATE_RANDOM_ACCESS);
        write.setIterationEncoding(IterationEncoding::variableBased);
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(next_filename(), Access::CREATE_LINEAR);
        write.setIterationEncoding(IterationEncoding::variableBased);
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(next_filename(), Access::CREATE_RANDOM_ACCESS);
        write.setIterationEncoding(IterationEncoding::variableBased);
        write.flush();
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(next_filename(), Access::CREATE_LINEAR);
        write.setIterationEncoding(IterationEncoding::variableBased);
        write.flush();
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    // explicitly use API call to set group encoding

    {
        Series write(next_filename(), Access::CREATE);
        write.setIterationEncoding(IterationEncoding::groupBased);
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    {
        Series write(next_filename(), Access::CREATE_RANDOM_ACCESS);
        write.setIterationEncoding(IterationEncoding::groupBased);
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    {
        Series write(next_filename(), Access::CREATE_LINEAR);
        write.setIterationEncoding(IterationEncoding::groupBased);
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    {
        Series write(next_filename(), Access::CREATE_RANDOM_ACCESS);
        write.setIterationEncoding(IterationEncoding::groupBased);
        write.flush();
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    {
        Series write(next_filename(), Access::CREATE_LINEAR);
        write.setIterationEncoding(IterationEncoding::groupBased);
        write.flush();
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    // explicitly use API call to set group encoding

    {
        Series write(next_filename(), Access::CREATE);
        write.setIterationEncoding(IterationEncoding::variableBased);
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(next_filename(), Access::CREATE_RANDOM_ACCESS);
        write.setIterationEncoding(IterationEncoding::variableBased);
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(next_filename(), Access::CREATE_LINEAR);
        write.setIterationEncoding(IterationEncoding::variableBased);
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(next_filename(), Access::CREATE_RANDOM_ACCESS);
        write.setIterationEncoding(IterationEncoding::variableBased);
        write.flush();
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(next_filename(), Access::CREATE_LINEAR);
        write.setIterationEncoding(IterationEncoding::variableBased);
        write.flush();
        write.snapshots();
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    // explicitly use API call to set group encoding a bit late

    {
        Series write(next_filename(), Access::CREATE_RANDOM_ACCESS);
        write.snapshots();
        write.setIterationEncoding(IterationEncoding::groupBased);
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    {
        Series write(next_filename(), Access::CREATE_LINEAR);
        write.snapshots();
        write.setIterationEncoding(IterationEncoding::groupBased);
        write.close();
        require_encoding(IterationEncoding::groupBased);
    }

    // explicitly use API call to set variable encoding a bit late

    {
        Series write(next_filename(), Access::CREATE_RANDOM_ACCESS);
        write.snapshots();
        write.setIterationEncoding(IterationEncoding::variableBased);
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }

    {
        Series write(next_filename(), Access::CREATE_LINEAR);
        write.snapshots();
        write.setIterationEncoding(IterationEncoding::variableBased);
        write.close();
        require_encoding(IterationEncoding::variableBased);
    }
#endif
}
} // namespace automatic_variable_encoding
