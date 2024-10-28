// expose private and protected members for invasive testing
#include "openPMD/ChunkInfo_internal.hpp"
#include "openPMD/Datatype.hpp"
#include "openPMD/IO/Access.hpp"
#if openPMD_USE_INVASIVE_TESTS
#define OPENPMD_private public:
#define OPENPMD_protected public:
#endif

#include "openPMD/IO/ADIOS/macros.hpp"
#include "openPMD/auxiliary/Environment.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/openPMD.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <complex>
#include <fstream>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#ifdef __unix__
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
// windows.h defines this macro and it breaks any function with the same name
#undef max
#endif

using namespace openPMD;

struct BackendSelection
{
    std::string backendName;
    std::string extension;

    [[nodiscard]] inline std::string jsonBaseConfig() const
    {
        return R"({"backend": ")" + backendName + "\"}";
    }
};

std::vector<BackendSelection> testedBackends()
{
    auto variants = getVariants();
    std::map<std::string, std::string> extensions{
        {"json", "json"}, {"adios2", "bp"}, {"hdf5", "h5"}};
    std::vector<BackendSelection> res;
    for (auto const &pair : variants)
    {
        if (pair.second)
        {
            auto lookup = extensions.find(pair.first);
            if (lookup != extensions.end())
            {
                std::string extension = lookup->second;
                res.push_back({pair.first, std::move(extension)});
            }
        }
    }
    return res;
}

std::vector<std::string> testedFileExtensions()
{
    auto allExtensions = getFileExtensions();
    auto newEnd = std::remove_if(
        allExtensions.begin(),
        allExtensions.end(),
        []([[maybe_unused]] std::string const &ext) {
#if openPMD_HAS_ADIOS_2_9
            // sst and ssc need a receiver for testing
            // bp5 is already tested via bp
            // toml parsing is very slow and its implementation is equivalent to
            // the json backend, so it is only activated for selected tests
            return ext == "sst" || ext == "ssc" || ext == "bp5" ||
                ext == "toml";
#else
            // toml parsing is very slow and its implementation is equivalent to
            // the json backend, so it is only activated for selected tests
            // sst and ssc need a receiver for testing
            // bp4 is already tested via bp
            return ext == "sst" || ext == "ssc" || ext == "bp4" ||
                ext == "toml";
#endif
        });
    return {allExtensions.begin(), newEnd};
}

namespace detail
{
template <typename Char>
void writeChar(Series &series, std::string const &component_name)
{
    auto component = series.iterations[0].meshes["E"][component_name];
    std::vector<Char> data(10);
    component.resetDataset({determineDatatype<Char>(), {10}});
    component.storeChunk(data, {0}, {10});
    series.flush();
}
template <typename Char>
void readChar(Series &series, std::string const &component_name)
{
    auto component = series.iterations[0].meshes["E"][component_name];
    std::vector<Char> data(10);
    auto chunk = component.loadChunk<Char>();
    series.flush();
    for (size_t i = 0; i < 10; ++i)
    {
        REQUIRE(data[i] == chunk.get()[i]);
    }
}
} // namespace detail

void char_roundtrip(std::string const &extension)
{
    Series write("../samples/char_rountrip." + extension, Access::CREATE);
    ::detail::writeChar<char>(write, "char");
    ::detail::writeChar<unsigned char>(write, "uchar");
    ::detail::writeChar<signed char>(write, "schar");
    write.close();

    Series read("../samples/char_rountrip." + extension, Access::READ_ONLY);
    ::detail::readChar<char>(read, "char");
    ::detail::readChar<unsigned char>(read, "uchar");
    ::detail::readChar<signed char>(read, "schar");
    read.close();
}

TEST_CASE("char_roundtrip", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        char_roundtrip(t);
    }
}

void write_and_read_many_iterations(
    std::string const &ext, bool intermittentFlushes)
{
    // the idea here is to trigger the maximum allowed number of file handles,
    // e.g., the upper limit in "ulimit -n" (default: often 1024). Once this
    // is reached, files should be closed automatically for open iterations

    // By flushing the series before closing an iteration, we ensure that the
    // iteration is not dirty before closing
    // Our flushing logic must not forget to close even if the iteration is
    // otherwise untouched and needs not be flushed.
    unsigned int nIterations =
        auxiliary::getEnvNum("OPENPMD_TEST_NFILES_MAX", 1030);
    std::string filename =
        "../samples/many_iterations/many_iterations_%T." + ext;

    std::vector<float> data(10);
    std::iota(data.begin(), data.end(), 0.);
    Dataset ds{Datatype::FLOAT, {10}};

    {
        Series write(filename, Access::CREATE);
        for (unsigned int i = 0; i < nIterations; ++i)
        {
            // std::cout << "Putting iteration " << i << std::endl;
            Iteration it = write.iterations[i];
            auto E_x = it.meshes["E"]["x"];
            E_x.resetDataset(ds);
            E_x.storeChunk(data, {0}, {10});
            if (intermittentFlushes)
            {
                write.flush();
            }
            it.close();
        }
        // ~Series intentionally not yet called

        Series read(
            filename, Access::READ_ONLY, "{\"defer_iteration_parsing\": true}");
        for (auto iteration : read.iterations)
        {
            iteration.second.open();
            // std::cout << "Reading iteration " << iteration.first <<
            // std::endl;
            auto E_x = iteration.second.meshes["E"]["x"];
            auto chunk = E_x.loadChunk<float>({0}, {10});
            if (intermittentFlushes)
            {
                read.flush();
            }
            iteration.second.close();

            auto array = chunk.get();
            for (size_t i = 0; i < 10; ++i)
            {
                REQUIRE(array[i] == float(i));
            }
        }
    }

    Series list(filename, Access::READ_ONLY);
    helper::listSeries(list);
}

TEST_CASE("write_and_read_many_iterations", "[serial]")
{
    bool intermittentFlushes = false;
    if (auxiliary::directory_exists("../samples/many_iterations"))
        auxiliary::remove_directory("../samples/many_iterations");
    for (auto const &t : testedFileExtensions())
    {
        if (t == "bp4" || t == "bp5")
        {
            continue;
        }
        write_and_read_many_iterations(t, intermittentFlushes);
        intermittentFlushes = !intermittentFlushes;
    }
}

TEST_CASE("multi_series_test", "[serial]")
{
    std::list<Series> allSeries;

    auto myfileExtensions = testedFileExtensions();

    // have multiple serial series alive at the same time
    for (auto const sn : {1, 2, 3})
    {
        for (auto const &file_ending : myfileExtensions)
        {
            std::cout << file_ending << std::endl;
            allSeries.emplace_back(
                std::string("../samples/multi_open_test_")
                    .append(std::to_string(sn))
                    .append(".")
                    .append(file_ending),
                Access::CREATE);
            allSeries.back().iterations[sn].setAttribute("wululu", sn);
            allSeries.back().flush();
        }
    }
    // skip some series: sn=1
    auto it = allSeries.begin();
    std::for_each(
        myfileExtensions.begin(),
        myfileExtensions.end(),
        [&it](std::string const &) { it++; });
    // remove some series: sn=2
    std::for_each(
        myfileExtensions.begin(),
        myfileExtensions.end(),
        [&it, &allSeries](std::string const &) { it = allSeries.erase(it); });
    // write from last series: sn=3
    std::for_each(
        myfileExtensions.begin(),
        myfileExtensions.end(),
        [&it](std::string const &) {
            it->iterations[10].setAttribute("wululu", 10);
            it->flush();
            it++;
        });

    // remove all leftover series
    allSeries.clear();
}

TEST_CASE("available_chunks_test_json", "[serial][json]")
{
    /*
     * This test is JSON specific
     * Our JSON backend does not store chunks explicitly,
     * so the JSON backend will simply go through the multidimensional array
     * and gather the data items into chunks
     * Example dataset:
     *
     *      0123
     *    0 ____
     *    1 ____
     *    2 ****
     *    3 ****
     *    4 ****
     *    5 ****
     *    6 ****
     *    7 **__
     *    8 **_*
     *    9 ___*
     *
     * Will be read as three chunks:
     * 1. (2,0) -- (5,4) (offset -- extent)
     * 2. (7,0) -- (2,2) (offset -- extent)
     * 3. (8,3) -- (2,1) (offset -- extent)
     *
     * Since the chunks are reconstructed, they won't necessarily
     * correspond with the way that the chunks were written.
     * As an example, let's write the first chunk in the above depiction
     * line by line.
     */
    constexpr unsigned height = 10;
    std::string name = "../samples/available_chunks.json";

    std::vector<int> data{2, 4, 6, 8};
    {
        Series write(name, Access::CREATE);
        Iteration it0 = write.iterations[0];
        auto E_x = it0.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {height, 4}});
        for (unsigned line = 2; line < 7; ++line)
        {
            E_x.storeChunk(data, {line, 0}, {1, 4});
        }
        for (unsigned line = 7; line < 9; ++line)
        {
            E_x.storeChunk(data, {line, 0}, {1, 2});
        }
        E_x.storeChunk(data, {8, 3}, {2, 1});

        auto E_y = it0.meshes["E"]["y"];
        E_y.resetDataset({Datatype::INT, {height, 4}});
        E_y.makeConstant(1234);

        it0.close();
    }

    {
        Series read(name, Access::READ_ONLY);
        Iteration it0 = read.iterations[0];
        auto E_x = it0.meshes["E"]["x"];
        ChunkTable table = E_x.availableChunks();
        REQUIRE(table.size() == 3);
        /*
         * Explicitly convert things to bool, so Catch doesn't get the splendid
         * idea to print the Chunk struct.
         */
        REQUIRE(bool(table[0] == WrittenChunkInfo({2, 0}, {5, 4})));
        REQUIRE(bool(table[1] == WrittenChunkInfo({7, 0}, {2, 2})));
        REQUIRE(bool(table[2] == WrittenChunkInfo({8, 3}, {2, 1})));

        auto E_y = it0.meshes["E"]["y"];
        table = E_y.availableChunks();
        REQUIRE(table.size() == 1);
        REQUIRE(bool(table[0] == WrittenChunkInfo({0, 0}, {height, 4})));
    }
}

TEST_CASE("multiple_series_handles_test", "[serial]")
{
    /*
     * clang also understands these pragmas.
     */
#if defined(__GNUC_MINOR__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    /*
     * First test: No premature flushes through destructor when another copy
     * is still around
     */
    {
        std::unique_ptr<openPMD::Series> series_ptr;
        {
            openPMD::Series series(
                "sample%T.json", openPMD::AccessType::CREATE);
            series_ptr = std::make_unique<openPMD::Series>(series);
            /*
             * we have two handles for the same Series instance now:
             * series and series_ptr
             * series goes out of scope before series_ptr
             * destructor should only do a flush after both of them go out of
             * scope, but it will currently run directly after this comment
             * since no iteration has been written yet, an error will be thrown
             */
        }
        series_ptr->iterations[0].meshes["E"]["x"].makeEmpty<int>(1);
    }
    /*
     * Second test: A Series handle should remain accessible even if the
     * original handle is destroyed
     */
    {
        std::unique_ptr<openPMD::Series> series_ptr;
        {
            openPMD::Series series(
                "sample%T.json", openPMD::AccessType::CREATE);
            series_ptr = std::make_unique<openPMD::Series>(series);
            series_ptr->iterations[0].meshes["E"]["x"].makeEmpty<int>(1);
        }
        /*
         * series_ptr is still in scope, but the original Series instance
         * has been destroyed
         * since internal pointers actually refer to the original *handle*,
         * doing anything with series_ptr now (such as flushing it) yields
         * nullpointer accesses
         */
        series_ptr->flush();
    }
#if defined(__GNUC_MINOR__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif
}

void close_iteration_test(std::string const &file_ending)
{
    std::string name = "../samples/close_iterations_%T." + file_ending;

    std::vector<int> data{2, 4, 6, 8};
    // { // we do *not* need these parentheses
    Series write(name, Access::CREATE);
    {
        Iteration it0 = write.iterations[0];
        auto E_x = it0.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {2, 2}});
        E_x.storeChunk(data, {0, 0}, {2, 2});
        it0.close(/* flush = */ false);
    }
    write.flush();
    // }

    {
        Series read(name, Access::READ_ONLY);
        Iteration it0 = read.iterations[0];
        auto E_x_read = it0.meshes["E"]["x"];
        auto chunk = E_x_read.loadChunk<int>({0, 0}, {2, 2});
        it0.close(/* flush = */ false);
        read.flush();
        for (size_t i = 0; i < data.size(); ++i)
        {
            REQUIRE(data[i] == chunk.get()[i]);
        }
    }

    {
        Iteration it1 = write.iterations[1];
        auto E_x = it1.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {2, 2}});
        E_x.storeChunk(data, {0, 0}, {2, 2});
        it1.close(/* flush = */ true);

        // illegally access iteration after closing
        E_x.storeChunk(data, {0, 0}, {2, 2});
        REQUIRE_THROWS(write.flush());
    }

    {
        Series read(name, Access::READ_ONLY);
        Iteration it1 = read.iterations[1];
        auto E_x_read = it1.meshes["E"]["x"];
        auto chunk = E_x_read.loadChunk<int>({0, 0}, {2, 2});
        it1.close(/* flush = */ true);
        for (size_t i = 0; i < data.size(); ++i)
        {
            REQUIRE(data[i] == chunk.get()[i]);
        }
        auto read_again = E_x_read.loadChunk<int>({0, 0}, {2, 2});
        REQUIRE_THROWS(read.flush());
    }

    {
        Series list{name, Access::READ_ONLY};
        helper::listSeries(list);
    }
}

TEST_CASE("close_iteration_test", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        close_iteration_test(t);
    }
}

void close_iteration_interleaved_test(
    std::string const &file_ending, IterationEncoding const it_encoding)
{
    std::string name = "../samples/close_iterations_interleaved_";
    if (it_encoding == IterationEncoding::fileBased)
        name.append("f_%T");
    else if (it_encoding == IterationEncoding::groupBased)
        name.append("g");
    else if (it_encoding == IterationEncoding::variableBased)
        name.append("v");
    name.append(".").append(file_ending);
    std::cout << name << std::endl;

    std::vector<int> data{2, 4, 6, 8};
    {
        Series write(name, Access::CREATE);
        write.setIterationEncoding(it_encoding);

        // interleaved write pattern
        Iteration it1 = write.iterations[1];
        auto E_x = it1.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {2, 2}});
        E_x.storeChunk(data, {0, 0}, {1, 2});
        E_x.seriesFlush();

        Iteration it2 = write.iterations[2];
        E_x = it2.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {2, 2}});
        E_x.storeChunk(data, {0, 0}, {1, 2});
        E_x.seriesFlush();

        E_x = it1.meshes["E"]["x"];
        E_x.storeChunk(data, {1, 0}, {1, 1});
        E_x.seriesFlush();

        E_x = it2.meshes["E"]["x"];
        E_x.storeChunk(data, {1, 0}, {1, 1});
        E_x.seriesFlush();

        // now we start a third iteration
        Iteration it3 = write.iterations[3];
        E_x = it3.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {2, 2}});
        E_x.storeChunk(data, {0, 0}, {1, 2});
        E_x.seriesFlush();

        // let's finish writing to 1 and 2
        E_x = it1.meshes["E"]["x"];
        E_x.storeChunk(data, {1, 1}, {1, 1});
        E_x.seriesFlush();
        it1.close();

        E_x = it2.meshes["E"]["x"];
        E_x.storeChunk(data, {1, 1}, {1, 1});
        E_x.seriesFlush();
        it2.close();

        E_x = it3.meshes["E"]["x"];
        E_x.storeChunk(data, {1, 0}, {1, 2});
        E_x.seriesFlush();
        it3.close();
    }
}

TEST_CASE("close_iteration_interleaved_test", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        close_iteration_interleaved_test(t, IterationEncoding::fileBased);
        close_iteration_interleaved_test(t, IterationEncoding::groupBased);

        // run this test for ADIOS2 & JSON only
        if (t == "h5")
            continue;
#if openPMD_HAS_ADIOS_2_9
        close_iteration_interleaved_test(t, IterationEncoding::variableBased);
#endif // openPMD_HAS_ADIOS_2_9
    }
}

namespace detail
{
template <typename T>
struct CopyableDeleter : std::function<void(T *)>
{
    CopyableDeleter()
        : std::function<void(T *)>{[](T const *ptr) { delete[] ptr; }}
    {}
};

template <typename T>
struct NonCopyableDeleter : std::function<void(T *)>
{
    NonCopyableDeleter()
        : std::function<void(T *)>{[](T const *ptr) { delete[] ptr; }}
    {}
    NonCopyableDeleter(NonCopyableDeleter const &) = delete;
    NonCopyableDeleter &operator=(NonCopyableDeleter const &) = delete;
    /*
     * MSVC does not recognize these when declaring noexcept and this is
     * for a test only anyway.
     */
    // NOLINTNEXTLINE(performance-noexcept-move-constructor)
    NonCopyableDeleter(NonCopyableDeleter &&) = default;
    // NOLINTNEXTLINE(performance-noexcept-move-constructor)
    NonCopyableDeleter &operator=(NonCopyableDeleter &&) = default;
};
} // namespace detail

void close_and_copy_attributable_test(std::string const &file_ending)
{
    using position_t = int;

    // open file for writing
    Series series("electrons." + file_ending, Access::CREATE);

    Datatype datatype = determineDatatype<position_t>();
    constexpr unsigned long length = 10ul;
    Extent global_extent = {length};
    Dataset dataset = Dataset(datatype, global_extent);
    std::shared_ptr<position_t> local_data(
        new position_t[length], [](position_t const *ptr) { delete[] ptr; });

    std::unique_ptr<Iteration> iteration_ptr;
    for (size_t i = 0; i < 100; i += 10)
    {
        if (iteration_ptr)
        {
            *iteration_ptr = series.iterations[i];
        }
        else
        {
            // use copy constructor
            iteration_ptr = std::make_unique<Iteration>(series.iterations[i]);
        }
        Record electronPositions = iteration_ptr->particles["e"]["position"];
        // TODO set this automatically to zero if not provided
        Record electronPositionsOffset =
            iteration_ptr->particles["e"]["positionOffset"];

        std::iota(local_data.get(), local_data.get() + length, i * length);
        /*
         * Hijack this test to additionally test the unique_ptr storeChunk API
         */
        // scalar unique_ptr, default delete
        auto pos_x = electronPositions["x"];
        pos_x.resetDataset(Dataset{datatype, {1}});
        pos_x.storeChunk(std::make_unique<int>(5), {0}, {1});

        // array unique_ptr, default delete
        auto posOff_x = electronPositionsOffset["x"];
        posOff_x.resetDataset(dataset);
        posOff_x.storeChunk(
            std::unique_ptr<int[]>{new int[10]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},
            {0},
            {global_extent});

        using CD = auxiliary::CustomDelete<int>;
        CD array_deleter{[](int const *ptr) { delete[] ptr; }};

        // scalar unique_ptr, custom delete
        auto pos_y = electronPositions["y"];
        pos_y.resetDataset(dataset);
        pos_y.storeChunk(
            std::unique_ptr<int, CD>{
                new int[10]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, array_deleter},
            {0},
            {global_extent});

        // array unique_ptr, custom delete
        auto posOff_y = electronPositionsOffset["y"];
        posOff_y.resetDataset(dataset);
        posOff_y.storeChunk(
            std::unique_ptr<int[], CD>{
                new int[10]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, array_deleter},
            {0},
            {global_extent});

        // scalar UniquePtrWithLambda, default delete
        auto pos_z = electronPositions["z"];
        pos_z.resetDataset(dataset);
        pos_z.storeChunk(
            UniquePtrWithLambda<int>{
                new int[10]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, array_deleter},
            {0},
            {global_extent});

        // array UniquePtrWithLambda, default delete
        auto posOff_z = electronPositionsOffset["z"];
        posOff_z.resetDataset(dataset);
        posOff_z.storeChunk(
            UniquePtrWithLambda<int[]>{
                new int[10]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, array_deleter},
            {0},
            {global_extent});

        // scalar UniquePtrWithLambda, custom delete
        // we're playing 4D now
        auto pos_w = electronPositions["w"];
        pos_w.resetDataset(dataset);
        pos_w.storeChunk(
            UniquePtrWithLambda<int>{
                new int[10]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, array_deleter},
            {0},
            {global_extent});

        // array UniquePtrWithLambda, custom delete
        auto posOff_w = electronPositionsOffset["w"];
        posOff_w.resetDataset(dataset);
        posOff_w.storeChunk(
            UniquePtrWithLambda<int[]>{
                new int[10]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, array_deleter},
            {0},
            {global_extent});

        // UniquePtrWithLambda from unique_ptr with custom delete type
        auto pos_v = electronPositions["v"];
        pos_v.resetDataset(dataset);
        std::unique_ptr<int, ::detail::CopyableDeleter<int>>
        ptr_v_copyable_deleter(new int[10]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
        pos_v.storeChunk(
            UniquePtrWithLambda<int>(std::move(ptr_v_copyable_deleter)),
            {0},
            {global_extent});

        // UniquePtrWithLambda from unique_ptr with non-copyable custom delete
        // type
        auto posOff_v = electronPositionsOffset["v"];
        posOff_v.resetDataset(dataset);
        std::unique_ptr<int, ::detail::NonCopyableDeleter<int>>
        ptr_v_noncopyable_deleter(new int[10]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
        posOff_v.storeChunk(
            UniquePtrWithLambda<int>(std::move(ptr_v_noncopyable_deleter)),
            {0},
            {global_extent});

        iteration_ptr->close();
        // force re-flush of previous iterations
        series.flush();
    }
}

TEST_CASE("close_and_copy_attributable_test", "[serial]")
{
    // demonstrator for https://github.com/openPMD/openPMD-api/issues/765
    for (auto const &t : testedFileExtensions())
    {
        close_and_copy_attributable_test(t);
    }
}

#if openPMD_HAVE_ADIOS2
TEST_CASE("close_iteration_throws_test", "[serial]")
{
    /*
     * Iterations should not be accessed any more after closing.
     * Test that the openPMD API detects that case and throws.
     */
    {
        Series series("../samples/close_iteration_throws_1.bp", Access::CREATE);
        auto it0 = series.iterations[0];
        auto E_x = it0.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {5}});
        std::vector<int> data{0, 1, 2, 3, 4};
        E_x.storeChunk(data, {0}, {5});
        it0.close();

        auto B_y = it0.meshes["B"]["y"];
        B_y.resetDataset({Datatype::INT, {5}});
        B_y.storeChunk(data, {0}, {5});
        REQUIRE_THROWS(series.flush());
    }
    {
        Series series("../samples/close_iteration_throws_2.bp", Access::CREATE);
        auto it0 = series.iterations[0];
        auto E_x = it0.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {5}});
        std::vector<int> data{0, 1, 2, 3, 4};
        E_x.storeChunk(data, {0}, {5});
        it0.close();

        auto e_position_x = it0.particles["e"]["position"]["x"];
        e_position_x.resetDataset({Datatype::INT, {5}});
        e_position_x.storeChunk(data, {0}, {5});
        REQUIRE_THROWS(series.flush());
    }
    {
        Series series("../samples/close_iteration_throws_3.bp", Access::CREATE);
        auto it0 = series.iterations[0];
        auto E_x = it0.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {5}});
        std::vector<int> data{0, 1, 2, 3, 4};
        E_x.storeChunk(data, {0}, {5});
        it0.close();

        it0.setTimeUnitSI(2.0);
        REQUIRE_THROWS(series.flush());
    }
}
#endif

inline void empty_dataset_test(std::string const &file_ending)
{
    {
        Series series(
            "../samples/empty_datasets." + file_ending, Access::CREATE);

        auto makeEmpty_dim_7_int =
            series.iterations[1].meshes["rho"]["makeEmpty_dim_7_int"];
        auto makeEmpty_dim_7_long =
            series.iterations[1].meshes["rho"]["makeEmpty_dim_7_bool"];
        auto makeEmpty_dim_7_int_alt =
            series.iterations[1].meshes["rho"]["makeEmpty_dim_7_int_alt"];
        auto makeEmpty_dim_7_long_alt =
            series.iterations[1].meshes["rho"]["makeEmpty_dim_7_bool_alt"];
        auto makeEmpty_resetDataset_dim3 =
            series.iterations[1].meshes["rho"]["makeEmpty_resetDataset_dim3"];
        auto makeEmpty_resetDataset_dim3_notallzero =
            series.iterations[1]
                .meshes["rho"]["makeEmpty_resetDataset_dim3_notallzero"];
        makeEmpty_dim_7_int.makeEmpty<int>(7);
        makeEmpty_dim_7_long.makeEmpty<long>(7);
        makeEmpty_dim_7_int_alt.makeEmpty(Datatype::INT, 7);
        makeEmpty_dim_7_long_alt.makeEmpty(Datatype::LONG, 7);
        makeEmpty_resetDataset_dim3.resetDataset(
            Dataset(Datatype::LONG, Extent(3, 0)));
        makeEmpty_resetDataset_dim3_notallzero.resetDataset(
            Dataset(Datatype::LONG_DOUBLE, Extent{1, 2, 0}));
        series.flush();
    }
    {
        Series series(
            "../samples/empty_datasets." + file_ending, Access::READ_ONLY);

        REQUIRE(series.iterations.contains(1));
        REQUIRE(series.iterations.count(1) == 1);
        REQUIRE(series.iterations.count(123456) == 0);

        REQUIRE(series.iterations[1].meshes.contains("rho"));
        REQUIRE(
            series.iterations[1].meshes["rho"].contains("makeEmpty_dim_7_int"));

        auto makeEmpty_dim_7_int =
            series.iterations[1].meshes["rho"]["makeEmpty_dim_7_int"];
        auto makeEmpty_dim_7_long =
            series.iterations[1].meshes["rho"]["makeEmpty_dim_7_bool"];
        auto makeEmpty_dim_7_int_alt =
            series.iterations[1].meshes["rho"]["makeEmpty_dim_7_int_alt"];
        auto makeEmpty_dim_7_long_alt =
            series.iterations[1].meshes["rho"]["makeEmpty_dim_7_bool_alt"];
        auto makeEmpty_resetDataset_dim3 =
            series.iterations[1].meshes["rho"]["makeEmpty_resetDataset_dim3"];
        auto makeEmpty_resetDataset_dim3_notallzero =
            series.iterations[1]
                .meshes["rho"]["makeEmpty_resetDataset_dim3_notallzero"];
        REQUIRE(makeEmpty_dim_7_int.getDimensionality() == 7);
        REQUIRE(makeEmpty_dim_7_int.getExtent() == Extent(7, 0));
        REQUIRE(isSame(
            makeEmpty_dim_7_int.getDatatype(), determineDatatype<int>()));

        REQUIRE(makeEmpty_dim_7_long.getDimensionality() == 7);
        REQUIRE(makeEmpty_dim_7_long.getExtent() == Extent(7, 0));
        REQUIRE(isSame(
            makeEmpty_dim_7_long.getDatatype(), determineDatatype<long>()));

        REQUIRE(makeEmpty_dim_7_int_alt.getDimensionality() == 7);
        REQUIRE(makeEmpty_dim_7_int_alt.getExtent() == Extent(7, 0));
        REQUIRE(isSame(
            makeEmpty_dim_7_int_alt.getDatatype(), determineDatatype<int>()));

        REQUIRE(makeEmpty_dim_7_long_alt.getDimensionality() == 7);
        REQUIRE(makeEmpty_dim_7_long_alt.getExtent() == Extent(7, 0));
        REQUIRE(isSame(
            makeEmpty_dim_7_long_alt.getDatatype(), determineDatatype<long>()));

        REQUIRE(makeEmpty_resetDataset_dim3.getDimensionality() == 3);
        REQUIRE(makeEmpty_resetDataset_dim3.getExtent() == Extent(3, 0));
        REQUIRE(
            isSame(makeEmpty_resetDataset_dim3.getDatatype(), Datatype::LONG));

        REQUIRE(
            makeEmpty_resetDataset_dim3_notallzero.getDimensionality() == 3);
        REQUIRE(
            makeEmpty_resetDataset_dim3_notallzero.getExtent() ==
            Extent{1, 2, 0});
        REQUIRE(isSame(
            makeEmpty_resetDataset_dim3_notallzero.getDatatype(),
            Datatype::LONG_DOUBLE));
    }

    {
        Series list{
            "../samples/empty_datasets." + file_ending, Access::READ_ONLY};
        helper::listSeries(list);
    }
}

TEST_CASE("empty_dataset_test", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        empty_dataset_test(t);
    }
}

inline void constant_scalar(std::string const &file_ending)
{
    Mesh::Geometry const geometry = Mesh::Geometry::spherical;
    std::string const geometryParameters = "dummyGeometryParameters";
    Mesh::DataOrder const dataOrder = Mesh::DataOrder::F;
    std::vector<double> const gridSpacing{1.0, 2.0, 3.0};
    std::vector<double> const gridGlobalOffset{11.0, 22.0, 33.0};
    double const gridUnitSI = 3.14;
    std::vector<std::string> const axisLabels{"x", "y", "z"};
    std::map<UnitDimension, double> const unitDimensions{
        {UnitDimension::I, 1.0}, {UnitDimension::J, 2.0}};
    double const timeOffset = 1234.0;

    {
        // constant scalar
        Series s =
            Series("../samples/constant_scalar." + file_ending, Access::CREATE);
        auto rho = s.iterations[1].meshes["rho"][MeshRecordComponent::SCALAR];
        REQUIRE(s.iterations[1].meshes["rho"].scalar());
        rho.resetDataset(Dataset(Datatype::CHAR, {1, 2, 3}));
        rho.makeConstant(static_cast<char>('a'));
        REQUIRE(rho.constant());

        // mixed constant/non-constant
        auto E_x = s.iterations[1].meshes["E"]["x"];
        E_x.resetDataset(Dataset(Datatype::FLOAT, {1, 2, 3}));
        E_x.makeConstant(static_cast<float>(13.37));
        auto E_y = s.iterations[1].meshes["E"]["y"];
        E_y.resetDataset(Dataset(Datatype::UINT, {1, 2, 3}));
        UniquePtrWithLambda<unsigned int> E(
            new unsigned int[6], [](unsigned int const *p) { delete[] p; });
        unsigned int e{0};
        std::generate(E.get(), E.get() + 6, [&e] { return e++; });
        E_y.storeChunk(std::move(E), {0, 0, 0}, {1, 2, 3});

        // store a number of predefined attributes in E
        Mesh &E_mesh = s.iterations[1].meshes["E"];
        E_mesh.setGeometry(geometry);
        E_mesh.setGeometryParameters(geometryParameters);
        E_mesh.setDataOrder(dataOrder);
        E_mesh.setGridSpacing(gridSpacing);
        E_mesh.setGridGlobalOffset(gridGlobalOffset);
        E_mesh.setGridUnitSI(gridUnitSI);
        E_mesh.setAxisLabels(axisLabels);
        E_mesh.setUnitDimension(unitDimensions);
        E_mesh.setTimeOffset(timeOffset);

        // constant scalar
        auto pos =
            s.iterations[1].particles["e"]["position"][RecordComponent::SCALAR];
        pos.resetDataset(Dataset(Datatype::DOUBLE, {3, 2, 1}));
        pos.makeConstant(static_cast<double>(42.));
        auto posOff =
            s.iterations[1]
                .particles["e"]["positionOffset"][RecordComponent::SCALAR];
        posOff.resetDataset(Dataset(Datatype::INT, {3, 2, 1}));
        posOff.makeConstant(static_cast<int>(-42));

        // mixed constant/non-constant
        auto vel_x = s.iterations[1].particles["e"]["velocity"]["x"];
        vel_x.resetDataset(Dataset(Datatype::SHORT, {3, 2, 1}));
        vel_x.makeConstant(static_cast<short>(-1));
        auto vel_y = s.iterations[1].particles["e"]["velocity"]["y"];
        vel_y.resetDataset(Dataset(Datatype::ULONGLONG, {3, 2, 1}));
        UniquePtrWithLambda<unsigned long long> vel(
            new unsigned long long[6],
            [](unsigned long long const *p) { delete[] p; });
        unsigned long long v{0};
        std::generate(vel.get(), vel.get() + 6, [&v] { return v++; });
        vel_y.storeChunk(std::move(vel), {0, 0, 0}, {3, 2, 1});
    }

    {
        Series s = Series(
            "../samples/constant_scalar." + file_ending, Access::READ_ONLY);
        REQUIRE(s.iterations[1].meshes.count("rho") == 1);
        REQUIRE(
            s.iterations[1].meshes["rho"].count(MeshRecordComponent::SCALAR) ==
            1);
        REQUIRE(s.iterations[1]
                    .meshes["rho"][MeshRecordComponent::SCALAR]
                    .containsAttribute("shape"));
        REQUIRE(
            s.iterations[1]
                .meshes["rho"][MeshRecordComponent::SCALAR]
                .getAttribute("shape")
                .get<std::vector<uint64_t>>() == Extent{1, 2, 3});
        REQUIRE(s.iterations[1]
                    .meshes["rho"][MeshRecordComponent::SCALAR]
                    .containsAttribute("value"));
        REQUIRE(
            s.iterations[1]
                .meshes["rho"][MeshRecordComponent::SCALAR]
                .getAttribute("value")
                .get<char>() == 'a');
        REQUIRE(s.iterations[1].meshes["rho"].scalar());
        REQUIRE(s.iterations[1]
                    .meshes["rho"][MeshRecordComponent::SCALAR]
                    .constant());
        REQUIRE(s.iterations[1].meshes.count("E") == 1);
        REQUIRE(!s.iterations[1].meshes["E"].scalar());
        REQUIRE(s.iterations[1].meshes["E"].count("x") == 1);
        REQUIRE(s.iterations[1].meshes["E"]["x"].constant());
        REQUIRE(s.iterations[1].meshes["E"]["x"].containsAttribute("shape"));
        REQUIRE(
            s.iterations[1]
                .meshes["E"]["x"]
                .getAttribute("shape")
                .get<std::vector<uint64_t>>() == Extent{1, 2, 3});
        REQUIRE(s.iterations[1].meshes["E"]["x"].containsAttribute("value"));
        REQUIRE(
            s.iterations[1]
                .meshes["E"]["x"]
                .getAttribute("value")
                .get<float>() == static_cast<float>(13.37));
        REQUIRE(!s.iterations[1].meshes["E"]["y"].constant());
        REQUIRE(
            s.iterations[1].meshes["E"]["y"].getExtent() == Extent{1, 2, 3});
        REQUIRE(s.iterations[1].meshes["E"].count("y") == 1);
        REQUIRE(!s.iterations[1].meshes["E"]["y"].containsAttribute("shape"));
        REQUIRE(!s.iterations[1].meshes["E"]["y"].containsAttribute("value"));
        REQUIRE(
            s.iterations[1].meshes["E"]["y"].getExtent() == Extent{1, 2, 3});

        REQUIRE(s.iterations[1].particles.count("e") == 1);
        REQUIRE(s.iterations[1].particles["e"].count("position") == 1);
        REQUIRE(
            s.iterations[1].particles["e"]["position"].count(
                RecordComponent::SCALAR) == 1);
        REQUIRE(s.iterations[1]
                    .particles["e"]["position"][RecordComponent::SCALAR]
                    .containsAttribute("shape"));
        REQUIRE(
            s.iterations[1]
                .particles["e"]["position"][RecordComponent::SCALAR]
                .getAttribute("shape")
                .get<std::vector<uint64_t>>() == Extent{3, 2, 1});
        REQUIRE(s.iterations[1]
                    .particles["e"]["position"][RecordComponent::SCALAR]
                    .containsAttribute("value"));
        REQUIRE(
            s.iterations[1]
                .particles["e"]["position"][RecordComponent::SCALAR]
                .getAttribute("value")
                .get<double>() == 42.);
        REQUIRE(
            s.iterations[1]
                .particles["e"]["position"][RecordComponent::SCALAR]
                .getExtent() == Extent{3, 2, 1});
        REQUIRE(s.iterations[1].particles["e"].count("positionOffset") == 1);
        REQUIRE(
            s.iterations[1].particles["e"]["positionOffset"].count(
                RecordComponent::SCALAR) == 1);
        REQUIRE(s.iterations[1]
                    .particles["e"]["positionOffset"][RecordComponent::SCALAR]
                    .containsAttribute("shape"));
        REQUIRE(
            s.iterations[1]
                .particles["e"]["positionOffset"][RecordComponent::SCALAR]
                .getAttribute("shape")
                .get<std::vector<uint64_t>>() == Extent{3, 2, 1});
        REQUIRE(s.iterations[1]
                    .particles["e"]["positionOffset"][RecordComponent::SCALAR]
                    .containsAttribute("value"));
        REQUIRE(
            s.iterations[1]
                .particles["e"]["positionOffset"][RecordComponent::SCALAR]
                .getAttribute("value")
                .get<int>() == -42);
        REQUIRE(
            s.iterations[1]
                .particles["e"]["positionOffset"][RecordComponent::SCALAR]
                .getExtent() == Extent{3, 2, 1});
        REQUIRE(s.iterations[1].particles["e"].count("velocity") == 1);
        REQUIRE(s.iterations[1].particles["e"]["velocity"].count("x") == 1);
        REQUIRE(
            s.iterations[1].particles["e"]["velocity"]["x"].containsAttribute(
                "shape"));
        REQUIRE(
            s.iterations[1]
                .particles["e"]["velocity"]["x"]
                .getAttribute("shape")
                .get<std::vector<uint64_t>>() == Extent{3, 2, 1});
        REQUIRE(
            s.iterations[1].particles["e"]["velocity"]["x"].containsAttribute(
                "value"));
        REQUIRE(
            s.iterations[1]
                .particles["e"]["velocity"]["x"]
                .getAttribute("value")
                .get<short>() == -1);
        REQUIRE(
            s.iterations[1].particles["e"]["velocity"]["x"].getExtent() ==
            Extent{3, 2, 1});
        REQUIRE(s.iterations[1].particles["e"]["velocity"].count("y") == 1);
        REQUIRE(
            !s.iterations[1].particles["e"]["velocity"]["y"].containsAttribute(
                "shape"));
        REQUIRE(
            !s.iterations[1].particles["e"]["velocity"]["y"].containsAttribute(
                "value"));
        REQUIRE(
            s.iterations[1].particles["e"]["velocity"]["y"].getExtent() ==
            Extent{3, 2, 1});

        Mesh &E_mesh = s.iterations[1].meshes["E"];
        REQUIRE(E_mesh.geometry() == geometry);
        REQUIRE(E_mesh.geometryParameters() == geometryParameters);
        REQUIRE(E_mesh.dataOrder() == dataOrder);
        REQUIRE(E_mesh.gridSpacing<double>() == gridSpacing);
        REQUIRE(E_mesh.gridGlobalOffset() == gridGlobalOffset);
        REQUIRE(E_mesh.gridUnitSI() == gridUnitSI);
        REQUIRE(E_mesh.axisLabels() == axisLabels);
        // REQUIRE( E_mesh.unitDimension() == unitDimensions );
        REQUIRE(E_mesh.timeOffset<double>() == timeOffset);

        auto E_x_value = s.iterations[1].meshes["E"]["x"].loadChunk<float>();
        s.flush();
        for (int idx = 0; idx < 1 * 2 * 3; ++idx)
            REQUIRE(E_x_value.get()[idx] == static_cast<float>(13.37));
    }

    {
        Series list{
            "../samples/constant_scalar." + file_ending, Access::READ_ONLY};
        helper::listSeries(list);
    }
}

TEST_CASE("constant_scalar", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        constant_scalar(t);
    }
}

TEST_CASE("flush_without_position_positionOffset", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        const std::string &file_ending = t;
        Series s = Series(
            "../samples/flush_without_position_positionOffset." + file_ending,
            Access::CREATE);
        ParticleSpecies e = s.iterations[0].particles["e"];
        RecordComponent weighting = e["weighting"][RecordComponent::SCALAR];
        weighting.resetDataset(Dataset(Datatype::FLOAT, Extent{2, 2}));
        weighting.storeChunk(
            std::shared_ptr<float>(
                new float[4](), [](float const *ptr) { delete[] ptr; }),
            {0, 0},
            {2, 2});

        s.flush();

        for (auto const &key : {"position", "positionOffset"})
        {
            for (auto const &dim : {"x", "y", "z"})
            {
                RecordComponent rc = e[key][dim];
                rc.resetDataset(Dataset(Datatype::FLOAT, Extent{2, 2}));
                rc.storeChunk(
                    std::shared_ptr<float>(
                        new float[4](), [](float const *ptr) { delete[] ptr; }),
                    {0, 0},
                    {2, 2});
            }
        }
    }
}

inline void particle_patches(std::string const &file_ending)
{
    constexpr auto SCALAR = openPMD::RecordComponent::SCALAR;

    uint64_t const extent = 123u;
    uint64_t const num_patches = 2u;
    {
        // constant scalar
        Series s = Series(
            "../samples/particle_patches%T." + file_ending, Access::CREATE);

        auto e = s.iterations[42].particles["electrons"];

        for (auto r : {"x", "y"})
        {
            auto x = e["position"][r];
            x.resetDataset(Dataset(determineDatatype<float>(), {extent}));
            std::vector<float> xd(extent);
            std::iota(xd.begin(), xd.end(), 0);
            x.storeChunk(xd);
            auto o = e["positionOffset"][r];
            o.resetDataset(Dataset(determineDatatype<uint64_t>(), {extent}));
            std::vector<uint64_t> od(extent);
            std::iota(od.begin(), od.end(), 0);
            o.storeChunk(od);
            s.flush();
        }

        auto const dset_n = Dataset(
            determineDatatype<uint64_t>(),
            {
                num_patches,
            });
        e.particlePatches["numParticles"][SCALAR].resetDataset(dset_n);
        e.particlePatches["numParticlesOffset"][SCALAR].resetDataset(dset_n);

        auto const dset_f = Dataset(
            determineDatatype<float>(),
            {
                num_patches,
            });
        e.particlePatches["offset"]["x"].resetDataset(dset_f);
        e.particlePatches["offset"]["y"].resetDataset(dset_f);
        e.particlePatches["extent"]["x"].resetDataset(dset_f);
        e.particlePatches["extent"]["y"].resetDataset(dset_f);

        // patch 0 (decomposed in x)
        e.particlePatches["numParticles"][SCALAR].store(0, uint64_t(10u));
        e.particlePatches["numParticlesOffset"][SCALAR].store(0, uint64_t(0u));
        e.particlePatches["offset"]["x"].store(0, float(0.));
        e.particlePatches["offset"]["y"].store(0, float(0.));
        e.particlePatches["extent"]["x"].store(0, float(10.));
        e.particlePatches["extent"]["y"].store(0, float(123.));

        // patch 1 (decomposed in x)
        e.particlePatches["numParticles"][SCALAR].store(1, uint64_t(113u));
        e.particlePatches["numParticlesOffset"][SCALAR].store(1, uint64_t(10u));
        e.particlePatches["offset"]["x"].store(1, float(10.));
        e.particlePatches["offset"]["y"].store(1, float(0.));
        e.particlePatches["extent"]["x"].store(1, float(113));
        e.particlePatches["extent"]["y"].store(1, float(123.));
    }
    {
        Series s = Series(
            "../samples/particle_patches%T." + file_ending, Access::READ_ONLY);

        auto e = s.iterations[42].particles["electrons"];

        auto numParticles =
            e.particlePatches["numParticles"][SCALAR].template load<uint64_t>();
        auto numParticlesOffset =
            e.particlePatches["numParticlesOffset"][SCALAR]
                .template load<uint64_t>();
        auto extent_x = e.particlePatches["extent"]["x"].template load<float>();
        auto extent_y = e.particlePatches["extent"]["y"].template load<float>();
        auto offset_x = e.particlePatches["offset"]["x"].template load<float>();
        auto offset_y = e.particlePatches["offset"]["y"].template load<float>();

        s.flush();

        REQUIRE(numParticles.get()[0] == 10);
        REQUIRE(numParticles.get()[1] == 113);
        REQUIRE(numParticlesOffset.get()[0] == 0);
        REQUIRE(numParticlesOffset.get()[1] == 10);
        REQUIRE(extent_x.get()[0] == 10.);
        REQUIRE(extent_x.get()[1] == 113.);
        REQUIRE(extent_y.get()[0] == 123.);
        REQUIRE(extent_y.get()[1] == 123.);
        REQUIRE(offset_x.get()[0] == 0.);
        REQUIRE(offset_x.get()[1] == 10.);
        REQUIRE(offset_y.get()[0] == 0.);
        REQUIRE(offset_y.get()[1] == 0.);
    }
}

TEST_CASE("particle_patches", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        particle_patches(t);
    }
}

inline void dtype_test(const std::string &backend)
{
    bool test_long_double = backend != "json" && backend != "toml";
    bool test_long_long = (backend != "json") || sizeof(long long) <= 8;
    {
        Series s = Series("../samples/dtype_test." + backend, Access::CREATE);

        char c = 'c';
        s.setAttribute("char", c);
        unsigned char uc = 'u';
        s.setAttribute("uchar", uc);
        signed char sc = 's';
        s.setAttribute("schar", sc);
        int16_t i16 = 16;
        s.setAttribute("int16", i16);
        int32_t i32 = 32;
        s.setAttribute("int32", i32);
        int64_t i64 = 64;
        s.setAttribute("int64", i64);
        uint16_t u16 = 16u;
        s.setAttribute("uint16", u16);
        uint32_t u32 = 32u;
        s.setAttribute("uint32", u32);
        uint64_t u64 = 64u;
        s.setAttribute("uint64", u64);
        float f = 16.e10f;
        s.setAttribute("float", f);
        double d = 1.e64;
        s.setAttribute("double", d);
        if (test_long_double)
        {
            long double ld = 1.e80L;
            s.setAttribute("longdouble", ld);
        }
        std::string str = "string";
        s.setAttribute("string", str);
        s.setAttribute("emptyString", "");
        s.setAttribute("vecChar", std::vector<char>({'c', 'h', 'a', 'r'}));
        s.setAttribute("vecInt16", std::vector<int16_t>({32766, 32767}));
        s.setAttribute(
            "vecInt32", std::vector<int32_t>({2147483646, 2147483647}));
        s.setAttribute(
            "vecInt64",
            std::vector<int64_t>({9223372036854775806, 9223372036854775807}));
        s.setAttribute(
            "vecUchar", std::vector<unsigned char>({'u', 'c', 'h', 'a', 'r'}));
        s.setAttribute(
            "vecSchar", std::vector<signed char>({'s', 'c', 'h', 'a', 'r'}));
        s.setAttribute("vecUint16", std::vector<uint16_t>({65534u, 65535u}));
        s.setAttribute(
            "vecUint32", std::vector<uint32_t>({4294967294u, 4294967295u}));
        s.setAttribute(
            "vecUint64",
            std::vector<uint64_t>(
                {18446744073709551614u, 18446744073709551615u}));
        s.setAttribute("vecFloat", std::vector<float>({0.f, 3.40282e+38f}));
        s.setAttribute("vecDouble", std::vector<double>({0., 1.79769e+308}));
        if (test_long_double)
        {
            s.setAttribute(
                "vecLongdouble",
                std::vector<long double>(
                    {0.L, std::numeric_limits<long double>::max()}));
        }
        s.setAttribute(
            "vecString", std::vector<std::string>({"vector", "of", "strings"}));
        s.setAttribute("vecEmptyString", std::vector<std::string>{"", "", ""});
        s.setAttribute(
            "vecMixedString", std::vector<std::string>{"hi", "", "ho"});
        s.setAttribute("bool", true);
        s.setAttribute("boolF", false);

        // non-fixed size integer types
        short ss = 16;
        s.setAttribute("short", ss);
        int si = 32;
        s.setAttribute("int", si);
        long sl = 64;
        s.setAttribute("long", sl);
        if (test_long_long)
        {
            long long sll = 128;
            s.setAttribute("longlong", sll);
        }
        unsigned short us = 16u;
        s.setAttribute("ushort", us);
        unsigned int ui = 32u;
        s.setAttribute("uint", ui);
        unsigned long ul = 64u;
        s.setAttribute("ulong", ul);
        if (test_long_long)
        {
            unsigned long long ull = 128u;
            s.setAttribute("ulonglong", ull);
        }
        s.setAttribute("vecShort", std::vector<short>({32766, 32767}));
        s.setAttribute("vecInt", std::vector<int>({32766, 32767}));
        s.setAttribute("vecLong", std::vector<long>({2147483646, 2147483647}));
        if (test_long_long)
        {
            s.setAttribute(
                "vecLongLong",
                std::vector<long long>({2147483644, 2147483643}));
        }
        s.setAttribute(
            "vecUShort", std::vector<unsigned short>({65534u, 65535u}));
        s.setAttribute("vecUInt", std::vector<unsigned int>({65533u, 65531u}));
        s.setAttribute(
            "vecULong", std::vector<unsigned long>({65532u, 65530u}));
        if (test_long_long)
        {
            s.setAttribute(
                "vecULongLong",
                std::vector<unsigned long long>({65531u, 65529u}));
        }

        // long double grid spacing
        // should be possible to parse without error upon opening
        // the series for reading
        {
            auto E = s.iterations[0].meshes["E"];
            E.setGridSpacing(std::vector<long double>{1.0, 1.0});
            auto E_x = E["x"];
            E_x.makeEmpty<double>(1);
        }
    }

    Series s = Series("../samples/dtype_test." + backend, Access::READ_ONLY);

    REQUIRE(s.getAttribute("char").get<char>() == 'c');
    REQUIRE(s.getAttribute("uchar").get<unsigned char>() == 'u');
    REQUIRE(s.getAttribute("schar").get<signed char>() == 's');
    REQUIRE(s.getAttribute("int16").get<int16_t>() == 16);
    REQUIRE(s.getAttribute("int32").get<int32_t>() == 32);
    REQUIRE(s.getAttribute("int64").get<int64_t>() == 64);
    REQUIRE(s.getAttribute("uint16").get<uint16_t>() == 16u);
    REQUIRE(s.getAttribute("uint32").get<uint32_t>() == 32u);
    REQUIRE(s.getAttribute("uint64").get<uint64_t>() == 64u);
    REQUIRE(s.getAttribute("float").get<float>() == 16.e10f);
    REQUIRE(s.getAttribute("double").get<double>() == 1.e64);
    if (test_long_double)
    {
        REQUIRE(s.getAttribute("longdouble").get<long double>() == 1.e80L);
    }
    REQUIRE(s.getAttribute("string").get<std::string>() == "string");
    REQUIRE(s.getAttribute("emptyString").get<std::string>().empty());
    REQUIRE(
        s.getAttribute("vecChar").get<std::vector<char>>() ==
        std::vector<char>({'c', 'h', 'a', 'r'}));
    REQUIRE(
        s.getAttribute("vecInt16").get<std::vector<int16_t>>() ==
        std::vector<int16_t>({32766, 32767}));
    REQUIRE(
        s.getAttribute("vecInt32").get<std::vector<int32_t>>() ==
        std::vector<int32_t>({2147483646, 2147483647}));
    REQUIRE(
        s.getAttribute("vecInt64").get<std::vector<int64_t>>() ==
        std::vector<int64_t>({9223372036854775806, 9223372036854775807}));
    REQUIRE(
        s.getAttribute("vecUchar").get<std::vector<unsigned char>>() ==
        std::vector<unsigned char>({'u', 'c', 'h', 'a', 'r'}));
    REQUIRE(
        s.getAttribute("vecSchar").get<std::vector<signed char>>() ==
        std::vector<signed char>({'s', 'c', 'h', 'a', 'r'}));
    REQUIRE(
        s.getAttribute("vecUint16").get<std::vector<uint16_t>>() ==
        std::vector<uint16_t>({65534u, 65535u}));
    REQUIRE(
        s.getAttribute("vecUint32").get<std::vector<uint32_t>>() ==
        std::vector<uint32_t>({4294967294u, 4294967295u}));
    REQUIRE(
        s.getAttribute("vecUint64").get<std::vector<uint64_t>>() ==
        std::vector<uint64_t>({18446744073709551614u, 18446744073709551615u}));
    REQUIRE(
        s.getAttribute("vecFloat").get<std::vector<float>>() ==
        std::vector<float>({0.f, 3.40282e+38f}));
    REQUIRE(
        s.getAttribute("vecDouble").get<std::vector<double>>() ==
        std::vector<double>({0., 1.79769e+308}));
    if (test_long_double)
    {
        REQUIRE(
            s.getAttribute("vecLongdouble").get<std::vector<long double>>() ==
            std::vector<long double>(
                {0.L, std::numeric_limits<long double>::max()}));
    }
    REQUIRE(
        s.getAttribute("vecString").get<std::vector<std::string>>() ==
        std::vector<std::string>({"vector", "of", "strings"}));
    REQUIRE(
        s.getAttribute("vecEmptyString").get<std::vector<std::string>>() ==
        std::vector<std::string>({"", "", ""}));
    REQUIRE(
        s.getAttribute("vecMixedString").get<std::vector<std::string>>() ==
        std::vector<std::string>({"hi", "", "ho"}));
    REQUIRE(s.getAttribute("bool").get<bool>() == true);
    REQUIRE(s.getAttribute("boolF").get<bool>() == false);

    // same implementation types (not necessary aliases) detection
#if !defined(_MSC_VER)
    REQUIRE(s.getAttribute("short").dtype == Datatype::SHORT);
    REQUIRE(s.getAttribute("int").dtype == Datatype::INT);
    REQUIRE(s.getAttribute("long").dtype == Datatype::LONG);
    if (test_long_long)
    {
        REQUIRE(s.getAttribute("longlong").dtype == Datatype::LONGLONG);
    }
    REQUIRE(s.getAttribute("ushort").dtype == Datatype::USHORT);
    REQUIRE(s.getAttribute("uint").dtype == Datatype::UINT);
    REQUIRE(s.getAttribute("ulong").dtype == Datatype::ULONG);
    if (test_long_long)
    {
        REQUIRE(s.getAttribute("ulonglong").dtype == Datatype::ULONGLONG);
    }

    REQUIRE(s.getAttribute("vecShort").dtype == Datatype::VEC_SHORT);
    REQUIRE(s.getAttribute("vecInt").dtype == Datatype::VEC_INT);
    REQUIRE(s.getAttribute("vecLong").dtype == Datatype::VEC_LONG);
    if (test_long_long)
    {
        REQUIRE(s.getAttribute("vecLongLong").dtype == Datatype::VEC_LONGLONG);
    }
    REQUIRE(s.getAttribute("vecUShort").dtype == Datatype::VEC_USHORT);
    REQUIRE(s.getAttribute("vecUInt").dtype == Datatype::VEC_UINT);
    REQUIRE(s.getAttribute("vecULong").dtype == Datatype::VEC_ULONG);
    if (test_long_long)
    {
        REQUIRE(
            s.getAttribute("vecULongLong").dtype == Datatype::VEC_ULONGLONG);
    }
#endif
    REQUIRE(isSame(s.getAttribute("short").dtype, Datatype::SHORT));
    REQUIRE(isSame(s.getAttribute("int").dtype, Datatype::INT));
    REQUIRE(isSame(s.getAttribute("long").dtype, Datatype::LONG));
    if (test_long_long)
    {
        REQUIRE(isSame(s.getAttribute("longlong").dtype, Datatype::LONGLONG));
    }
    REQUIRE(isSame(s.getAttribute("ushort").dtype, Datatype::USHORT));
    REQUIRE(isSame(s.getAttribute("uint").dtype, Datatype::UINT));
    REQUIRE(isSame(s.getAttribute("ulong").dtype, Datatype::ULONG));
    if (test_long_long)
    {
        REQUIRE(isSame(s.getAttribute("ulonglong").dtype, Datatype::ULONGLONG));
    }

    REQUIRE(isSame(s.getAttribute("vecShort").dtype, Datatype::VEC_SHORT));
    REQUIRE(isSame(s.getAttribute("vecInt").dtype, Datatype::VEC_INT));
    REQUIRE(isSame(s.getAttribute("vecLong").dtype, Datatype::VEC_LONG));
    if (test_long_long)
    {
        REQUIRE(isSame(
            s.getAttribute("vecLongLong").dtype, Datatype::VEC_LONGLONG));
    }
    REQUIRE(isSame(s.getAttribute("vecUShort").dtype, Datatype::VEC_USHORT));
    REQUIRE(isSame(s.getAttribute("vecUInt").dtype, Datatype::VEC_UINT));
    REQUIRE(isSame(s.getAttribute("vecULong").dtype, Datatype::VEC_ULONG));
    if (test_long_long)
    {
        REQUIRE(isSame(
            s.getAttribute("vecULongLong").dtype, Datatype::VEC_ULONGLONG));
    }
}

TEST_CASE("dtype_test", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        dtype_test(t);
    }
    if (auto extensions = getFileExtensions();
        std::find(extensions.begin(), extensions.end(), "toml") !=
        extensions.end())
    { /*
       * TOML backend is not generally tested for performance reasons, opt in to
       * testing it here.
       */
        dtype_test("toml");
    }
}

struct ReadFromAnyType
{
    template <typename T>
    static std::shared_ptr<void> call(RecordComponent &rc)
    {
        return std::static_pointer_cast<void>(rc.loadChunk<T>());
    }
};

inline void write_test(const std::string &backend)
{
#ifdef _WIN32
    std::string jsonCfg = "{}";
#else
    std::string jsonCfg = R"({"rank_table": "posix_hostname"})";
    chunk_assignment::RankMeta compare{
        {0,
         host_info::byMethod(
             host_info::methodFromStringDescription("posix_hostname", false))}};
#endif
    Series o =
        Series("../samples/serial_write." + backend, Access::CREATE, jsonCfg);

    ParticleSpecies &e_1 = o.iterations[1].particles["e"];

    std::vector<double> position_global(4);
    double pos{0.};
    std::generate(position_global.begin(), position_global.end(), [&pos] {
        return pos++;
    });
    std::shared_ptr<double> position_local_1(new double);
    e_1["position"]["x"].resetDataset(
        Dataset(determineDatatype(position_local_1), {4}));

    for (uint64_t i = 0; i < 4; ++i)
    {
        *position_local_1 = position_global[i];
        e_1["position"]["x"].storeChunk(position_local_1, {i}, {1});
    }

    std::vector<uint64_t> positionOffset_global(4);
    uint64_t posOff{0};
    std::generate(
        positionOffset_global.begin(), positionOffset_global.end(), [&posOff] {
            return posOff++;
        });
    std::shared_ptr<uint64_t> positionOffset_local_1(new uint64_t);
    e_1["positionOffset"]["x"].resetDataset(
        Dataset(determineDatatype(positionOffset_local_1), {4}));

    for (uint64_t i = 0; i < 4; ++i)
    {
        *positionOffset_local_1 = positionOffset_global[i];
        e_1["positionOffset"]["x"].storeChunk(positionOffset_local_1, {i}, {1});
    }

    ParticleSpecies &e_2 = o.iterations[2].particles["e"];

    std::generate(position_global.begin(), position_global.end(), [&pos] {
        return pos++;
    });
    std::shared_ptr<double> position_local_2(new double);
    e_2["position"]["x"].resetDataset(
        Dataset(determineDatatype(position_local_2), {4}));

    for (uint64_t i = 0; i < 4; ++i)
    {
        *position_local_2 = position_global[i];
        e_2["position"]["x"].storeChunk(position_local_2, {i}, {1});
    }

    std::generate(
        positionOffset_global.begin(), positionOffset_global.end(), [&posOff] {
            return posOff++;
        });
    std::shared_ptr<uint64_t> positionOffset_local_2(new uint64_t);
    e_2["positionOffset"]["x"].resetDataset(
        Dataset(determineDatatype(positionOffset_local_2), {4}));

    for (uint64_t i = 0; i < 4; ++i)
    {
        *positionOffset_local_2 = positionOffset_global[i];
        e_2["positionOffset"]["x"].storeChunk(positionOffset_local_2, {i}, {1});
    }

    o.flush();

    ParticleSpecies &e_3 = o.iterations[3].particles["e"];

    std::generate(position_global.begin(), position_global.end(), [&pos] {
        return pos++;
    });
    std::shared_ptr<double> position_local_3(new double);
    e_3["position"]["x"].resetDataset(
        Dataset(determineDatatype(position_local_3), {4}));

    for (uint64_t i = 0; i < 4; ++i)
    {
        *position_local_3 = position_global[i];
        e_3["position"]["x"].storeChunk(position_local_3, {i}, {1});
    }

    std::generate(
        positionOffset_global.begin(), positionOffset_global.end(), [&posOff] {
            return posOff++;
        });
    std::shared_ptr<uint64_t> positionOffset_local_3(new uint64_t);
    e_3["positionOffset"]["x"].resetDataset(
        Dataset(determineDatatype(positionOffset_local_3), {4}));

    for (uint64_t i = 0; i < 4; ++i)
    {
        *positionOffset_local_3 = positionOffset_global[i];
        e_3["positionOffset"]["x"].storeChunk(positionOffset_local_3, {i}, {1});
    }

    o.flush();

    o.close();

    Series read("../samples/serial_write." + backend, Access::READ_ONLY);
    auto rc = read.iterations[1].particles["e"]["position"]["x"];
    auto opaqueTypeDataset = rc.visit<ReadFromAnyType>();

    auto variantTypeDataset = rc.loadChunkVariant();
    rc.seriesFlush();
    std::visit(
        [](auto &&shared_ptr) {
            std::cout << "First value in loaded chunk: '" << shared_ptr.get()[0]
                      << '\'' << std::endl;
        },
        variantTypeDataset);

#ifndef _WIN32
    REQUIRE(read.rankTable(/* collective = */ false) == compare);
#endif
}

TEST_CASE("write_test", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        write_test(t);
        Series list{"../samples/serial_write." + t, Access::READ_ONLY};
        helper::listSeries(list);
    }
}

void test_complex(const std::string &backend)
{
    {
        Series o = Series(
            "../samples/serial_write_complex." + backend, Access::CREATE);
        o.setAttribute("lifeIsComplex", std::complex<double>(4.56, 7.89));
        o.setAttribute("butComplexFloats", std::complex<float>(42.3, -99.3));
        if (o.backend() != "ADIOS2")
            o.setAttribute(
                "longDoublesYouSay", std::complex<long double>(5.5, -4.55));

        auto Cflt = o.iterations[0].meshes["Cflt"][RecordComponent::SCALAR];
        std::vector<std::complex<float>> cfloats(3);
        cfloats.at(0) = {1., 2.};
        cfloats.at(1) = {-3., 4.};
        cfloats.at(2) = {5., -6.};
        Cflt.resetDataset(Dataset(Datatype::CFLOAT, {cfloats.size()}));
        Cflt.storeChunk(cfloats, {0});

        auto Cdbl = o.iterations[0].meshes["Cdbl"][RecordComponent::SCALAR];
        std::vector<std::complex<double>> cdoubles(3);
        cdoubles.at(0) = {2., 1.};
        cdoubles.at(1) = {-4., 3.};
        cdoubles.at(2) = {6., -5.};
        Cdbl.resetDataset(Dataset(Datatype::CDOUBLE, {cdoubles.size()}));
        Cdbl.storeChunk(cdoubles, {0});

        std::vector<std::complex<long double>> cldoubles(3);
        if (o.backend() != "ADIOS2")
        {
            auto Cldbl =
                o.iterations[0].meshes["Cldbl"][RecordComponent::SCALAR];
            cldoubles.at(0) = {3., 2.};
            cldoubles.at(1) = {-5., 4.};
            cldoubles.at(2) = {7., -6.};
            Cldbl.resetDataset(
                Dataset(Datatype::CLONG_DOUBLE, {cldoubles.size()}));
            Cldbl.storeChunk(cldoubles, {0});
        }

        o.flush();
    }

    {
        Series i = Series(
            "../samples/serial_write_complex." + backend, Access::READ_ONLY);
        REQUIRE(
            i.getAttribute("lifeIsComplex").get<std::complex<double>>() ==
            std::complex<double>(4.56, 7.89));
        REQUIRE(
            i.getAttribute("butComplexFloats").get<std::complex<float>>() ==
            std::complex<float>(42.3, -99.3));
        if (i.backend() != "ADIOS2")
        {
            REQUIRE(
                i.getAttribute("longDoublesYouSay")
                    .get<std::complex<long double>>() ==
                std::complex<long double>(5.5, -4.55));
        }

        auto rcflt = i.iterations[0]
                         .meshes["Cflt"] //[RecordComponent::SCALAR]
                         .loadChunk<std::complex<float>>();
        auto rcdbl = i.iterations[0]
                         .meshes["Cdbl"][RecordComponent::SCALAR]
                         .loadChunk<std::complex<double>>();
        i.flush();

        REQUIRE(rcflt.get()[1] == std::complex<float>(-3., 4.));
        REQUIRE(rcdbl.get()[2] == std::complex<double>(6, -5.));

        if (i.backend() != "ADIOS2")
        {
            auto rcldbl = i.iterations[0]
                              .meshes["Cldbl"][RecordComponent::SCALAR]
                              .loadChunk<std::complex<long double>>();
            i.flush();
            REQUIRE(rcldbl.get()[2] == std::complex<long double>(7., -6.));
        }
    }

    {
        Series list{
            "../samples/serial_write_complex." + backend, Access::READ_ONLY};
        helper::listSeries(list);
    }
}

TEST_CASE("test_complex", "[serial]")
{
    // Notes:
    // - ADIOS 2.7.0 has no complex long double
    // - JSON read-back not distinguishable yet from N+1 shaped data set
    for (auto const &t : testedFileExtensions())
    {
        test_complex(t);
    }
}

inline void
fileBased_add_EDpic(ParticleSpecies &e, uint64_t const num_particles)
{
    // ED-PIC
    e["position"].setAttribute("weightingPower", 0.0);
    e["position"].setAttribute("macroWeighted", uint32_t(0));
    e["positionOffset"].setAttribute("weightingPower", 0.0);
    e["positionOffset"].setAttribute("macroWeighted", uint32_t(0));

    auto const dsDbl = Dataset(Datatype::DOUBLE, {num_particles});
    e["momentum"]["x"].resetDataset(dsDbl);
    e["momentum"]["x"].makeConstant(1.2);
    e["momentum"].setAttribute("weightingPower", 1.0);
    e["momentum"].setAttribute("macroWeighted", uint32_t(0));

    e["charge"][RecordComponent::SCALAR].resetDataset(dsDbl);
    e["charge"][RecordComponent::SCALAR].makeConstant(2.3);
    e["charge"].setAttribute("weightingPower", 1.0);
    e["charge"].setAttribute("macroWeighted", uint32_t(0));

    e["mass"][RecordComponent::SCALAR].resetDataset(dsDbl);
    e["mass"][RecordComponent::SCALAR].makeConstant(3.4);
    e["mass"].setAttribute("weightingPower", 1.0);
    e["mass"].setAttribute("macroWeighted", uint32_t(0));

    e["weighting"][RecordComponent::SCALAR].resetDataset(dsDbl);
    e["weighting"][RecordComponent::SCALAR].makeConstant(1.0);
    e["weighting"].setAttribute("weightingPower", 1.0);
    e["weighting"].setAttribute("macroWeighted", uint32_t(1));

    e.setAttribute("particleShape", 3.0);
    e.setAttribute("currentDeposition", "Esirkepov");
    e.setAttribute("particlePush", "Boris");
    e.setAttribute("particleInterpolation", "uniform");
    e.setAttribute("particleSmoothing", "none");
}

inline void fileBased_write_test(const std::string &backend)
{
#ifdef _WIN32
    std::string jsonCfg = "{}";
#else
    std::string jsonCfg = R"({"rank_table": "posix_hostname"})";
#endif
    if (auxiliary::directory_exists("../samples/subdir"))
        auxiliary::remove_directory("../samples/subdir");

    {
        Series o = Series(
            "../samples/subdir/serial_fileBased_write%03T." + backend,
            Access::CREATE,
            jsonCfg);

        ParticleSpecies &e_1 = o.iterations[1].particles["e"];

        std::vector<double> position_global(4);
        double pos{0.};
        std::generate(position_global.begin(), position_global.end(), [&pos] {
            return pos++;
        });
        std::shared_ptr<double> position_local_1(new double);
        e_1["position"]["x"].resetDataset(
            Dataset(determineDatatype(position_local_1), {4}));
        std::vector<uint64_t> positionOffset_global(4);
        uint64_t posOff{0};
        std::generate(
            positionOffset_global.begin(),
            positionOffset_global.end(),
            [&posOff] { return posOff++; });
        std::shared_ptr<uint64_t> positionOffset_local_1(new uint64_t);
        e_1["positionOffset"]["x"].resetDataset(
            Dataset(determineDatatype(positionOffset_local_1), {4}));

        fileBased_add_EDpic(e_1, 4);

        for (uint64_t i = 0; i < 4; ++i)
        {
            *position_local_1 = position_global[i];
            e_1["position"]["x"].storeChunk(position_local_1, {i}, {1});
            *positionOffset_local_1 = positionOffset_global[i];
            e_1["positionOffset"]["x"].storeChunk(
                positionOffset_local_1, {i}, {1});
            o.flush();
        }

        o.iterations[1].setTime(static_cast<double>(1));

        ParticleSpecies &e_2 = o.iterations[2].particles["e"];

        std::generate(position_global.begin(), position_global.end(), [&pos] {
            return pos++;
        });
        e_2["position"]["x"].resetDataset(
            Dataset(determineDatatype<double>(), {4}));
        std::generate(
            positionOffset_global.begin(),
            positionOffset_global.end(),
            [&posOff] { return posOff++; });
        std::shared_ptr<uint64_t> positionOffset_local_2(new uint64_t);
        e_2["positionOffset"]["x"].resetDataset(
            Dataset(determineDatatype(positionOffset_local_2), {4}));

        fileBased_add_EDpic(e_2, 4);

        for (uint64_t i = 0; i < 4; ++i)
        {
            double const position_local_2 = position_global.at(i);
            e_2["position"]["x"].storeChunkRaw(&position_local_2, {i}, {1});
            *positionOffset_local_2 = positionOffset_global[i];
            e_2["positionOffset"]["x"].storeChunk(
                positionOffset_local_2, {i}, {1});
            o.flush();
        }

        o.iterations[2].setTime(static_cast<double>(2));

        ParticleSpecies &e_3 = o.iterations[3].particles["e"];

        std::generate(position_global.begin(), position_global.end(), [&pos] {
            return pos++;
        });
        std::shared_ptr<double> position_local_3(new double);
        e_3["position"]["x"].resetDataset(
            Dataset(determineDatatype(position_local_3), {4}));
        std::generate(
            positionOffset_global.begin(),
            positionOffset_global.end(),
            [&posOff] { return posOff++; });
        std::shared_ptr<uint64_t> positionOffset_local_3(new uint64_t);
        e_3["positionOffset"]["x"].resetDataset(
            Dataset(determineDatatype(positionOffset_local_3), {4}));

        fileBased_add_EDpic(e_3, 4);

        for (uint64_t i = 0; i < 4; ++i)
        {
            *position_local_3 = position_global[i];
            e_3["position"]["x"].storeChunk(position_local_3, {i}, {1});
            *positionOffset_local_3 = positionOffset_global[i];
            e_3["positionOffset"]["x"].storeChunk(
                positionOffset_local_3, {i}, {1});
            o.flush();
        }

        o.setOpenPMDextension(
            1); // this happens intentionally "late" in this test
        o.iterations[3].setTime(static_cast<double>(3));
        o.iterations[4].setTime(static_cast<double>(4));
        o.flush();
        o.iterations[5].setTime(static_cast<double>(5));
    }
    REQUIRE(
        (auxiliary::file_exists(
             "../samples/subdir/serial_fileBased_write001." + backend) ||
         auxiliary::directory_exists(
             "../samples/subdir/serial_fileBased_write001." + backend)));
    REQUIRE(
        (auxiliary::file_exists(
             "../samples/subdir/serial_fileBased_write002." + backend) ||
         auxiliary::directory_exists(
             "../samples/subdir/serial_fileBased_write002." + backend)));
    REQUIRE(
        (auxiliary::file_exists(
             "../samples/subdir/serial_fileBased_write003." + backend) ||
         auxiliary::directory_exists(
             "../samples/subdir/serial_fileBased_write003." + backend)));

    {
        Series o = Series(
            "../samples/subdir/serial_fileBased_write%T." + backend,
            Access::READ_ONLY,
            jsonCfg);

        REQUIRE(o.iterations.size() == 5);
        REQUIRE(o.iterations.count(1) == 1);
        REQUIRE(o.iterations.count(2) == 1);
        REQUIRE(o.iterations.count(3) == 1);
        REQUIRE(o.iterations.count(4) == 1);
        REQUIRE(o.iterations.count(5) == 1);

#if openPMD_USE_INVASIVE_TESTS
        REQUIRE(o.get().m_filenamePadding == 3);
#endif

        REQUIRE(o.basePath() == "/data/%T/");
        REQUIRE(o.iterationEncoding() == IterationEncoding::fileBased);
        REQUIRE(o.iterationFormat() == "serial_fileBased_write%03T");
        REQUIRE(o.openPMD() == "1.1.0");
        REQUIRE(o.openPMDextension() == 1u);
        REQUIRE(o.particlesPath() == "particles/");
        REQUIRE_FALSE(o.containsAttribute("meshesPath"));
        REQUIRE_THROWS_AS(o.meshesPath(), no_such_attribute_error);
        std::array<double, 7> udim{{1, 0, 0, 0, 0, 0, 0}};
        Extent ext{4};
        for (auto &entry : o.iterations)
        {
            auto &it = entry.second;
            REQUIRE(it.dt<double>() == 1.);
            REQUIRE(it.time<double>() == static_cast<double>(entry.first));
            REQUIRE(it.timeUnitSI() == 1.);

            if (entry.first > 3)
                continue; // empty iterations

            auto &pos = it.particles.at("e").at("position");
            REQUIRE(pos.timeOffset<float>() == 0.f);
            REQUIRE(pos.unitDimension() == udim);
            REQUIRE(!pos.scalar());
            auto &pos_x = pos.at("x");
            REQUIRE(pos_x.unitSI() == 1.);
            REQUIRE(pos_x.getExtent() == ext);
            REQUIRE(pos_x.getDatatype() == Datatype::DOUBLE);
            REQUIRE(!pos_x.constant());
            auto &posOff = it.particles.at("e").at("positionOffset");
            REQUIRE(posOff.timeOffset<float>() == 0.f);
            REQUIRE(posOff.unitDimension() == udim);
            auto &posOff_x = posOff.at("x");
            REQUIRE(posOff_x.unitSI() == 1.);
            REQUIRE(posOff_x.getExtent() == ext);
#if !defined(_MSC_VER)
            REQUIRE(posOff_x.getDatatype() == determineDatatype<uint64_t>());
#endif
            REQUIRE(
                isSame(posOff_x.getDatatype(), determineDatatype<uint64_t>()));

            auto position = pos_x.loadChunk<double>({0}, {4});
            auto position_raw = position.get();
            auto positionOffset = posOff_x.loadChunk<uint64_t>({0}, {4});
            auto positionOffset_raw = positionOffset.get();
            o.flush();
            for (uint64_t j = 0; j < 4; ++j)
            {
                REQUIRE(
                    position_raw[j] ==
                    static_cast<double>(j + (entry.first - 1) * 4));
                REQUIRE(positionOffset_raw[j] == j + (entry.first - 1) * 4);
            }
        }
        REQUIRE(o.iterations[3].time<double>() == 3.0);
        REQUIRE(o.iterations[4].time<double>() == 4.0);
        REQUIRE(o.iterations[5].time<double>() == 5.0);
    }

    {
        // extend existing series with new step and auto-detection of iteration
        // padding
        Series o = Series(
            "../samples/subdir/serial_fileBased_write%T." + backend,
            Access::READ_WRITE,
            jsonCfg);

        REQUIRE(o.iterations.size() == 5);
        o.iterations[6];
        REQUIRE(o.iterations.size() == 6);
        // write something to trigger opening of the file
        o.iterations[6].particles["e"]["position"]["x"].resetDataset(
            {Datatype::DOUBLE, {10}});
        o.iterations[6].particles["e"]["position"]["x"].makeConstant<double>(
            1.0);

        // additional iteration with over-running iteration padding but similar
        // content
        //                  padding:    000
        uint64_t const overlong_it = 123456;
        o.iterations[overlong_it];
        // write something to trigger opening of the file
        o.iterations[overlong_it].particles["e"]["position"]["x"].resetDataset(
            {Datatype::DOUBLE, {12}});
        o.iterations[overlong_it]
            .particles["e"]["position"]["x"]
            .makeConstant<double>(1.0);

        o.iterations[overlong_it].setTime(static_cast<double>(overlong_it));
        o.flush();
        REQUIRE(o.iterations.size() == 7);
    }
    REQUIRE(
        (auxiliary::file_exists(
             "../samples/subdir/serial_fileBased_write004." + backend) ||
         auxiliary::directory_exists(
             "../samples/subdir/serial_fileBased_write004." + backend)));
    REQUIRE(
        (auxiliary::file_exists(
             "../samples/subdir/serial_fileBased_write123456." + backend) ||
         auxiliary::directory_exists(
             "../samples/subdir/serial_fileBased_write123456." + backend)));

    // additional iteration with shorter iteration padding but similar content
    {
        Series o = Series(
            "../samples/subdir/serial_fileBased_write%01T." + backend,
            Access::READ_WRITE,
            jsonCfg);

        REQUIRE(o.iterations.size() == 1);
        /*
         * 123456 has no padding, it's just a very long number.
         * So even when opening the series with a padding of 1,
         * that iteration will be opened.
         */
        REQUIRE(o.iterations.count(123456) == 1);

        auto &it = o.iterations[10];
        ParticleSpecies &e = it.particles["e"];
        e["position"]["x"].resetDataset(Dataset(Datatype::DOUBLE, {42}));
        e["positionOffset"]["x"].resetDataset(Dataset(Datatype::DOUBLE, {42}));
        e["position"]["x"].makeConstant(1.23);
        e["positionOffset"]["x"].makeConstant(1.23);

        fileBased_add_EDpic(e, 42);
        it.setTime(static_cast<double>(10));

        REQUIRE(o.iterations.size() == 2);
    }
    REQUIRE(
        (auxiliary::file_exists(
             "../samples/subdir/serial_fileBased_write10." + backend) ||
         auxiliary::directory_exists(
             "../samples/subdir/serial_fileBased_write10." + backend)));

    // read back with auto-detection and non-fixed padding
    {
        Series s = Series(
            "../samples/subdir/serial_fileBased_write%T." + backend,
            Access::READ_ONLY);
        REQUIRE(s.iterations.size() == 8);
        REQUIRE(s.iterations.contains(4));
        REQUIRE(s.iterations.contains(10));
        REQUIRE(s.iterations.contains(123456));

        REQUIRE(s.iterations[3].time<double>() == 3.0);
        REQUIRE(s.iterations[4].time<double>() == 4.0);
        REQUIRE(s.iterations[5].time<double>() == 5.0);
        REQUIRE(s.iterations[10].time<double>() == 10.0);
        REQUIRE(s.iterations[123456].time<double>() == double(123456));
    }

    // write with auto-detection and in-consistent padding from step 10
    REQUIRE_THROWS_WITH(
        Series(
            "../samples/subdir/serial_fileBased_write%T." + backend,
            Access::READ_WRITE),
        Catch::Equals(
            "Cannot write to a series with inconsistent iteration padding. "
            "Please specify '%0<N>T' or open as read-only."));

    // read back with fixed padding
    {
        Series s = Series(
            "../samples/subdir/serial_fileBased_write%03T." + backend,
            Access::READ_ONLY);
        REQUIRE(s.iterations.size() == 7);
        REQUIRE(s.iterations.contains(4));
        REQUIRE(!s.iterations.contains(10));
        REQUIRE(s.iterations.contains(123456));

        REQUIRE(s.iterations[3].time<double>() == 3.0);
        REQUIRE(s.iterations[4].time<double>() == 4.0);
        REQUIRE(s.iterations[5].time<double>() == 5.0);
    }

    // read back with auto-detection (allow relaxed/overflow padding)
    {
        Series s = Series(
            "../samples/subdir/serial_fileBased_write%T." + backend,
            Access::READ_ONLY);
        REQUIRE(s.iterations.size() == 8);
        REQUIRE(s.iterations.contains(4));
        REQUIRE(s.iterations.contains(10));
        REQUIRE(s.iterations.contains(123456));

        REQUIRE(s.iterations[3].time<double>() == 3.0);
        REQUIRE(s.iterations[4].time<double>() == 4.0);
        REQUIRE(s.iterations[5].time<double>() == 5.0);
        REQUIRE(s.iterations[10].time<double>() == 10.0);
        REQUIRE(
            s.iterations[123456].time<double>() == static_cast<double>(123456));
    }

    {
        Series list{
            "../samples/subdir/serial_fileBased_write%03T." + backend,
            Access::READ_ONLY};
        helper::listSeries(list);
    }

#ifdef __unix__
    /*
     * Check that the ranktable was written correctly to every iteration file.
     */
    {
        int dirfd = open("../samples/subdir/", O_RDONLY);
        if (dirfd < 0)
        {
            throw std::system_error(
                std::error_code(errno, std::system_category()));
        }
        DIR *directory = fdopendir(dirfd);
        if (!directory)
        {
            close(dirfd);
            throw std::system_error(
                std::error_code(errno, std::system_category()));
        }
        chunk_assignment::RankMeta compare{{0, host_info::posix_hostname()}};
        dirent *entry;
        while ((entry = readdir(directory)) != nullptr)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0 ||
                !auxiliary::ends_with(entry->d_name, "." + backend))
            {
                continue;
            }
            std::string fullPath =
                std::string("../samples/subdir/") + entry->d_name;
            Series single_file(fullPath, Access::READ_ONLY);
            REQUIRE(single_file.rankTable(/* collective = */ false) == compare);
        }
        closedir(directory);
        close(dirfd);
    }
#endif // defined(__unix__)
}

TEST_CASE("fileBased_write_test", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        fileBased_write_test(t);
    }
    if (auto extensions = getFileExtensions();
        std::find(extensions.begin(), extensions.end(), "toml") !=
        extensions.end())
    { /*
       * TOML backend is not generally tested for performance reasons, opt in to
       * testing it here.
       */
        fileBased_write_test("toml");
    }
}

inline void sample_write_thetaMode(std::string const &file_ending)
{
    Series o = Series(
        std::string("../samples/thetaMode_%05T.").append(file_ending),
        Access::CREATE);

    unsigned int const num_modes = 4u;
    unsigned int const num_fields =
        1u + (num_modes - 1u) * 2u; // the first mode is purely real
    unsigned int const N_r = 20;
    unsigned int const N_z = 64;

    std::shared_ptr<float> E_r_data(
        new float[num_fields * N_r * N_z], [](float const *p) { delete[] p; });
    std::shared_ptr<double> E_t_data(
        new double[num_fields * N_r * N_z],
        [](double const *p) { delete[] p; });
    float e_r{0};
    std::generate(
        E_r_data.get(), E_r_data.get() + num_fields * N_r * N_z, [&e_r] {
            return e_r += 1.0f;
        });
    double e_t{100};
    std::generate(
        E_t_data.get(), E_t_data.get() + num_fields * N_r * N_z, [&e_t] {
            return e_t += 2.0;
        });

    std::stringstream geos;
    geos << "m=" << num_modes << ";imag=+";
    std::string const geometryParameters = geos.str();

    for (int i = 0; i <= 400; i += 100)
    {
        auto it = o.iterations[i];

        Mesh E = it.meshes["E"];
        E.setGeometry(Mesh::Geometry::thetaMode);
        E.setGeometryParameters(geometryParameters);
        E.setDataOrder(Mesh::DataOrder::C);
        E.setGridSpacing(std::vector<double>{1.0, 1.0});
        E.setGridGlobalOffset(std::vector<double>{0.0, 0.0});
        E.setGridUnitSI(1.0);
        E.setAxisLabels(std::vector<std::string>{"r", "z"});
        std::map<UnitDimension, double> const unitDimensions{
            {UnitDimension::I, 1.0}, {UnitDimension::J, 2.0}};
        E.setUnitDimension(unitDimensions);
        E.setTimeOffset(1.e-12 * double(i));

        auto E_z = E["z"];
        E_z.setUnitSI(10.);
        E_z.setPosition(std::vector<double>{0.0, 0.5});
        E_z.resetDataset(Dataset(
            Datatype::FLOAT,
            {num_fields, N_r, N_z})); // (modes, r, z) see setGeometryParameters
        E_z.makeConstant(static_cast<float>(42.54));

        // write all modes at once (otherwise iterate over modes and first
        // index)
        auto E_r = E["r"];
        E_r.setUnitSI(10.);
        E_r.setPosition(std::vector<double>{0.5, 0.0});
        E_r.resetDataset(Dataset(Datatype::FLOAT, {num_fields, N_r, N_z}));
        E_r.storeChunk(E_r_data, Offset{0, 0, 0}, Extent{num_fields, N_r, N_z});

        auto E_t = E["t"];
        E_t.setUnitSI(10.);
        E_t.setPosition(std::vector<double>{0.0, 0.0});
        E_t.resetDataset(Dataset(Datatype::DOUBLE, {num_fields, N_r, N_z}));
        E_t.storeChunk(E_t_data, Offset{0, 0, 0}, Extent{num_fields, N_r, N_z});

        o.flush();
    }
}

TEST_CASE("sample_write_thetaMode", "[serial][thetaMode]")
{
    for (auto const &t : testedFileExtensions())
    {
        sample_write_thetaMode(t);

        Series list{
            std::string("../samples/thetaMode_%05T.").append(t),
            Access::READ_ONLY};
        helper::listSeries(list);
    }
}

inline void bool_test(const std::string &backend)
{
    {
        Series o = Series("../samples/serial_bool." + backend, Access::CREATE);

        o.setAttribute("Bool attribute true", true);
        o.setAttribute("Bool attribute false", false);
    }
    {
        Series o =
            Series("../samples/serial_bool." + backend, Access::READ_ONLY);

        auto attrs = o.attributes();
        REQUIRE(
            std::count(attrs.begin(), attrs.end(), "Bool attribute true") == 1);
        REQUIRE(
            std::count(attrs.begin(), attrs.end(), "Bool attribute false") ==
            1);
        REQUIRE(o.getAttribute("Bool attribute true").get<bool>() == true);
        REQUIRE(o.getAttribute("Bool attribute false").get<bool>() == false);
    }
    {
        Series list{"../samples/serial_bool." + backend, Access::READ_ONLY};
        helper::listSeries(list);
    }
}

TEST_CASE("bool_test", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        bool_test(t);
    }
}

inline void patch_test(const std::string &backend)
{
    Series o = Series("../samples/serial_patch." + backend, Access::CREATE);

    auto e = o.iterations[1].particles["e"];

    uint64_t const num_particles = 1u;
    auto dset_d = Dataset(Datatype::DOUBLE, {num_particles});
    e["position"]["x"].resetDataset(dset_d);
    e["position"]["x"].makeConstant(20.0);
    e["positionOffset"]["x"].resetDataset(dset_d);
    e["positionOffset"]["x"].makeConstant(22.0);

    uint64_t const patch_idx = 0u;
    uint64_t const num_patches = 1u;
    auto const dset_n = Dataset(
        determineDatatype<uint64_t>(),
        {
            num_patches,
        });
    e.particlePatches["numParticles"][RecordComponent::SCALAR].resetDataset(
        dset_n);
    e.particlePatches["numParticles"][RecordComponent::SCALAR].store(
        patch_idx, num_particles);
    e.particlePatches["numParticlesOffset"][RecordComponent::SCALAR]
        .resetDataset(dset_n);
    e.particlePatches["numParticlesOffset"][RecordComponent::SCALAR].store(
        patch_idx, uint64_t(0u));

    auto const dset_f = Dataset(
        determineDatatype<float>(),
        {
            num_patches,
        });
    e.particlePatches["offset"]["x"].resetDataset(dset_f);
    e.particlePatches["offset"]["x"].store(patch_idx, 0.f);
    e.particlePatches["extent"]["x"].resetDataset(dset_f);
    e.particlePatches["extent"]["x"].store(patch_idx, 50.f);
}

TEST_CASE("patch_test", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        patch_test(t);

        Series list{"../samples/serial_patch." + t, Access::READ_ONLY};
        helper::listSeries(list);
    }
}

inline void deletion_test(const std::string &backend)
{
    Series o = Series("../samples/serial_deletion." + backend, Access::CREATE);

    o.setAttribute(
        "removed",
        "this attribute will be removed after being written to disk");
    o.flush();

    o.deleteAttribute("removed");
    o.flush();

    ParticleSpecies &e = o.iterations[1].particles["e"];
    auto dset = Dataset(Datatype::DOUBLE, {1});
    e["position"][RecordComponent::SCALAR].resetDataset(dset);
    e["position"][RecordComponent::SCALAR].makeConstant(20.0);
    e["positionOffset"][RecordComponent::SCALAR].resetDataset(dset);
    e["positionOffset"][RecordComponent::SCALAR].makeConstant(22.0);
    e.erase("deletion");
    e.seriesFlush();

    e["deletion_scalar"][RecordComponent::SCALAR].resetDataset(dset);
    o.flush();

    e["deletion_scalar"].erase(RecordComponent::SCALAR);
    e.erase("deletion_scalar");
    o.flush();

    e["deletion_scalar_two"][RecordComponent::SCALAR].resetDataset(dset);
    o.flush();

    e["deletion_scalar_two"].erase(
        e["deletion_scalar_two"].find(RecordComponent::SCALAR));
    e.erase(e.find("deletion_scalar_two"));
    o.flush();

    double value = 0.;
    e["deletion_scalar_constant"][RecordComponent::SCALAR].resetDataset(dset);
    e["deletion_scalar_constant"][RecordComponent::SCALAR].makeConstant(value);
    o.flush();

    e["deletion_scalar_constant"].erase(RecordComponent::SCALAR);
    e.erase("deletion_scalar_constant");
    o.flush();
}

TEST_CASE("deletion_test", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        if (t == "bp" || t == "bp4" || t == "bp5")
        {
            continue; // deletion not implemented in ADIOS2 backend
        }
        deletion_test(t);
    }
}

inline void read_missing_throw_test(const std::string &backend)
{
    try
    {
        auto s = Series(
            "this/does/definitely/not/exist." + backend, Access::READ_ONLY);
    }
    catch (...)
    {
        std::cout << "read missing: successfully caught! " << backend
                  << std::endl;
    }
}

TEST_CASE("read_missing_throw_test", "[serial]")
{
    for (auto const &t : testedFileExtensions())
        read_missing_throw_test(t);
}

inline void optional_paths_110_test(const std::string &backend)
{
    try
    {
        {
            Series s = Series(
                "../samples/issue-sample/no_fields/data%T." + backend,
                Access::READ_ONLY);
            auto attrs = s.attributes();
            REQUIRE(std::count(attrs.begin(), attrs.end(), "meshesPath") == 1);
            REQUIRE(
                std::count(attrs.begin(), attrs.end(), "particlesPath") == 1);
            REQUIRE(s.iterations[400].meshes.empty());
            REQUIRE(s.iterations[400].particles.size() == 1);
        }

        {
            Series s = Series(
                "../samples/issue-sample/no_particles/data%T." + backend,
                Access::READ_ONLY);
            auto attrs = s.attributes();
            REQUIRE(std::count(attrs.begin(), attrs.end(), "meshesPath") == 1);
            REQUIRE(
                std::count(attrs.begin(), attrs.end(), "particlesPath") == 1);
            REQUIRE(s.iterations[400].meshes.size() == 2);
            REQUIRE(s.iterations[400].particles.empty());
        }
    }
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "issue sample not accessible. (" << e.what() << ")\n";
        }
        else
        {
            throw;
        }
    }

    {
        Series s = Series(
            "../samples/no_meshes_1.1.0_compliant." + backend, Access::CREATE);
        auto foo = s.iterations[1].particles["foo"];
        Dataset dset = Dataset(Datatype::DOUBLE, {1});
        foo["position"][RecordComponent::SCALAR].resetDataset(dset);
        foo["position"][RecordComponent::SCALAR].makeConstant(20.0);
        foo["positionOffset"][RecordComponent::SCALAR].resetDataset(dset);
        foo["positionOffset"][RecordComponent::SCALAR].makeConstant(22.0);
    }

    {
        Series s = Series(
            "../samples/no_particles_1.1.0_compliant." + backend,
            Access::CREATE);
        auto foo = s.iterations[1].meshes["foo"];
        Dataset dset = Dataset(Datatype::DOUBLE, {1});
        foo[RecordComponent::SCALAR].resetDataset(dset);
    }

    {
        Series s = Series(
            "../samples/no_meshes_1.1.0_compliant." + backend,
            Access::READ_ONLY);
        auto attrs = s.attributes();
        REQUIRE(std::count(attrs.begin(), attrs.end(), "meshesPath") == 0);
        REQUIRE(std::count(attrs.begin(), attrs.end(), "particlesPath") == 1);
        REQUIRE(s.iterations[1].meshes.empty());
        REQUIRE(s.iterations[1].particles.size() == 1);
    }

    {
        Series s = Series(
            "../samples/no_particles_1.1.0_compliant." + backend,
            Access::READ_ONLY);
        auto attrs = s.attributes();
        REQUIRE(std::count(attrs.begin(), attrs.end(), "meshesPath") == 1);
        REQUIRE(std::count(attrs.begin(), attrs.end(), "particlesPath") == 0);
        REQUIRE(s.iterations[1].meshes.size() == 1);
        REQUIRE(s.iterations[1].particles.empty());
    }
}

void git_early_chunk_query(
    std::string const &filename,
    std::string const &species,
    int const step,
    std::string const &jsonConfig = "{}")
{
    try
    {
        Series s = Series(filename, Access::READ_ONLY, jsonConfig);

        auto electrons = s.iterations[step].particles[species];

        for (auto &r : electrons)
        {
            std::cout << r.first << ": ";
            for (auto &r_c : r.second)
            {
                std::cout << r_c.first << "\n";
                auto chunks = r_c.second.availableChunks();
                std::cout << "no. of chunks: " << chunks.size() << std::endl;
            }
        }
    }
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "git sample not accessible. (" << e.what() << ")\n";
            return;
        }
        throw;
    }
}

#if openPMD_HAVE_HDF5
TEST_CASE("empty_alternate_fbpic", "[serial][hdf5]")
{
    // Ref.: https://github.com/openPMD/openPMD-viewer/issues/296
    try
    {
        {
            Series s = Series(
                "../samples/issue-sample/empty_alternate_fbpic_%T.h5",
                Access::READ_ONLY);
            REQUIRE(s.iterations.contains(50));
            REQUIRE(s.iterations[50].particles.contains("electrons"));
            REQUIRE(
                s.iterations[50].particles["electrons"].contains("momentum"));
            REQUIRE(
                s.iterations[50].particles["electrons"]["momentum"].contains(
                    "x"));
            auto empty_rc =
                s.iterations[50].particles["electrons"]["momentum"]["x"];

            REQUIRE(empty_rc.empty());
            REQUIRE(empty_rc.getDimensionality() == 1);
            REQUIRE(empty_rc.getExtent() == Extent{0});
            REQUIRE(
                isSame(empty_rc.getDatatype(), determineDatatype<double>()));
        }
        {
            Series list{
                "../samples/issue-sample/empty_alternate_fbpic_%T.h5",
                Access::READ_ONLY};
            helper::listSeries(list);
        }
    }
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "issue sample not accessible. (" << e.what() << ")\n";
        }
        else
        {
            throw;
        }
    }
}

TEST_CASE("available_chunks_test_hdf5", "[serial][json]")
{
    /*
     * This test is HDF5 specific
     * HDF5 does not store chunks explicitly,
     * so the HDF5 backend will simply return the whole dataset as one chunk.
     *
     * Let's just write some random chunks and show that the HDF5 backend
     * does not care.
     */
    constexpr unsigned height = 10;
    std::string name = "../samples/available_chunks.h5";

    std::vector<int> data{2, 4, 6, 8};
    {
        Series write(name, Access::CREATE);
        Iteration it0 = write.iterations[0];
        auto E_x = it0.meshes["E"]["x"];
        E_x.resetDataset({Datatype::INT, {height, 4}});
        for (unsigned line = 2; line < 7; ++line)
        {
            E_x.storeChunk(data, {line, 0}, {1, 4});
        }
        for (unsigned line = 7; line < 9; ++line)
        {
            E_x.storeChunk(data, {line, 0}, {1, 2});
        }
        E_x.storeChunk(data, {8, 3}, {2, 1});
        it0.close();
    }

    {
        Series read(name, Access::READ_ONLY);
        Iteration it0 = read.iterations[0];
        auto E_x = it0.meshes["E"]["x"];
        ChunkTable table = E_x.availableChunks();
        REQUIRE(table.size() == 1);
        /*
         * Explicitly convert things to bool, so Catch doesn't get the splendid
         * idea to print the Chunk struct.
         */
        REQUIRE(bool(table[0] == WrittenChunkInfo({0, 0}, {height, 4})));
    }
}

TEST_CASE("optional_paths_110_test", "[serial]")
{
    optional_paths_110_test("h5"); // samples only present for hdf5
}

TEST_CASE("git_hdf5_sample_structure_test", "[serial][hdf5]")
{
#if openPMD_USE_INVASIVE_TESTS
    try
    {
        Series o = Series("../samples/git-sample/data%T.h5", Access::READ_ONLY);

        REQUIRE(!o.parent());
        REQUIRE(o.iterations.parent() == getWritable(&o));
        REQUIRE_THROWS_AS(o.iterations[42], std::out_of_range);
        REQUIRE(o.iterations[100].parent() == getWritable(&o.iterations));
        REQUIRE(
            o.iterations[100].meshes.parent() ==
            getWritable(&o.iterations[100]));
        REQUIRE(
            o.iterations[100].meshes["E"].parent() ==
            getWritable(&o.iterations[100].meshes));
        REQUIRE(
            o.iterations[100].meshes["E"]["x"].parent() ==
            getWritable(&o.iterations[100].meshes["E"]));
        REQUIRE(
            o.iterations[100].meshes["E"]["y"].parent() ==
            getWritable(&o.iterations[100].meshes["E"]));
        REQUIRE(
            o.iterations[100].meshes["E"]["z"].parent() ==
            getWritable(&o.iterations[100].meshes["E"]));
        REQUIRE(
            o.iterations[100].meshes["rho"].parent() ==
            getWritable(&o.iterations[100].meshes));
        REQUIRE(
            o.iterations[100]
                .meshes["rho"][MeshRecordComponent::SCALAR]
                .parent() == getWritable(&o.iterations[100].meshes));
        REQUIRE_THROWS_AS(
            o.iterations[100].meshes["cherries"], std::out_of_range);
        REQUIRE(
            o.iterations[100].particles.parent() ==
            getWritable(&o.iterations[100]));
        REQUIRE(
            o.iterations[100].particles["electrons"].parent() ==
            getWritable(&o.iterations[100].particles));
        REQUIRE(
            o.iterations[100].particles["electrons"]["charge"].parent() ==
            getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(
            o.iterations[100]
                .particles["electrons"]["charge"][RecordComponent::SCALAR]
                .parent() ==
            getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(
            o.iterations[100].particles["electrons"]["mass"].parent() ==
            getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(
            o.iterations[100]
                .particles["electrons"]["mass"][RecordComponent::SCALAR]
                .parent() ==
            getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(
            o.iterations[100].particles["electrons"]["momentum"].parent() ==
            getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(
            o.iterations[100]
                .particles["electrons"]["momentum"]["x"]
                .parent() ==
            getWritable(&o.iterations[100].particles["electrons"]["momentum"]));
        REQUIRE(
            o.iterations[100]
                .particles["electrons"]["momentum"]["y"]
                .parent() ==
            getWritable(&o.iterations[100].particles["electrons"]["momentum"]));
        REQUIRE(
            o.iterations[100]
                .particles["electrons"]["momentum"]["z"]
                .parent() ==
            getWritable(&o.iterations[100].particles["electrons"]["momentum"]));
        REQUIRE(
            o.iterations[100].particles["electrons"]["position"].parent() ==
            getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(
            o.iterations[100]
                .particles["electrons"]["position"]["x"]
                .parent() ==
            getWritable(&o.iterations[100].particles["electrons"]["position"]));
        REQUIRE(
            o.iterations[100]
                .particles["electrons"]["position"]["y"]
                .parent() ==
            getWritable(&o.iterations[100].particles["electrons"]["position"]));
        REQUIRE(
            o.iterations[100]
                .particles["electrons"]["position"]["z"]
                .parent() ==
            getWritable(&o.iterations[100].particles["electrons"]["position"]));
        REQUIRE(
            o.iterations[100]
                .particles["electrons"]["positionOffset"]
                .parent() ==
            getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(
            o.iterations[100]
                .particles["electrons"]["positionOffset"]["x"]
                .parent() ==
            getWritable(
                &o.iterations[100].particles["electrons"]["positionOffset"]));
        REQUIRE(
            o.iterations[100]
                .particles["electrons"]["positionOffset"]["y"]
                .parent() ==
            getWritable(
                &o.iterations[100].particles["electrons"]["positionOffset"]));
        REQUIRE(
            o.iterations[100]
                .particles["electrons"]["positionOffset"]["z"]
                .parent() ==
            getWritable(
                &o.iterations[100].particles["electrons"]["positionOffset"]));
        REQUIRE(
            o.iterations[100].particles["electrons"]["weighting"].parent() ==
            getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(
            o.iterations[100]
                .particles["electrons"]["weighting"][RecordComponent::SCALAR]
                .parent() ==
            getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE_THROWS_AS(
            o.iterations[100].particles["electrons"]["numberOfLegs"],
            std::out_of_range);
        REQUIRE_THROWS_AS(
            o.iterations[100].particles["apples"], std::out_of_range);

        int32_t i32 = 32;
        REQUIRE_THROWS(o.setAttribute("setAttributeFail", i32));
    }
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "git sample not accessible. (" << e.what() << ")\n";
            return;
        }
        throw;
    }
#else
    std::cerr << "Invasive tests not enabled. Hierarchy is not visible.\n";
#endif
}

namespace
{
struct LoadDataset
{
    template <typename T>
    static void call(RecordComponent &rc)
    {
        auto chunk = rc.loadChunk<T>();
        rc.seriesFlush();
    }

    static constexpr char const *errorMsg = "LoadDataset";
};
} // namespace

TEST_CASE("git_hdf5_legacy_picongpu", "[serial][hdf5]")
{
    try
    {
        Series o = Series(
            "../samples/git-sample/legacy/simData_%T.h5", Access::READ_ONLY);

        /*
         * That dataset was written directly via HDF5 (not the openPMD-api)
         * and had two issues:
         *
         * 1) No unitSI defined for numParticles and numParticlesOffset.
         *    unitSI does not really make sense there, but the openPMD-standard
         *    is not quite clear if it is required, so the API writes it and
         *    also required it. We will keep writing it, but we don't require
         *    it any longer.
         * 2) A custom enum was used for writing a boolean dataset.
         *    At the least, the dataset should be skipped in parsing instead
         *    of failing the entire procedure. Ideally, the custom datatype
         *    should be upcasted to char type and treated as such.
         */

        auto radiationMask =
            o.iterations[200]
                .particles["e"]["radiationMask"][RecordComponent::SCALAR];
        switchNonVectorType<LoadDataset>(
            radiationMask.getDatatype(), radiationMask);

        auto particlePatches = o.iterations[200].particles["e"].particlePatches;
        REQUIRE(particlePatches.size() == 4);
        for (auto key : {"extent", "offset"})
        {
            REQUIRE(particlePatches.contains(key));
            REQUIRE(particlePatches.at(key).size() == 3);
            for (auto subkey : {"x", "y", "z"})
            {
                REQUIRE(particlePatches.at(key).contains(subkey));
                // unitSI is present in those records
                particlePatches.at(key).at(subkey).unitSI();
            }
        }
        for (auto key : {"numParticles", "numParticlesOffset"})
        {
            REQUIRE(particlePatches.contains(key));
            REQUIRE(particlePatches.at(key).contains(RecordComponent::SCALAR));
            // unitSI is not present in those records
            REQUIRE_THROWS_AS(
                particlePatches.at(key).at(RecordComponent::SCALAR).unitSI(),
                no_such_attribute_error);
        }

        helper::listSeries(o, true, std::cout);
    }
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "git sample not accessible. (" << e.what() << ")\n";
            return;
        }
        throw;
    }
}

TEST_CASE("git_hdf5_sample_attribute_test", "[serial][hdf5]")
{
    try
    {
        Series o = Series("../samples/git-sample/data%T.h5", Access::READ_ONLY);

        REQUIRE(o.openPMD() == "1.1.0");
        REQUIRE(o.openPMDextension() == 1);
        REQUIRE(o.basePath() == "/data/%T/");
        REQUIRE(o.meshesPath() == "fields/");
        REQUIRE(o.particlesPath() == "particles/");
        REQUIRE(o.iterationEncoding() == IterationEncoding::fileBased);
        REQUIRE(o.iterationFormat() == "data%T.h5");
        REQUIRE(o.name() == "data%T");

        REQUIRE(o.iterations.size() == 5);
        REQUIRE(o.iterations.count(100) == 1);

        Iteration &iteration_100 = o.iterations[100];
        REQUIRE(iteration_100.time<double>() == 3.2847121452090077e-14);
        REQUIRE(iteration_100.dt<double>() == 3.2847121452090093e-16);
        REQUIRE(iteration_100.timeUnitSI() == 1.0);

        REQUIRE(iteration_100.meshes.size() == 2);
        REQUIRE(iteration_100.meshes.count("E") == 1);
        REQUIRE(iteration_100.meshes.count("rho") == 1);

        std::vector<std::string> al{"x", "y", "z"};
        std::vector<double> gs{
            8.0000000000000007e-07,
            8.0000000000000007e-07,
            1.0000000000000001e-07};
        std::vector<double> ggo{
            -1.0000000000000001e-05,
            -1.0000000000000001e-05,
            -5.1999999999999993e-06};
        std::array<double, 7> ud{{1., 1., -3., -1., 0., 0., 0.}};
        Mesh &E = iteration_100.meshes["E"];
        REQUIRE(E.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(E.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(E.axisLabels() == al);
        REQUIRE(E.gridSpacing<double>() == gs);
        REQUIRE(E.gridGlobalOffset() == ggo);
        REQUIRE(E.gridUnitSI() == 1.0);
        REQUIRE(E.unitDimension() == ud);
        REQUIRE(E.timeOffset<double>() == 0.0);

        REQUIRE(E.size() == 3);
        REQUIRE(E.count("x") == 1);
        REQUIRE(E.count("y") == 1);
        REQUIRE(E.count("z") == 1);

        std::vector<double> p{0.5, 0., 0.};
        Extent e{26, 26, 201};
        MeshRecordComponent &E_x = E["x"];
        REQUIRE(E_x.unitSI() == 1.0);
        REQUIRE(E_x.position<double>() == p);
        REQUIRE(E_x.getDatatype() == Datatype::DOUBLE);
        REQUIRE(E_x.getExtent() == e);
        REQUIRE(E_x.getDimensionality() == 3);

        p = {0., 0.5, 0.};
        MeshRecordComponent &E_y = E["y"];
        REQUIRE(E_y.unitSI() == 1.0);
        REQUIRE(E_y.position<double>() == p);
        REQUIRE(E_y.getDatatype() == Datatype::DOUBLE);
        REQUIRE(E_y.getExtent() == e);
        REQUIRE(E_y.getDimensionality() == 3);

        p = {0., 0., 0.5};
        MeshRecordComponent &E_z = E["z"];
        REQUIRE(E_z.unitSI() == 1.0);
        REQUIRE(E_z.position<double>() == p);
        REQUIRE(E_z.getDatatype() == Datatype::DOUBLE);
        REQUIRE(E_z.getExtent() == e);
        REQUIRE(E_z.getDimensionality() == 3);

        gs = {
            8.0000000000000007e-07,
            8.0000000000000007e-07,
            1.0000000000000001e-07};
        ggo = {
            -1.0000000000000001e-05,
            -1.0000000000000001e-05,
            -5.1999999999999993e-06};
        ud = {{-3., 0., 1., 1., 0., 0., 0.}};
        Mesh &rho = iteration_100.meshes["rho"];
        REQUIRE(rho.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(rho.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(rho.axisLabels() == al);
        REQUIRE(rho.gridSpacing<double>() == gs);
        REQUIRE(rho.gridGlobalOffset() == ggo);
        REQUIRE(rho.gridUnitSI() == 1.0);
        REQUIRE(rho.unitDimension() == ud);
        REQUIRE(rho.timeOffset<double>() == 0.0);

        REQUIRE(rho.size() == 1);
        REQUIRE(rho.count(MeshRecordComponent::SCALAR) == 1);

        p = {0., 0., 0.};
        e = {26, 26, 201};
        MeshRecordComponent &rho_scalar = rho[MeshRecordComponent::SCALAR];
        REQUIRE(rho_scalar.unitSI() == 1.0);
        REQUIRE(rho_scalar.position<double>() == p);
        REQUIRE(rho_scalar.getDatatype() == Datatype::DOUBLE);
        REQUIRE(rho_scalar.getExtent() == e);
        REQUIRE(rho_scalar.getDimensionality() == 3);

        REQUIRE(iteration_100.particles.size() == 1);
        REQUIRE(iteration_100.particles.count("electrons") == 1);

        ParticleSpecies &electrons = iteration_100.particles["electrons"];

        REQUIRE(electrons.size() == 6);
        REQUIRE(electrons.count("charge") == 1);
        REQUIRE(electrons.count("mass") == 1);
        REQUIRE(electrons.count("momentum") == 1);
        REQUIRE(electrons.count("position") == 1);
        REQUIRE(electrons.count("positionOffset") == 1);
        REQUIRE(electrons.count("weighting") == 1);

        ud = {{0., 0., 1., 1., 0., 0., 0.}};
        Record &charge = electrons["charge"];
        REQUIRE(charge.unitDimension() == ud);
        REQUIRE(charge.timeOffset<double>() == 0.0);

        REQUIRE(charge.size() == 1);
        REQUIRE(charge.count(RecordComponent::SCALAR) == 1);

        e = {85625};
        RecordComponent &charge_scalar = charge[RecordComponent::SCALAR];
        REQUIRE(charge_scalar.unitSI() == 1.0);
        REQUIRE(charge_scalar.getDatatype() == Datatype::DOUBLE);
        REQUIRE(charge_scalar.getDimensionality() == 1);
        REQUIRE(charge_scalar.getExtent() == e);

        ud = {{1., 0., 0., 0., 0., 0., 0.}};
        Record &mass = electrons["mass"];
        REQUIRE(mass.unitDimension() == ud);
        REQUIRE(mass.timeOffset<double>() == 0.0);

        REQUIRE(mass.size() == 1);
        REQUIRE(mass.count(RecordComponent::SCALAR) == 1);

        RecordComponent &mass_scalar = mass[RecordComponent::SCALAR];
        REQUIRE(mass_scalar.unitSI() == 1.0);
        REQUIRE(mass_scalar.getDatatype() == Datatype::DOUBLE);
        REQUIRE(mass_scalar.getDimensionality() == 1);
        REQUIRE(mass_scalar.getExtent() == e);

        ud = {{1., 1., -1., 0., 0., 0., 0.}};
        Record &momentum = electrons["momentum"];
        REQUIRE(momentum.unitDimension() == ud);
        REQUIRE(momentum.timeOffset<double>() == 0.0);

        REQUIRE(momentum.size() == 3);
        REQUIRE(momentum.count("x") == 1);
        REQUIRE(momentum.count("y") == 1);
        REQUIRE(momentum.count("z") == 1);

        RecordComponent &momentum_x = momentum["x"];
        REQUIRE(momentum_x.unitSI() == 1.0);
        REQUIRE(momentum_x.getDatatype() == Datatype::DOUBLE);
        REQUIRE(momentum_x.getDimensionality() == 1);
        REQUIRE(momentum_x.getExtent() == e);

        RecordComponent &momentum_y = momentum["y"];
        REQUIRE(momentum_y.unitSI() == 1.0);
        REQUIRE(momentum_y.getDatatype() == Datatype::DOUBLE);
        REQUIRE(momentum_y.getDimensionality() == 1);
        REQUIRE(momentum_y.getExtent() == e);

        RecordComponent &momentum_z = momentum["z"];
        REQUIRE(momentum_z.unitSI() == 1.0);
        REQUIRE(momentum_z.getDatatype() == Datatype::DOUBLE);
        REQUIRE(momentum_z.getDimensionality() == 1);
        REQUIRE(momentum_z.getExtent() == e);

        ud = {{1., 0., 0., 0., 0., 0., 0.}};
        Record &position = electrons["position"];
        REQUIRE(position.unitDimension() == ud);
        REQUIRE(position.timeOffset<double>() == 0.0);

        REQUIRE(position.size() == 3);
        REQUIRE(position.count("x") == 1);
        REQUIRE(position.count("y") == 1);
        REQUIRE(position.count("z") == 1);

        RecordComponent &position_x = position["x"];
        REQUIRE(position_x.unitSI() == 1.0);
        REQUIRE(position_x.getDatatype() == Datatype::DOUBLE);
        REQUIRE(position_x.getDimensionality() == 1);
        REQUIRE(position_x.getExtent() == e);

        RecordComponent &position_y = position["y"];
        REQUIRE(position_y.unitSI() == 1.0);
        REQUIRE(position_y.getDatatype() == Datatype::DOUBLE);
        REQUIRE(position_y.getDimensionality() == 1);
        REQUIRE(position_y.getExtent() == e);

        RecordComponent &position_z = position["z"];
        REQUIRE(position_z.unitSI() == 1.0);
        REQUIRE(position_z.getDatatype() == Datatype::DOUBLE);
        REQUIRE(position_z.getDimensionality() == 1);
        REQUIRE(position_z.getExtent() == e);

        Record &positionOffset = electrons["positionOffset"];
        REQUIRE(positionOffset.unitDimension() == ud);
        REQUIRE(positionOffset.timeOffset<double>() == 0.0);

        REQUIRE(positionOffset.size() == 3);
        REQUIRE(positionOffset.count("x") == 1);
        REQUIRE(positionOffset.count("y") == 1);
        REQUIRE(positionOffset.count("z") == 1);

        RecordComponent &positionOffset_x = positionOffset["x"];
        REQUIRE(positionOffset_x.unitSI() == 1.0);
        REQUIRE(positionOffset_x.getDatatype() == Datatype::DOUBLE);
        REQUIRE(positionOffset_x.getDimensionality() == 1);
        REQUIRE(positionOffset_x.getExtent() == e);

        RecordComponent &positionOffset_y = positionOffset["y"];
        REQUIRE(positionOffset_y.unitSI() == 1.0);
        REQUIRE(positionOffset_y.getDatatype() == Datatype::DOUBLE);
        REQUIRE(positionOffset_y.getDimensionality() == 1);
        REQUIRE(positionOffset_y.getExtent() == e);

        RecordComponent &positionOffset_z = positionOffset["z"];
        REQUIRE(positionOffset_z.unitSI() == 1.0);
        REQUIRE(positionOffset_z.getDatatype() == Datatype::DOUBLE);
        REQUIRE(positionOffset_z.getDimensionality() == 1);
        REQUIRE(positionOffset_z.getExtent() == e);

        ud = {{0., 0., 0., 0., 0., 0., 0.}};
        Record &weighting = electrons["weighting"];
        REQUIRE(weighting.unitDimension() == ud);
        REQUIRE(weighting.timeOffset<double>() == 0.0);

        REQUIRE(weighting.size() == 1);
        REQUIRE(weighting.count(RecordComponent::SCALAR) == 1);

        RecordComponent &weighting_scalar = weighting[RecordComponent::SCALAR];
        REQUIRE(weighting_scalar.unitSI() == 1.0);
        REQUIRE(weighting_scalar.getDatatype() == Datatype::DOUBLE);
        REQUIRE(weighting_scalar.getDimensionality() == 1);
        REQUIRE(weighting_scalar.getExtent() == e);
    }
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "git sample not accessible. (" << e.what() << ")\n";
            return;
        }
        throw;
    }
}

TEST_CASE("git_hdf5_sample_content_test", "[serial][hdf5]")
{
    try
    {
        Series o = Series("../samples/git-sample/data%T.h5", Access::READ_ONLY);

        {
            double actual[3][3][3] = {
                {{-1.9080703683727052e-09,
                  -1.5632650729457964e-10,
                  1.1497536256399599e-09},
                 {-1.9979540244463578e-09,
                  -2.5512036927466397e-10,
                  1.0402234629225404e-09},
                 {-1.7353589676361025e-09,
                  -8.0899198451334087e-10,
                  -1.6443779671249104e-10}},

                {{-2.0029988778702545e-09,
                  -1.9543477947081556e-10,
                  1.0916454407094989e-09},
                 {-2.3890367462087170e-09,
                  -4.7158010829662089e-10,
                  9.0026075483251589e-10},
                 {-1.9033881137886510e-09,
                  -7.5192119197708962e-10,
                  5.0038861942880430e-10}},

                {{-1.3271805876513554e-09,
                  -5.9243276950837753e-10,
                  -2.2445734160214670e-10},
                 {-7.4578609954301101e-10,
                  -1.1995737736469891e-10,
                  2.5611823772919706e-10},
                 {-9.4806251738077663e-10,
                  -1.5472800818372434e-10,
                  -3.6461900165818406e-10}}};
            Mesh rhoMesh = o.iterations[100].meshes["rho"];
            MeshRecordComponent rho = rhoMesh[MeshRecordComponent::SCALAR];
            Offset offset{20, 20, 190};
            Extent extent{3, 3, 3};
            auto data = rho.loadChunk<double>(offset, extent);
            rhoMesh.seriesFlush();
            double *raw_ptr = data.get();

            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    for (int k = 0; k < 3; ++k)
                        REQUIRE(
                            raw_ptr[((i * 3) + j) * 3 + k] == actual[i][j][k]);
        }

        {
            double constant_value = 9.1093829099999999e-31;
            RecordComponent &electrons_mass =
                o.iterations[100]
                    .particles["electrons"]["mass"][RecordComponent::SCALAR];
            Offset offset{15};
            Extent extent{3};
            auto data = electrons_mass.loadChunk<double>(offset, extent);
            o.flush();
            double *raw_ptr = data.get();

            for (int i = 0; i < 3; ++i)
                REQUIRE(raw_ptr[i] == constant_value);
        }
    }
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "git sample not accessible. (" << e.what() << ")\n";
            return;
        }
        throw;
    }
}

TEST_CASE("git_hdf5_sample_fileBased_read_test", "[serial][hdf5]")
{
    try
    {
        Series o = Series("../samples/git-sample/data%T.h5", Access::READ_ONLY);

        REQUIRE(o.iterations.size() == 5);
        REQUIRE(o.iterations.count(100) == 1);
        REQUIRE(o.iterations.count(200) == 1);
        REQUIRE(o.iterations.count(300) == 1);
        REQUIRE(o.iterations.count(400) == 1);
        REQUIRE(o.iterations.count(500) == 1);

#if openPMD_USE_INVASIVE_TESTS
        REQUIRE(o.get().m_filenamePadding == 8);
#endif
    }
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible &&
            e.affectedObject == error::AffectedObject::File)
        {
            std::cerr << "git sample not accessible. (" << e.what() << ")\n";
            return;
        }
        throw;
    }

    try
    {
        Series o =
            Series("../samples/git-sample/data%08T.h5", Access::READ_ONLY);

        REQUIRE(o.iterations.size() == 5);
        REQUIRE(o.iterations.count(100) == 1);
        REQUIRE(o.iterations.count(200) == 1);
        REQUIRE(o.iterations.count(300) == 1);
        REQUIRE(o.iterations.count(400) == 1);
        REQUIRE(o.iterations.count(500) == 1);

#if openPMD_USE_INVASIVE_TESTS
        REQUIRE(o.get().m_filenamePadding == 8);
#endif
    }
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "git sample not accessible. (" << e.what() << ")\n";
            return;
        }
        throw;
    }

    REQUIRE_THROWS_AS(
        Series("../samples/git-sample/data%07T.h5", Access::READ_ONLY),
        error::ReadError);

    try
    {
        std::vector<std::string> newFiles{
            "../samples/git-sample/data00000001.h5",
            "../samples/git-sample/data00000010.h5",
            "../samples/git-sample/data00001000.h5",
            "../samples/git-sample/data00010000.h5",
            "../samples/git-sample/data00100000.h5"};

        for (auto const &file : newFiles)
            if (auxiliary::file_exists(file))
                auxiliary::remove_file(file);

        {
            Series o =
                Series("../samples/git-sample/data%T.h5", Access::READ_WRITE);

#if openPMD_USE_INVASIVE_TESTS
            REQUIRE(o.get().m_filenamePadding == 8);
#endif

            o.iterations[1];
            o.iterations[10];
            o.iterations[1000];
            o.iterations[10000];
            o.iterations[100000];
            o.flush();
        }

        for (auto const &file : newFiles)
        {
            REQUIRE(auxiliary::file_exists(file));
            auxiliary::remove_file(file);
        }
    }
    // use no_such_file_error here to check that the backward-compatibility
    // alias works
    catch (no_such_file_error &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "git sample not accessible. (" << e.what() << ")\n";
            return;
        }
        throw;
    }
}

TEST_CASE("git_hdf5_early_chunk_query", "[serial][hdf5]")
{
    git_early_chunk_query("../samples/git-sample/data%T.h5", "electrons", 400);
}

TEST_CASE("git_hdf5_sample_read_thetaMode", "[serial][hdf5][thetaMode]")
{
    try
    {
        Series o = Series(
            "../samples/git-sample/thetaMode/data%T.h5", Access::READ_ONLY);

        REQUIRE(o.iterations.size() == 5);
        REQUIRE(o.iterations.count(100) == 1);
        REQUIRE(o.iterations.count(200) == 1);
        REQUIRE(o.iterations.count(300) == 1);
        REQUIRE(o.iterations.count(400) == 1);
        REQUIRE(o.iterations.count(500) == 1);

        auto i = o.iterations[500];

        REQUIRE(i.meshes.size() == 4);
        REQUIRE(i.meshes.count("B") == 1);
        REQUIRE(i.meshes.count("E") == 1);
        REQUIRE(i.meshes.count("J") == 1);
        REQUIRE(i.meshes.count("rho") == 1);

        Mesh B = i.meshes["B"];
        std::vector<std::string> const al{"r", "z"};
        std::vector<double> const gs{3.e-7, 1.e-7};
        std::vector<double> const ggo{0., 3.02e-5};
        std::array<double, 7> const ud{{0., 1., -2., -1., 0., 0., 0.}};
        REQUIRE(B.geometry() == Mesh::Geometry::thetaMode);
        REQUIRE(B.geometryParameters() == "m=2;imag=+");
        REQUIRE(B.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(B.axisLabels() == al);
        REQUIRE(B.gridSpacing<double>().size() == 2u);
        REQUIRE(B.gridGlobalOffset().size() == 2u);
        REQUIRE(
            std::abs(B.gridSpacing<double>()[0] - gs[0]) <=
            std::numeric_limits<double>::epsilon());
        REQUIRE(
            std::abs(B.gridSpacing<double>()[1] - gs[1]) <=
            std::numeric_limits<double>::epsilon());
        REQUIRE(
            std::abs(B.gridGlobalOffset()[0] - ggo[0]) <=
            std::numeric_limits<double>::epsilon());
        REQUIRE(
            std::abs(B.gridGlobalOffset()[1] - ggo[1]) <=
            std::numeric_limits<double>::epsilon());
        REQUIRE(B.gridUnitSI() == 1.0);
        REQUIRE(B.unitDimension() == ud);
        REQUIRE(B.timeOffset<double>() == static_cast<double>(0.0f));

        REQUIRE(B.size() == 3);
        REQUIRE(B.count("r") == 1);
        REQUIRE(B.count("t") == 1);
        REQUIRE(B.count("z") == 1);

        MeshRecordComponent B_z = B["z"];
        std::vector<double> const pos{0.5, 0.0};
        Extent const ext{3, 51, 201};
        REQUIRE(B_z.unitSI() == 1.0);
        REQUIRE(B_z.position<double>() == pos);
        REQUIRE(B_z.getDatatype() == Datatype::DOUBLE);
        REQUIRE(B_z.getExtent() == ext);
        REQUIRE(B_z.getDimensionality() == 3);

        Offset const offset{1, 10, 90}; // skip mode_0 (one scalar field)
        Extent const extent{2, 30, 20}; // mode_1 (two scalar fields)
        auto data = B_z.loadChunk<double>(offset, extent);
        o.flush();
    }
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "git sample not accessible. (" << e.what() << ")\n";
            return;
        }
        throw;
    }
}

TEST_CASE("hzdr_hdf5_sample_content_test", "[serial][hdf5]")
{
    // since this file might not be publicly available, gracefully handle errors
    try
    {
        /* HZDR: /bigdata/hplsim/development/huebl/lwfa-openPMD-062-smallLWFA-h5
         * DOI:10.14278/rodare.57 */
        Series o = Series(
            "../samples/hzdr-sample/h5/simData_%T.h5", Access::READ_ONLY);

        REQUIRE(o.openPMD() == "1.0.0");
        REQUIRE(o.openPMDextension() == 1);
        REQUIRE(o.basePath() == "/data/%T/");
        REQUIRE(o.meshesPath() == "fields/");
        REQUIRE(o.particlesPath() == "particles/");
        REQUIRE(o.author() == "Axel Huebl <a.huebl@hzdr.de>");
        REQUIRE(o.software() == "PIConGPU");
        REQUIRE(o.softwareVersion() == "0.2.0");
        REQUIRE(o.date() == "2016-11-04 00:59:14 +0100");
        REQUIRE(o.iterationEncoding() == IterationEncoding::fileBased);
        REQUIRE(o.iterationFormat() == "h5/simData_%T.h5");
        REQUIRE(o.name() == "simData_%T");

        REQUIRE(o.iterations.size() >= 1);
        REQUIRE(o.iterations.count(0) == 1);

        Iteration &i = o.iterations[0];
        REQUIRE(i.time<float>() == static_cast<float>(0.0f));
        REQUIRE(i.dt<float>() == static_cast<float>(1.0f));
        REQUIRE(i.timeUnitSI() == 1.3899999999999999e-16);

        REQUIRE(i.meshes.size() == 4);
        REQUIRE(i.meshes.count("B") == 1);
        REQUIRE(i.meshes.count("E") == 1);
        REQUIRE(i.meshes.count("e_chargeDensity") == 1);
        REQUIRE(i.meshes.count("e_energyDensity") == 1);

        std::vector<std::string> al{"z", "y", "x"};
        std::vector<float> gs{
            static_cast<float>(6.2393283843994141f),
            static_cast<float>(1.0630855560302734f),
            static_cast<float>(6.2393283843994141f)};
        std::vector<double> ggo{0., 0., 0.};
        std::array<double, 7> ud{{0., 1., -2., -1., 0., 0., 0.}};
        Mesh &B = i.meshes["B"];
        REQUIRE(B.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(B.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(B.axisLabels() == al);
        REQUIRE(B.gridSpacing<float>() == gs);
        REQUIRE(B.gridGlobalOffset() == ggo);
        REQUIRE(B.gridUnitSI() == 4.1671151661999998e-08);
        REQUIRE(B.unitDimension() == ud);
        REQUIRE(B.timeOffset<float>() == static_cast<float>(0.0f));

        REQUIRE(B.size() == 3);
        REQUIRE(B.count("x") == 1);
        REQUIRE(B.count("y") == 1);
        REQUIRE(B.count("z") == 1);

        std::vector<float> p{
            static_cast<float>(0.0f),
            static_cast<float>(0.5f),
            static_cast<float>(0.5f)};
        Extent e{80, 384, 80};
        MeshRecordComponent &B_x = B["x"];
        REQUIRE(B_x.unitSI() == 40903.822240601701);
        REQUIRE(B_x.position<float>() == p);
        REQUIRE(B_x.getDatatype() == Datatype::FLOAT);
        REQUIRE(B_x.getExtent() == e);
        REQUIRE(B_x.getDimensionality() == 3);

        p = {
            static_cast<float>(0.5f),
            static_cast<float>(0.0f),
            static_cast<float>(0.5f)};
        MeshRecordComponent &B_y = B["y"];
        REQUIRE(B_y.unitSI() == 40903.822240601701);
        REQUIRE(B_y.position<float>() == p);
        REQUIRE(B_y.getDatatype() == Datatype::FLOAT);
        REQUIRE(B_y.getExtent() == e);
        REQUIRE(B_y.getDimensionality() == 3);

        p = {
            static_cast<float>(0.5f),
            static_cast<float>(0.5f),
            static_cast<float>(0.0f)};
        MeshRecordComponent &B_z = B["z"];
        REQUIRE(B_z.unitSI() == 40903.822240601701);
        REQUIRE(B_z.position<float>() == p);
        REQUIRE(B_z.getDatatype() == Datatype::FLOAT);
        REQUIRE(B_z.getExtent() == e);
        REQUIRE(B_z.getDimensionality() == 3);

        ud = {{1., 1., -3., -1., 0., 0., 0.}};
        Mesh &E = i.meshes["E"];
        REQUIRE(E.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(E.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(E.axisLabels() == al);
        REQUIRE(E.gridSpacing<float>() == gs);
        REQUIRE(E.gridGlobalOffset() == ggo);
        REQUIRE(E.gridUnitSI() == 4.1671151661999998e-08);
        REQUIRE(E.unitDimension() == ud);
        REQUIRE(E.timeOffset<float>() == static_cast<float>(0.0f));

        REQUIRE(E.size() == 3);
        REQUIRE(E.count("x") == 1);
        REQUIRE(E.count("y") == 1);
        REQUIRE(E.count("z") == 1);

        p = {
            static_cast<float>(0.5f),
            static_cast<float>(0.0f),
            static_cast<float>(0.0f)};
        e = {80, 384, 80};
        MeshRecordComponent &E_x = E["x"];
        REQUIRE(E_x.unitSI() == 12262657411105.049);
        REQUIRE(E_x.position<float>() == p);
        REQUIRE(E_x.getDatatype() == Datatype::FLOAT);
        REQUIRE(E_x.getExtent() == e);
        REQUIRE(E_x.getDimensionality() == 3);

        p = {
            static_cast<float>(0.0f),
            static_cast<float>(0.5f),
            static_cast<float>(0.0f)};
        MeshRecordComponent &E_y = E["y"];
        REQUIRE(E_y.unitSI() == 12262657411105.049);
        REQUIRE(E_y.position<float>() == p);
        REQUIRE(E_y.getDatatype() == Datatype::FLOAT);
        REQUIRE(E_y.getExtent() == e);
        REQUIRE(E_y.getDimensionality() == 3);

        p = {
            static_cast<float>(0.0f),
            static_cast<float>(0.0f),
            static_cast<float>(0.5f)};
        MeshRecordComponent &E_z = E["z"];
        REQUIRE(E_z.unitSI() == 12262657411105.049);
        REQUIRE(E_z.position<float>() == p);
        REQUIRE(E_z.getDatatype() == Datatype::FLOAT);
        REQUIRE(E_z.getExtent() == e);
        REQUIRE(E_z.getDimensionality() == 3);

        ud = {{-3., 0., 1., 1., 0., 0., 0.}};
        Mesh &e_chargeDensity = i.meshes["e_chargeDensity"];
        REQUIRE(e_chargeDensity.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(e_chargeDensity.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(e_chargeDensity.axisLabels() == al);
        REQUIRE(e_chargeDensity.gridSpacing<float>() == gs);
        REQUIRE(e_chargeDensity.gridGlobalOffset() == ggo);
        REQUIRE(e_chargeDensity.gridUnitSI() == 4.1671151661999998e-08);
        REQUIRE(e_chargeDensity.unitDimension() == ud);
        REQUIRE(
            e_chargeDensity.timeOffset<float>() == static_cast<float>(0.0f));

        REQUIRE(e_chargeDensity.size() == 1);
        REQUIRE(e_chargeDensity.count(MeshRecordComponent::SCALAR) == 1);

        p = {
            static_cast<float>(0.f),
            static_cast<float>(0.f),
            static_cast<float>(0.f)};
        MeshRecordComponent &e_chargeDensity_scalar =
            e_chargeDensity[MeshRecordComponent::SCALAR];
        REQUIRE(e_chargeDensity_scalar.unitSI() == 66306201.002331272);
        REQUIRE(e_chargeDensity_scalar.position<float>() == p);
        REQUIRE(e_chargeDensity_scalar.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_chargeDensity_scalar.getExtent() == e);
        REQUIRE(e_chargeDensity_scalar.getDimensionality() == 3);

        ud = {{-1., 1., -2., 0., 0., 0., 0.}};
        Mesh &e_energyDensity = i.meshes["e_energyDensity"];
        REQUIRE(e_energyDensity.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(e_energyDensity.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(e_energyDensity.axisLabels() == al);
        REQUIRE(e_energyDensity.gridSpacing<float>() == gs);
        REQUIRE(e_energyDensity.gridGlobalOffset() == ggo);
        REQUIRE(e_energyDensity.gridUnitSI() == 4.1671151661999998e-08);
        REQUIRE(e_energyDensity.unitDimension() == ud);
        REQUIRE(
            e_energyDensity.timeOffset<float>() == static_cast<float>(0.0f));

        REQUIRE(e_energyDensity.size() == 1);
        REQUIRE(e_energyDensity.count(MeshRecordComponent::SCALAR) == 1);

        MeshRecordComponent &e_energyDensity_scalar =
            e_energyDensity[MeshRecordComponent::SCALAR];
        REQUIRE(e_energyDensity_scalar.unitSI() == 1.0146696675429705e+18);
        REQUIRE(e_energyDensity_scalar.position<float>() == p);
        REQUIRE(e_energyDensity_scalar.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_energyDensity_scalar.getExtent() == e);
        REQUIRE(e_energyDensity_scalar.getDimensionality() == 3);

        REQUIRE(i.particles.size() == 1);
        REQUIRE(i.particles.count("e") == 1);

        ParticleSpecies &species_e = i.particles["e"];

        REQUIRE(species_e.size() == 6);
        REQUIRE(species_e.count("charge") == 1);
        REQUIRE(species_e.count("mass") == 1);
        REQUIRE(species_e.count("momentum") == 1);
        REQUIRE(species_e.count("particlePatches") == 0);
        REQUIRE(species_e.count("position") == 1);
        REQUIRE(species_e.count("positionOffset") == 1);
        REQUIRE(species_e.count("weighting") == 1);

        ud = {{0., 0., 1., 1., 0., 0., 0.}};
        Record &e_charge = species_e["charge"];
        REQUIRE(e_charge.unitDimension() == ud);
        REQUIRE(e_charge.timeOffset<float>() == static_cast<float>(0.0f));

        REQUIRE(e_charge.size() == 1);
        REQUIRE(e_charge.count(RecordComponent::SCALAR) == 1);

        e = {2150400};
        RecordComponent &e_charge_scalar = e_charge[RecordComponent::SCALAR];
        REQUIRE(e_charge_scalar.unitSI() == 4.7980045488500004e-15);
        REQUIRE(e_charge_scalar.getDatatype() == Datatype::DOUBLE);
        REQUIRE(e_charge_scalar.getExtent() == e);
        REQUIRE(e_charge_scalar.getDimensionality() == 1);

        ud = {{0., 1., 0., 0., 0., 0., 0.}};
        Record &e_mass = species_e["mass"];
        REQUIRE(e_mass.unitDimension() == ud);
        REQUIRE(e_mass.timeOffset<float>() == static_cast<float>(0.0f));

        REQUIRE(e_mass.size() == 1);
        REQUIRE(e_mass.count(RecordComponent::SCALAR) == 1);

        RecordComponent &e_mass_scalar = e_mass[RecordComponent::SCALAR];
        REQUIRE(e_mass_scalar.unitSI() == 2.7279684799430467e-26);
        REQUIRE(e_mass_scalar.getDatatype() == Datatype::DOUBLE);
        REQUIRE(e_mass_scalar.getExtent() == e);
        REQUIRE(e_mass_scalar.getDimensionality() == 1);

        ud = {{1., 1., -1., 0., 0., 0., 0.}};
        Record &e_momentum = species_e["momentum"];
        REQUIRE(e_momentum.unitDimension() == ud);
        REQUIRE(e_momentum.timeOffset<float>() == static_cast<float>(0.0f));

        REQUIRE(e_momentum.size() == 3);
        REQUIRE(e_momentum.count("x") == 1);
        REQUIRE(e_momentum.count("y") == 1);
        REQUIRE(e_momentum.count("z") == 1);

        RecordComponent &e_momentum_x = e_momentum["x"];
        REQUIRE(e_momentum_x.unitSI() == 8.1782437594864961e-18);
        REQUIRE(e_momentum_x.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_momentum_x.getExtent() == e);
        REQUIRE(e_momentum_x.getDimensionality() == 1);

        RecordComponent &e_momentum_y = e_momentum["y"];
        REQUIRE(e_momentum_y.unitSI() == 8.1782437594864961e-18);
        REQUIRE(e_momentum_y.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_momentum_y.getExtent() == e);
        REQUIRE(e_momentum_y.getDimensionality() == 1);

        RecordComponent &e_momentum_z = e_momentum["z"];
        REQUIRE(e_momentum_z.unitSI() == 8.1782437594864961e-18);
        REQUIRE(e_momentum_z.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_momentum_z.getExtent() == e);
        REQUIRE(e_momentum_z.getDimensionality() == 1);

        ud = {{1., 0., 0., 0., 0., 0., 0.}};
        Record &e_position = species_e["position"];
        REQUIRE(e_position.unitDimension() == ud);
        REQUIRE(e_position.timeOffset<float>() == static_cast<float>(0.0f));

        REQUIRE(e_position.size() == 3);
        REQUIRE(e_position.count("x") == 1);
        REQUIRE(e_position.count("y") == 1);
        REQUIRE(e_position.count("z") == 1);

        RecordComponent &e_position_x = e_position["x"];
        REQUIRE(e_position_x.unitSI() == 2.599999993753294e-07);
        REQUIRE(e_position_x.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_position_x.getExtent() == e);
        REQUIRE(e_position_x.getDimensionality() == 1);

        RecordComponent &e_position_y = e_position["y"];
        REQUIRE(e_position_y.unitSI() == 4.4299999435019118e-08);
        REQUIRE(e_position_y.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_position_y.getExtent() == e);
        REQUIRE(e_position_y.getDimensionality() == 1);

        RecordComponent &e_position_z = e_position["z"];
        REQUIRE(e_position_z.unitSI() == 2.599999993753294e-07);
        REQUIRE(e_position_z.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_position_z.getExtent() == e);
        REQUIRE(e_position_z.getDimensionality() == 1);

        ud = {{1., 0., 0., 0., 0., 0., 0.}};
        Record &e_positionOffset = species_e["positionOffset"];
        REQUIRE(e_positionOffset.unitDimension() == ud);
        REQUIRE(
            e_positionOffset.timeOffset<float>() == static_cast<float>(0.0f));

        REQUIRE(e_positionOffset.size() == 3);
        REQUIRE(e_positionOffset.count("x") == 1);
        REQUIRE(e_positionOffset.count("y") == 1);
        REQUIRE(e_positionOffset.count("z") == 1);

        RecordComponent &e_positionOffset_x = e_positionOffset["x"];
        REQUIRE(e_positionOffset_x.unitSI() == 2.599999993753294e-07);
        REQUIRE(
            e_positionOffset_x.getDatatype() == determineDatatype<int32_t>());
        REQUIRE(e_positionOffset_x.getExtent() == e);
        REQUIRE(e_positionOffset_x.getDimensionality() == 1);

        RecordComponent &e_positionOffset_y = e_positionOffset["y"];
        REQUIRE(e_positionOffset_y.unitSI() == 4.4299999435019118e-08);
        REQUIRE(
            e_positionOffset_y.getDatatype() == determineDatatype<int32_t>());
        REQUIRE(e_positionOffset_y.getExtent() == e);
        REQUIRE(e_positionOffset_y.getDimensionality() == 1);

        RecordComponent &e_positionOffset_z = e_positionOffset["z"];
        REQUIRE(e_positionOffset_z.unitSI() == 2.599999993753294e-07);
        REQUIRE(
            e_positionOffset_z.getDatatype() == determineDatatype<int32_t>());
        REQUIRE(e_positionOffset_z.getExtent() == e);
        REQUIRE(e_positionOffset_z.getDimensionality() == 1);

        ud = {{0., 0., 0., 0., 0., 0., 0.}};
        Record &e_weighting = species_e["weighting"];
        REQUIRE(e_weighting.unitDimension() == ud);
        REQUIRE(e_weighting.timeOffset<float>() == static_cast<float>(0.0f));

        REQUIRE(e_weighting.size() == 1);
        REQUIRE(e_weighting.count(RecordComponent::SCALAR) == 1);

        RecordComponent &e_weighting_scalar =
            e_weighting[RecordComponent::SCALAR];
        REQUIRE(e_weighting_scalar.unitSI() == 1.0);
        REQUIRE(e_weighting_scalar.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_weighting_scalar.getExtent() == e);
        REQUIRE(e_weighting_scalar.getDimensionality() == 1);

        ParticlePatches &e_patches = species_e.particlePatches;
        REQUIRE(
            e_patches.size() ==
            4); /* extent, numParticles, numParticlesOffset, offset */
        REQUIRE(e_patches.count("extent") == 1);
        REQUIRE(e_patches.count("numParticles") == 1);
        REQUIRE(e_patches.count("numParticlesOffset") == 1);
        REQUIRE(e_patches.count("offset") == 1);
        REQUIRE(e_patches.numPatches() == 4);

        ud = {{1., 0., 0., 0., 0., 0., 0.}};
        PatchRecord &e_extent = e_patches["extent"];
        REQUIRE(e_extent.unitDimension() == ud);

        REQUIRE(e_extent.size() == 3);
        REQUIRE(e_extent.count("x") == 1);
        REQUIRE(e_extent.count("y") == 1);
        REQUIRE(e_extent.count("z") == 1);

        PatchRecordComponent &e_extent_x = e_extent["x"];
        REQUIRE(e_extent_x.unitSI() == 2.599999993753294e-07);
#if !defined(_MSC_VER)
        REQUIRE(e_extent_x.getDatatype() == determineDatatype<uint64_t>());
#endif
        REQUIRE(
            isSame(e_extent_x.getDatatype(), determineDatatype<uint64_t>()));

        PatchRecordComponent &e_extent_y = e_extent["y"];
        REQUIRE(e_extent_y.unitSI() == 4.429999943501912e-08);
#if !defined(_MSC_VER)
        REQUIRE(e_extent_y.getDatatype() == determineDatatype<uint64_t>());
#endif
        REQUIRE(
            isSame(e_extent_y.getDatatype(), determineDatatype<uint64_t>()));

        PatchRecordComponent &e_extent_z = e_extent["z"];
        REQUIRE(e_extent_z.unitSI() == 2.599999993753294e-07);
#if !defined(_MSC_VER)
        REQUIRE(e_extent_z.getDatatype() == determineDatatype<uint64_t>());
#endif
        REQUIRE(
            isSame(e_extent_z.getDatatype(), determineDatatype<uint64_t>()));

        std::vector<uint64_t> data(e_patches.size());
        e_extent_z.loadRaw(data.data());
        species_e.seriesFlush();
        REQUIRE(data.at(0) == static_cast<uint64_t>(80));
        REQUIRE(data.at(1) == static_cast<uint64_t>(80));
        REQUIRE(data.at(2) == static_cast<uint64_t>(80));
        REQUIRE(data.at(3) == static_cast<uint64_t>(80));

        PatchRecord &e_numParticles = e_patches["numParticles"];
        REQUIRE(e_numParticles.size() == 1);
        REQUIRE(e_numParticles.count(RecordComponent::SCALAR) == 1);

        PatchRecordComponent &e_numParticles_scalar =
            e_numParticles[RecordComponent::SCALAR];
#if !defined(_MSC_VER)
        REQUIRE(
            e_numParticles_scalar.getDatatype() ==
            determineDatatype<uint64_t>());
#endif
        REQUIRE(isSame(
            e_numParticles_scalar.getDatatype(),
            determineDatatype<uint64_t>()));

        e_numParticles_scalar.loadRaw(data.data());
        o.flush();
        REQUIRE(data.at(0) == static_cast<uint64_t>(512000));
        REQUIRE(data.at(1) == static_cast<uint64_t>(819200));
        REQUIRE(data.at(2) == static_cast<uint64_t>(819200));
        REQUIRE(data.at(3) == static_cast<uint64_t>(0));

        PatchRecord &e_numParticlesOffset = e_patches["numParticlesOffset"];
        REQUIRE(e_numParticlesOffset.size() == 1);
        REQUIRE(e_numParticlesOffset.count(RecordComponent::SCALAR) == 1);

        PatchRecordComponent &e_numParticlesOffset_scalar =
            e_numParticlesOffset[RecordComponent::SCALAR];
#if !defined(_MSC_VER)
        REQUIRE(
            e_numParticlesOffset_scalar.getDatatype() ==
            determineDatatype<uint64_t>());
#endif
        REQUIRE(isSame(
            e_numParticlesOffset_scalar.getDatatype(),
            determineDatatype<uint64_t>()));

        PatchRecord &e_offset = e_patches["offset"];
        REQUIRE(e_offset.unitDimension() == ud);

        REQUIRE(e_offset.size() == 3);
        REQUIRE(e_offset.count("x") == 1);
        REQUIRE(e_offset.count("y") == 1);
        REQUIRE(e_offset.count("z") == 1);

        PatchRecordComponent &e_offset_x = e_offset["x"];
        REQUIRE(e_offset_x.unitSI() == 2.599999993753294e-07);
#if !defined(_MSC_VER)
        REQUIRE(e_offset_x.getDatatype() == determineDatatype<uint64_t>());
#endif
        REQUIRE(
            isSame(e_offset_x.getDatatype(), determineDatatype<uint64_t>()));

        PatchRecordComponent &e_offset_y = e_offset["y"];
        REQUIRE(e_offset_y.unitSI() == 4.429999943501912e-08);
#if !defined(_MSC_VER)
        REQUIRE(e_offset_y.getDatatype() == determineDatatype<uint64_t>());
#endif
        REQUIRE(
            isSame(e_offset_y.getDatatype(), determineDatatype<uint64_t>()));

        e_offset_y.loadRaw(data.data());
        o.flush();
        REQUIRE(data.at(0) == static_cast<uint64_t>(0));
        REQUIRE(data.at(1) == static_cast<uint64_t>(128));
        REQUIRE(data.at(2) == static_cast<uint64_t>(256));
        REQUIRE(data.at(3) == static_cast<uint64_t>(384));

        PatchRecordComponent &e_offset_z = e_offset["z"];
        REQUIRE(e_offset_z.unitSI() == 2.599999993753294e-07);
#if !defined(_MSC_VER)
        REQUIRE(e_offset_z.getDatatype() == determineDatatype<uint64_t>());
#endif
        REQUIRE(
            isSame(e_offset_z.getDatatype(), determineDatatype<uint64_t>()));
    }
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "HZDR sample not accessible. (" << e.what() << ")\n";
            return;
        }
    }
}

TEST_CASE("hdf5_bool_test", "[serial][hdf5]")
{
    bool_test("h5");
}

TEST_CASE("hdf5_patch_test", "[serial][hdf5]")
{
    patch_test("h5");
}

TEST_CASE("hdf5_deletion_test", "[serial][hdf5]")
{
    deletion_test("h5");
}
#else
TEST_CASE("no_serial_hdf5", "[serial][hdf5]")
{
    REQUIRE(true);
}
#endif
#if openPMD_HAVE_ADIOS2

TEST_CASE("hzdr_bp3_sample_content_test", "[serial][adios2][bp3]")
{
    // since this file might not be publicly available, gracefully handle errors
    /** @todo add bp3 example files to
     * https://github.com/openPMD/openPMD-example-datasets */
    try
    {
        /* HZDR: /bigdata/hplsim/development/huebl/lwfa-bgfield-001
         * DOI:10.14278/rodare.57 */
        Series o = Series(
            "../samples/hzdr-sample/bp/checkpoint_%T.bp", Access::READ_ONLY);

        REQUIRE(o.openPMD() == "1.0.0");
        REQUIRE(o.openPMDextension() == 1);
        REQUIRE(o.basePath() == "/data/%T/");
        REQUIRE(o.meshesPath() == "fields/");
        REQUIRE(o.particlesPath() == "particles/");
        REQUIRE(o.author() == "Axel Huebl <a.huebl@hzdr.de>");
        REQUIRE(o.software() == "PIConGPU");
        REQUIRE(o.softwareVersion() == "0.4.0-dev");
        REQUIRE(o.date() == "2017-07-14 19:29:13 +0200");
        REQUIRE(o.iterationEncoding() == IterationEncoding::fileBased);
        REQUIRE(o.iterationFormat() == "checkpoint_%T.bp");

        REQUIRE(o.iterations.size() >= 1);
        REQUIRE(o.iterations.count(0) == 1);

        Iteration &i = o.iterations[0];
        REQUIRE(i.time<float>() == static_cast<float>(0.0f));
        REQUIRE(i.dt<float>() == static_cast<float>(1.0f));
        REQUIRE(i.timeUnitSI() == 1.3899999999999999e-16);

        REQUIRE(i.meshes.count("B") == 1);
        REQUIRE(i.meshes.count("E") == 1);
        REQUIRE(i.meshes.size() == 2);

        std::vector<std::string> al{"z", "y", "x"};
        std::vector<float> gs{
            static_cast<float>(4.252342224121094f),
            static_cast<float>(1.0630855560302734f),
            static_cast<float>(4.252342224121094f)};
        std::vector<double> ggo{0., 0., 0.};
        std::array<double, 7> ud{{0., 1., -2., -1., 0., 0., 0.}};
        Mesh &B = i.meshes["B"];
        REQUIRE(B.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(B.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(B.axisLabels() == al);
        REQUIRE(B.gridSpacing<float>() == gs);
        REQUIRE(B.gridGlobalOffset() == ggo);
        REQUIRE(B.gridUnitSI() == 4.1671151661999998e-08);
        REQUIRE(B.unitDimension() == ud);
        REQUIRE(B.timeOffset<float>() == static_cast<float>(0.0f));

        REQUIRE(B.size() == 3);
        REQUIRE(B.count("x") == 1);
        REQUIRE(B.count("y") == 1);
        REQUIRE(B.count("z") == 1);

        std::vector<float> p{
            static_cast<float>(0.0f),
            static_cast<float>(0.5f),
            static_cast<float>(0.5f)};
        Extent e{192, 512, 192};
        MeshRecordComponent &B_x = B["x"];
        REQUIRE(B_x.unitSI() == 40903.82224060171);
        REQUIRE(B_x.position<float>() == p);
        REQUIRE(B_x.getDatatype() == Datatype::FLOAT);
        REQUIRE(B_x.getExtent() == e);
        REQUIRE(B_x.getDimensionality() == 3);

        p = {
            static_cast<float>(0.5f),
            static_cast<float>(0.0f),
            static_cast<float>(0.5f)};
        MeshRecordComponent &B_y = B["y"];
        REQUIRE(B_y.unitSI() == 40903.82224060171);
        REQUIRE(B_y.position<float>() == p);
        REQUIRE(B_y.getDatatype() == Datatype::FLOAT);
        REQUIRE(B_y.getExtent() == e);
        REQUIRE(B_y.getDimensionality() == 3);

        p = {
            static_cast<float>(0.5f),
            static_cast<float>(0.5f),
            static_cast<float>(0.0f)};
        MeshRecordComponent &B_z = B["z"];
        REQUIRE(B_z.unitSI() == 40903.82224060171);
        REQUIRE(B_z.position<float>() == p);
        REQUIRE(B_z.getDatatype() == Datatype::FLOAT);
        REQUIRE(B_z.getExtent() == e);
        REQUIRE(B_z.getDimensionality() == 3);

        ud = {{1., 1., -3., -1., 0., 0., 0.}};
        Mesh &E = i.meshes["E"];
        REQUIRE(E.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(E.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(E.axisLabels() == al);
        REQUIRE(E.gridSpacing<float>() == gs);
        REQUIRE(E.gridGlobalOffset() == ggo);
        REQUIRE(E.gridUnitSI() == 4.1671151661999998e-08);
        REQUIRE(E.unitDimension() == ud);
        REQUIRE(E.timeOffset<float>() == static_cast<float>(0.0f));

        REQUIRE(E.size() == 3);
        REQUIRE(E.count("x") == 1);
        REQUIRE(E.count("y") == 1);
        REQUIRE(E.count("z") == 1);

        p = {
            static_cast<float>(0.5f),
            static_cast<float>(0.0f),
            static_cast<float>(0.0f)};
        e = {192, 512, 192};
        MeshRecordComponent &E_x = E["x"];
        REQUIRE(E_x.unitSI() == 12262657411105.05);
        REQUIRE(E_x.position<float>() == p);
        REQUIRE(E_x.getDatatype() == Datatype::FLOAT);
        REQUIRE(E_x.getExtent() == e);
        REQUIRE(E_x.getDimensionality() == 3);

        p = {
            static_cast<float>(0.0f),
            static_cast<float>(0.5f),
            static_cast<float>(0.0f)};
        MeshRecordComponent &E_y = E["y"];
        REQUIRE(E_y.unitSI() == 12262657411105.05);
        REQUIRE(E_y.position<float>() == p);
        REQUIRE(E_y.getDatatype() == Datatype::FLOAT);
        REQUIRE(E_y.getExtent() == e);
        REQUIRE(E_y.getDimensionality() == 3);

        p = {
            static_cast<float>(0.0f),
            static_cast<float>(0.0f),
            static_cast<float>(0.5f)};
        MeshRecordComponent &E_z = E["z"];
        REQUIRE(E_z.unitSI() == 12262657411105.05);
        REQUIRE(E_z.position<float>() == p);
        REQUIRE(E_z.getDatatype() == Datatype::FLOAT);
        REQUIRE(E_z.getExtent() == e);
        REQUIRE(E_z.getDimensionality() == 3);

        REQUIRE(i.particles.empty());

        float actual[3][3][3] = {
            {{6.7173387e-06f, 6.7173387e-06f, 6.7173387e-06f},
             {7.0438218e-06f, 7.0438218e-06f, 7.0438218e-06f},
             {7.3689453e-06f, 7.3689453e-06f, 7.3689453e-06f}},
            {{6.7173387e-06f, 6.7173387e-06f, 6.7173387e-06f},
             {7.0438218e-06f, 7.0438218e-06f, 7.0438218e-06f},
             {7.3689453e-06f, 7.3689453e-06f, 7.3689453e-06f}},
            {{6.7173387e-06f, 6.7173387e-06f, 6.7173387e-06f},
             {7.0438218e-06f, 7.0438218e-06f, 7.0438218e-06f},
             {7.3689453e-06f, 7.3689453e-06f, 7.3689453e-06f}}};
        Offset offset{20, 20, 150};
        Extent extent{3, 3, 3};
        auto data = B_z.loadChunk<float>(offset, extent);
        o.flush();
        float *raw_ptr = data.get();

        for (int a = 0; a < 3; ++a)
            for (int b = 0; b < 3; ++b)
                for (int c = 0; c < 3; ++c)
                    REQUIRE(raw_ptr[((a * 3) + b) * 3 + c] == actual[a][b][c]);
    }
    catch (error::ReadError &e)
    {
        if (e.reason == error::Reason::Inaccessible)
        {
            std::cerr << "HZDR sample not accessible. (" << e.what() << ")\n";
            return;
        }
    }
}
#endif

#if openPMD_HAVE_ADIOS2
TEST_CASE("git_adios2_early_chunk_query", "[serial][adios2]")
{
    git_early_chunk_query(
        "../samples/git-sample/3d-bp4/example-3d-bp4_%T.bp",
        "e",
        600,
        R"({"backend": "adios2"})");
}

/*
 * Require __unix__ since we need all that filestat stuff for this test.
 */
#if defined(__unix__) && openPMD_HAVE_ADIOS2_BP5

enum class FlushDuringStep
{
    Never,
    Default_No,
    Default_Yes,
    Always
};

void adios2_bp5_flush(std::string const &cfg, FlushDuringStep flushDuringStep)
{
    constexpr size_t size = 1024 * 1024;

    auto getsize = []() {
        off_t res = 0;
        int dirfd = open("../samples/bp5_flush.bp", O_RDONLY);
        if (dirfd < 0)
        {
            throw std::system_error(
                std::error_code(errno, std::system_category()));
        }
        DIR *directory = fdopendir(dirfd);
        if (!directory)
        {
            close(dirfd);
            throw std::system_error(
                std::error_code(errno, std::system_category()));
        }
        dirent *entry;
        struct stat statbuf;
        while ((entry = readdir(directory)) != nullptr)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }
            int err = fstatat(dirfd, entry->d_name, &statbuf, 0);
            if (err != 0)
            {
                closedir(directory);
                close(dirfd);
                throw std::system_error(
                    std::error_code(errno, std::system_category()));
            }
            res += statbuf.st_size;
        }
        closedir(directory);
        close(dirfd);
        return res;
    };
    std::vector<int32_t> data(size, 10);
    Datatype dtype = determineDatatype<int32_t>();
    {
        Series write("../samples/bp5_flush.bp", Access::CREATE, cfg);

        {
            auto component =
                write.writeIterations()[0]
                    .meshes["e_chargeDensity"][RecordComponent::SCALAR];
            component.resetDataset({dtype, {size}});
            component.storeChunk(data, {0}, {size});
            // component.seriesFlush(FlushMode::NonCollective);
            component.seriesFlush();
        }

        auto currentSize = getsize();
        if (flushDuringStep == FlushDuringStep::Default_Yes ||
            flushDuringStep == FlushDuringStep::Always)
        {
            // should be roughly within 1% of 4Mb
            REQUIRE(std::abs(1 - double(currentSize) / (4 * size)) <= 0.01);
        }
        else
        {
            // should be roughly zero
            REQUIRE(currentSize <= 4096);
        }

        {
            auto component =
                write.writeIterations()[0]
                    .meshes["i_chargeDensity"][RecordComponent::SCALAR];
            component.resetDataset({dtype, {size}});
            component.storeChunk(data, {0}, {size});
        }

        currentSize = getsize();
        if (flushDuringStep == FlushDuringStep::Default_Yes ||
            flushDuringStep == FlushDuringStep::Always)
        {
            // should still be roughly within 1% of 4Mb
            REQUIRE(std::abs(1 - double(currentSize) / (4 * size)) <= 0.01);
        }
        else
        {
            // should be roughly zero
            REQUIRE(currentSize <= 4096);
        }

        write.flush();
        currentSize = getsize();
        if (flushDuringStep == FlushDuringStep::Default_Yes ||
            flushDuringStep == FlushDuringStep::Always)
        {
            // should now be roughly within 1% of 8Mb
            REQUIRE(std::abs(1 - double(currentSize) / (8 * size)) <= 0.01);
        }
        else
        {
            // should be roughly zero
            REQUIRE(currentSize <= 4096);
        }

        {
            auto component =
                write.writeIterations()[0]
                    .meshes["temperature"][RecordComponent::SCALAR];
            component.resetDataset({dtype, {size}});
            component.storeChunk(data, {0}, {size});
            // component.seriesFlush(FlushMode::NonCollective);
            component.seriesFlush(
                "adios2.engine.preferred_flush_target = \"buffer\"");
        }
        currentSize = getsize();
        if (flushDuringStep == FlushDuringStep::Always)
        {
            // should now be roughly within 1% of 12Mb
            REQUIRE(std::abs(1 - double(currentSize) / (12 * size)) <= 0.01);
        }
        else if (flushDuringStep == FlushDuringStep::Default_Yes)
        {
            // should now be roughly within 1% of 8Mb
            REQUIRE(std::abs(1 - double(currentSize) / (8 * size)) <= 0.01);
        }
        else
        {
            // should be roughly zero
            REQUIRE(currentSize <= 4096);
        }

        auto has_been_deleted = std::make_shared<bool>(false);
        UniquePtrWithLambda<int32_t> copied_as_unique(
            new int[size], [has_been_deleted](int const *ptr) {
                delete[] ptr;
                *has_been_deleted = true;
            });
        std::copy_n(data.data(), size, copied_as_unique.get());
        {
            auto component =
                write.writeIterations()[0]
                    .meshes["temperature"][RecordComponent::SCALAR];
            component.resetDataset({dtype, {size}});
            component.storeChunk(std::move(copied_as_unique), {0}, {size});
            // component.seriesFlush(FlushMode::NonCollective);
            component.seriesFlush(
                "adios2.engine.preferred_flush_target = \"disk\"");
        }
        currentSize = getsize();
        if (flushDuringStep != FlushDuringStep::Never)
        {
            // should now be roughly within 1% of 16Mb
            REQUIRE(std::abs(1 - double(currentSize) / (16 * size)) <= 0.01);
            REQUIRE(*has_been_deleted);
        }
        else
        {
            // should be roughly zero
            REQUIRE(currentSize <= 4096);
            REQUIRE(!*has_been_deleted);
        }
    }
    auto currentSize = getsize();
    // should now be indiscriminately be roughly within 1% of 8Mb after
    // closing the Series
    REQUIRE(std::abs(1 - double(currentSize) / (16 * size)) <= 0.01);
}

TEST_CASE("adios2_bp5_flush", "[serial][adios2]")
{
    std::string cfg1 = R"(
[adios2]

[adios2.engine]
type = "bp5"
preferred_flush_target = "disk"

[adios2.engine.parameters]
AggregationType = "TwoLevelShm"
MaxShmSize = 3221225472
NumSubFiles = 1
NumAggregators = 1
BufferChunkSize = 2147483646 # 2^31 - 2
)";

    adios2_bp5_flush(
        cfg1, /* flushDuringStep = */ FlushDuringStep::Default_Yes);

    std::string cfg2 = R"(
[adios2]

[adios2.engine]
type = "bp5"
preferred_flush_target = "buffer"

[adios2.engine.parameters]
AggregationType = "TwoLevelShm"
MaxShmSize = 3221225472
NumSubFiles = 1
NumAggregators = 1
BufferChunkSize = 2147483646 # 2^31 - 2
)";

    adios2_bp5_flush(cfg2, /* flushDuringStep = */ FlushDuringStep::Default_No);

    std::string cfg3 = R"(
[adios2]

[adios2.engine]
type = "bp5"
# preferred_flush_target = <default>

[adios2.engine.parameters]
AggregationType = "TwoLevelShm"
MaxShmSize = 3221225472
NumSubFiles = 1
NumAggregators = 1
BufferChunkSize = 2147483646 # 2^31 - 2
)";

    adios2_bp5_flush(
        cfg3,
        /* flushDuringStep = */
        auxiliary::getEnvNum("OPENPMD_ADIOS2_ASYNC_WRITE", 0) == 0
            ? FlushDuringStep::Default_Yes
            : FlushDuringStep::Default_No);

    std::string cfg4 = R"(
[adios2]

[adios2.engine]
type = "bp5"
preferred_flush_target = "buffer_override"

[adios2.engine.parameters]
AggregationType = "TwoLevelShm"
MaxShmSize = 3221225472
NumSubFiles = 1
NumAggregators = 1
BufferChunkSize = 2147483646 # 2^31 - 2
)";

    adios2_bp5_flush(cfg4, /* flushDuringStep = */ FlushDuringStep::Never);

    std::string cfg5 = R"(
[adios2]

[adios2.engine]
type = "bp5"
preferred_flush_target = "disk_override"

[adios2.engine.parameters]
AggregationType = "TwoLevelShm"
MaxShmSize = 3221225472
NumSubFiles = 1
NumAggregators = 1
BufferChunkSize = 2147483646 # 2^31 - 2
)";

    adios2_bp5_flush(cfg5, /* flushDuringStep = */ FlushDuringStep::Always);

#if openPMD_HAVE_ADIOS2_BP5
    std::string cfg6 = R"(
[adios2]

[adios2.engine]
preferred_flush_target = "disk"

[adios2.engine.parameters]
AggregationType = "TwoLevelShm"
MaxShmSize = 3221225472
NumSubFiles = 1
NumAggregators = 1
BufferChunkSize = 2147483646 # 2^31 - 2
)";

    adios2_bp5_flush(
        cfg6, /* flushDuringStep = */ FlushDuringStep::Default_Yes);
#endif
}
#endif

#if openPMD_HAVE_ADIOS2_BP5
TEST_CASE("adios2_flush_via_step")
{
    Series write(
        "../samples/adios2_flush_via_step/simData_%T.bp5",
        Access::CREATE,
        R"(adios2.engine.parameters.FlattenSteps = "on")");
    std::vector<float> data(10);
    for (Iteration::IterationIndex_t i = 0; i < 5; ++i)
    {
        Iteration it = write.writeIterations()[i];
        auto E_x = it.meshes["E"]["x"];
        E_x.resetDataset({Datatype::FLOAT, {10, 10}});
        for (Extent::value_type j = 0; j < 10; ++j)
        {
            std::iota(data.begin(), data.end(), i * 100 + j * 10);
            E_x.storeChunk(data, {j, 0}, {1, 10});
            write.flush(R"(adios2.engine.preferred_flush_target = "new_step")");
        }
        it.close();
    }

#if openPMD_HAS_ADIOS_2_10_1
    for (auto access : {Access::READ_RANDOM_ACCESS, Access::READ_LINEAR})
    {
        Series read("../samples/adios2_flush_via_step/simData_%T.%E", access);
        std::vector<float> load_data(100);
        data.resize(100);
        for (auto iteration : read.readIterations())
        {
            std::iota(data.begin(), data.end(), iteration.iterationIndex * 100);
            iteration.meshes["E"]["x"].loadChunkRaw(
                load_data.data(), {0, 0}, {10, 10});
            iteration.close();
            REQUIRE(load_data == data);
        }
    }
#endif

    /*
     * Now emulate restarting from a checkpoint after a crash and continuing to
     * write to the output Series. The semantics of openPMD::Access::APPEND
     * don't fully fit here since that mode is for adding new Iterations to an
     * existing Series. What we truly want to do is to continue writing to an
     * Iteration without replacing it with a new one. So we must use the option
     * adios2.engine.access_mode = "append" to tell the ADIOS2 backend that new
     * steps should be added to an existing Iteration file.
     */

    write = Series(
        "../samples/adios2_flush_via_step/simData_%T.bp5",
        Access::APPEND,
        R"(
            [adios2.engine]
            access_mode = "append"
            parameters.FlattenSteps = "on"
        )");
    for (Iteration::IterationIndex_t i = 0; i < 5; ++i)
    {
        Iteration it = write.writeIterations()[i];
        auto E_x = it.meshes["E"]["y"];
        E_x.resetDataset({Datatype::FLOAT, {10, 10}});
        for (Extent::value_type j = 0; j < 10; ++j)
        {
            std::iota(data.begin(), data.end(), i * 100 + j * 10);
            E_x.storeChunk(data, {j, 0}, {1, 10});
            write.flush(R"(adios2.engine.preferred_flush_target = "new_step")");
        }
        it.close();
    }

#if openPMD_HAS_ADIOS_2_10_1
    for (auto access : {Access::READ_RANDOM_ACCESS, Access::READ_LINEAR})
    {
        Series read("../samples/adios2_flush_via_step/simData_%T.%E", access);
        std::vector<float> load_data(100);
        data.resize(100);
        for (auto iteration : read.readIterations())
        {
            std::iota(data.begin(), data.end(), iteration.iterationIndex * 100);
            iteration.meshes["E"]["x"].loadChunkRaw(
                load_data.data(), {0, 0}, {10, 10});
            iteration.meshes["E"]["y"].loadChunkRaw(
                load_data.data(), {0, 0}, {10, 10});
            iteration.close();
            REQUIRE(load_data == data);
            REQUIRE(load_data == data);
        }
    }
#endif
}
#endif

TEST_CASE("adios2_engines_and_file_endings")
{
    size_t filenameCounter = 0;
    auto groupbased_test_explicit_backend =
        [&filenameCounter](
            std::string const &ext,
            bool directory,
            std::string const &requiredEngine,
            std::string const &filesystemExt,
            std::string const &jsonCfg = "{}") mutable {
            auto basename = "../samples/file_endings/groupbased" +
                std::to_string(filenameCounter++);
            auto name = basename + ext;
            std::cout << "Writing to file '" << name << "', should be engine "
                      << requiredEngine << " (explicit backend)." << std::endl;
            auto filesystemname =
                filesystemExt.empty() ? name : basename + filesystemExt;
            {
                Series write(
                    name,
                    Access::CREATE,
                    json::merge("backend = \"adios2\"", jsonCfg));
            }
            if (directory)
            {
                REQUIRE(auxiliary::directory_exists(filesystemname));
            }
            else
            {
                REQUIRE(auxiliary::file_exists(filesystemname));
            }
            {
                Series read(
                    name,
                    Access::READ_ONLY,
                    "backend = \"adios2\"\nadios2.engine.type = \"" +
                        requiredEngine + "\"");
            }
        };

    groupbased_test_explicit_backend(".bp", true, "file", "");
    groupbased_test_explicit_backend(".bp4", true, "bp4", "");
    groupbased_test_explicit_backend(
        ".bp", true, "bp4", "", "adios2.engine.type = \"bp4\"");
    groupbased_test_explicit_backend(
        ".bp4", true, "bp4", "", "adios2.engine.type = \"bp4\"");
    groupbased_test_explicit_backend(
        ".bp5", true, "bp4", "", "adios2.engine.type = \"bp4\"");
    groupbased_test_explicit_backend(
        ".bp", false, "bp3", "", "adios2.engine.type = \"bp3\"");
    groupbased_test_explicit_backend(
        ".sst", false, "bp3", ".sst.bp", "adios2.engine.type = \"bp3\"");
    groupbased_test_explicit_backend(
        "", false, "bp3", ".bp", "adios2.engine.type = \"bp3\"");
    groupbased_test_explicit_backend(
        "", true, "bp4", "", "adios2.engine.type = \"bp4\"");

#if openPMD_HAVE_ADIOS2_BP5
    // BP5 tests
    groupbased_test_explicit_backend(".bp5", true, "bp5", "");
    groupbased_test_explicit_backend(
        ".bp", true, "bp5", "", "adios2.engine.type = \"bp5\"");
    groupbased_test_explicit_backend(
        ".bp4", true, "bp5", "", "adios2.engine.type = \"bp5\"");
    groupbased_test_explicit_backend(
        ".bp5", true, "bp5", "", "adios2.engine.type = \"bp5\"");
    groupbased_test_explicit_backend(
        "", true, "bp5", "", "adios2.engine.type = \"bp5\"");
#endif

    auto groupbased_test_no_explicit_backend =
        [&filenameCounter](
            std::string const &ext,
            bool directory,
            std::string const &requiredEngine,
            std::string const &filesystemExt,
            std::string const &jsonCfg = "{}") mutable {
            auto basename = "../samples/file_endings/groupbased" +
                std::to_string(filenameCounter++);
            auto name = basename + ext;
            std::cout << "Writing to file '" << name << "', should be engine "
                      << requiredEngine << " (no explicit backend)."
                      << std::endl;
            auto filesystemname =
                filesystemExt.empty() ? name : basename + filesystemExt;
            {
                Series write(name, Access::CREATE, jsonCfg);
                write.close();
            }
            if (directory)
            {
                REQUIRE(auxiliary::directory_exists(filesystemname));
            }
            else
            {
                REQUIRE(auxiliary::file_exists(filesystemname));
            }
            {
                Series read(
                    name,
                    Access::READ_ONLY,
                    "backend = \"adios2\"\nadios2.engine.type = \"" +
                        requiredEngine + "\"");
            }
        };

    groupbased_test_no_explicit_backend(".bp", true, "file", "");
    groupbased_test_no_explicit_backend(".bp4", true, "bp4", "");
    groupbased_test_no_explicit_backend(
        ".bp", true, "bp4", "", "adios2.engine.type = \"bp4\"");
    groupbased_test_no_explicit_backend(
        ".bp4", true, "bp4", "", "adios2.engine.type = \"bp4\"");
    groupbased_test_no_explicit_backend(
        ".bp5", true, "bp4", "", "adios2.engine.type = \"bp4\"");
    groupbased_test_no_explicit_backend(
        ".bp", false, "bp3", "", "adios2.engine.type = \"bp3\"");
    groupbased_test_no_explicit_backend(
        ".sst", false, "bp3", ".sst.bp", "adios2.engine.type = \"bp3\"");
    REQUIRE_THROWS(groupbased_test_no_explicit_backend(
        "", false, "bp3", ".bp", "adios2.engine.type = \"bp3\""));
    REQUIRE_THROWS(groupbased_test_no_explicit_backend(
        "", true, "bp4", "", "adios2.engine.type = \"bp4\""));

#if openPMD_HAVE_ADIOS2_BP5
    // BP5 tests
    groupbased_test_no_explicit_backend(".bp5", true, "bp5", "");
    groupbased_test_no_explicit_backend(
        ".bp", true, "bp5", "", "adios2.engine.type = \"bp5\"");
    groupbased_test_no_explicit_backend(
        ".bp4", true, "bp5", "", "adios2.engine.type = \"bp5\"");
    groupbased_test_no_explicit_backend(
        ".bp5", true, "bp5", "", "adios2.engine.type = \"bp5\"");
    REQUIRE_THROWS(groupbased_test_no_explicit_backend(
        "", true, "bp5", "", "adios2.engine.type = \"bp5\""));
#endif

    filenameCounter = 0;
    auto filebased_test_explicit_backend =
        [&filenameCounter](
            std::string const &ext,
            bool directory,
            std::string const &requiredEngine,
            std::string const &filesystemExt,
            std::string const &jsonCfg = "{}") mutable {
            auto basename = "../samples/file_endings/filebased" +
                std::to_string(filenameCounter++);
            auto name = basename + "_%T" + ext;
            std::cout << "Writing to file '" << name << "', should be engine "
                      << requiredEngine << " (explicit backend)." << std::endl;
            auto filesystemname =
                basename + "_0" + (filesystemExt.empty() ? ext : filesystemExt);
            {
                Series write(
                    name,
                    Access::CREATE,
                    json::merge("backend = \"adios2\"", jsonCfg));
                write.writeIterations()[0];
            }
            if (directory)
            {
                REQUIRE(auxiliary::directory_exists(filesystemname));
            }
            else
            {
                REQUIRE(auxiliary::file_exists(filesystemname));
            }
            {
                if (requiredEngine == "bp3" &&
                    !auxiliary::ends_with(name, ".bp"))
                {
                    /*
                     * File-based parsing procedures are not aware that BP3
                     * engine adds its ending and won't find the iterations if
                     * we don't give it a little nudge.
                     */
                    name += ".bp";
                }
                Series read(
                    name,
                    Access::READ_ONLY,
                    "backend = \"adios2\"\nadios2.engine.type = \"" +
                        requiredEngine + "\"");
            }
        };

    filebased_test_explicit_backend(".bp", true, "file", "");
    filebased_test_explicit_backend(".bp4", true, "bp4", "");
    filebased_test_explicit_backend(
        ".bp", true, "bp4", "", "adios2.engine.type = \"bp4\"");
    filebased_test_explicit_backend(
        ".bp4", true, "bp4", "", "adios2.engine.type = \"bp4\"");
    filebased_test_explicit_backend(
        ".bp5", true, "bp4", "", "adios2.engine.type = \"bp4\"");
    filebased_test_explicit_backend(
        ".bp", false, "bp3", "", "adios2.engine.type = \"bp3\"");
    filebased_test_explicit_backend(
        ".sst", false, "bp3", ".sst.bp", "adios2.engine.type = \"bp3\"");
    filebased_test_explicit_backend(
        "", false, "bp3", ".bp", "adios2.engine.type = \"bp3\"");
    filebased_test_explicit_backend(
        "", true, "bp4", "", "adios2.engine.type = \"bp4\"");

#if openPMD_HAVE_ADIOS2_BP5
    // BP5 tests
    filebased_test_explicit_backend(".bp5", true, "bp5", "");
    filebased_test_explicit_backend(
        ".bp", true, "bp5", "", "adios2.engine.type = \"bp5\"");
    filebased_test_explicit_backend(
        ".bp4", true, "bp5", "", "adios2.engine.type = \"bp5\"");
    filebased_test_explicit_backend(
        ".bp5", true, "bp5", "", "adios2.engine.type = \"bp5\"");
    filebased_test_explicit_backend(
        "", true, "bp5", "", "adios2.engine.type = \"bp5\"");
#endif

    auto filebased_test_no_explicit_backend =
        [&filenameCounter](
            std::string const &ext,
            bool directory,
            std::string const &requiredEngine,
            std::string const &filesystemExt,
            std::string const &jsonCfg = "{}") mutable {
            auto basename = "../samples/file_endings/filebased" +
                std::to_string(filenameCounter++);
            auto name = basename + "_%T" + ext;
            std::cout << "Writing to file '" << name << "', should be engine "
                      << requiredEngine << " (no explicit backend)."
                      << std::endl;
            auto filesystemname =
                basename + "_0" + (filesystemExt.empty() ? ext : filesystemExt);
            {
                Series write(name, Access::CREATE, jsonCfg);
                write.writeIterations()[0];
            }
            if (directory)
            {
                REQUIRE(auxiliary::directory_exists(filesystemname));
            }
            else
            {
                REQUIRE(auxiliary::file_exists(filesystemname));
            }
            {
                if (requiredEngine == "bp3" &&
                    !auxiliary::ends_with(name, ".bp"))
                {
                    /*
                     * File-based parsing procedures are not aware that BP3
                     * engine adds its ending and won't find the iterations if
                     * we don't give it a little nudge.
                     */
                    name += ".bp";
                }
                Series read(
                    name,
                    Access::READ_ONLY,
                    "backend = \"adios2\"\nadios2.engine.type = \"" +
                        requiredEngine + "\"");
            }
        };

    filebased_test_no_explicit_backend(".bp", true, "file", "");
    filebased_test_no_explicit_backend(".bp4", true, "bp4", "");
    filebased_test_no_explicit_backend(
        ".bp", true, "bp4", "", "adios2.engine.type = \"bp4\"");
    filebased_test_no_explicit_backend(
        ".bp4", true, "bp4", "", "adios2.engine.type = \"bp4\"");
    filebased_test_no_explicit_backend(
        ".bp5", true, "bp4", "", "adios2.engine.type = \"bp4\"");
    filebased_test_no_explicit_backend(
        ".bp", false, "bp3", "", "adios2.engine.type = \"bp3\"");
    filebased_test_no_explicit_backend(
        ".sst", false, "bp3", ".sst.bp", "adios2.engine.type = \"bp3\"");
    REQUIRE_THROWS(filebased_test_no_explicit_backend(
        "", false, "bp3", ".bp", "adios2.engine.type = \"bp3\""));
    REQUIRE_THROWS(filebased_test_no_explicit_backend(
        "", true, "bp4", "", "adios2.engine.type = \"bp4\""));

#if openPMD_HAVE_ADIOS2_BP5
    // BP5 tests
    filebased_test_no_explicit_backend(".bp5", true, "bp5", "");
    filebased_test_no_explicit_backend(
        ".bp", true, "bp5", "", "adios2.engine.type = \"bp5\"");
    filebased_test_no_explicit_backend(
        ".bp4", true, "bp5", "", "adios2.engine.type = \"bp5\"");
    filebased_test_no_explicit_backend(
        ".bp5", true, "bp5", "", "adios2.engine.type = \"bp5\"");
    REQUIRE_THROWS(filebased_test_no_explicit_backend(
        "", true, "bp5", "", "adios2.engine.type = \"bp5\""));
#endif
}

TEST_CASE("serial_adios2_backend_config", "[serial][adios2]")
{
    std::string writeConfigBP3 = R"END(
unused = "global parameter"

[hdf5]
unused = "hdf5 parameter please do not warn"

[adios2]
unused = "parameter"

[adios2.engine]
type = "bp3"
unused = "as well"

[adios2.engine.parameters]
BufferGrowthFactor = 2
Profile = "On"

# double brackets, because the operators are a list
[[adios2.dataset.operators]]
type = "blosc"
# It's possible to give parameters inline
parameters.clevel = "1"
parameters.doshuffle = "BLOSC_BITSHUFFLE"
)END";

    std::string writeConfigBP4 = R"END(
[adios2]
unused = "parameter"

[adios2.engine]
type = "bp4"
unused = "as well"

[adios2.engine.parameters]
BufferGrowthFactor = 2.0
Profile = "On"

[[adios2.dataset.operators]]
type = "blosc"

# even inside double brackets, we can go on with single brackets
[adios2.dataset.operators.parameters]
clevel = 1
doshuffle = "BLOSC_BITSHUFFLE"
)END";

    std::string writeConfigNull = R"END(
[adios2]
unused = "parameter"

[adios2.engine]
type = "nullcore"
unused = "as well"

[adios2.engine.parameters]
BufferGrowthFactor = "2.0"
Profile = "On"

[[adios2.dataset.operators]]
type = "blosc"
parameters.clevel = "1"
parameters.doshuffle = "BLOSC_BITSHUFFLE"
)END";
    /*
     * Notes on the upcoming dataset JSON configuration:
     * * The resizable key is needed by some backends (HDF5) so the backend can
     *   create a dataset that can later be resized.
     * * The asdf key should lead to a warning about unused parameters.
     * * Inside the hdf5 configuration, there are unused keys ("this").
     *   However, since this configuration is used by the ADIOS2 backend, there
     *   will be no warning for it.
     *
     * In the end, this config should lead to a warning similar to:
     * > Warning: parts of the JSON configuration for ADIOS2 dataset
     * > '/data/0/meshes/E/y' remain unused:
     * > {"adios2":{"dataset":{"unused":"too"},"unused":"dataset parameter"},
     * >  "asdf":"asdf"}
     */

    std::string datasetConfig = R"END(
resizable = true
asdf = "asdf"

[adios2]
unused = "dataset parameter"

[adios2.dataset]
unused = "too"

[[adios2.dataset.operators]]
type = "blosc"
[adios2.dataset.operators.parameters]
clevel = 3
doshuffle = "BLOSC_BITSHUFFLE"

[hdf5]
this = "should not warn"
)END";
    auto const write = [&datasetConfig](
                           std::string const &filename,
                           std::string const &config) {
        std::fstream file;
        file.open(
            "../samples/write_config.toml",
            std::ios_base::out | std::ios::binary);
        file << config;
        file.flush();
        openPMD::Series series(
            filename,
            openPMD::Access::CREATE,
            "@../samples/write_config.toml  ");
        auto E_x = series.iterations[0].meshes["E"]["x"];
        openPMD::Dataset ds(openPMD::Datatype::INT, {1000});
        E_x.resetDataset(ds);
        std::vector<int> data(1000, 0);
        E_x.storeChunk(data, {0}, {1000});

        auto E_y = series.iterations[0].meshes["E"]["y"];
        // let's override the global compression settings
        ds.options = datasetConfig;
        E_y.resetDataset(ds);
        E_y.storeChunk(data, {0}, {1000});
        series.flush();
    };
    write("../samples/jsonConfiguredBP4.bp", writeConfigBP4);
    write("../samples/jsonConfiguredBP3.bp", writeConfigBP3);
    write("../samples/jsonConfiguredNull.bp", writeConfigNull);

    // BP3 engine writes files, BP4 writes directories
    REQUIRE(openPMD::auxiliary::file_exists("../samples/jsonConfiguredBP3.bp"));
    REQUIRE(openPMD::auxiliary::directory_exists(
        "../samples/jsonConfiguredBP4.bp"));

    std::string readConfigBP3 = R"END(
{
  "adios2": {
    "engine": {
      "type": "bp3",
      "unused": "parameter"
    },
    "dataset": {
      "operators": [
        {
          "type": "blosc",
          "parameters": {
            "nthreads": 2
          }
        }
      ]
    }
  }
}
)END";
    std::string readConfigBP4 = R"END(
{
  "adios2": {
    "engine": {
      "type": "bp4",
      "unused": "parameter"
    },
    "dataset": {
      "operators": [
        {
          "type": "blosc",
          "parameters": {
            "nthreads": 2
          }
        }
      ]
    }
  }
}
)END";
    auto const read = [](std::string const &filename,
                         std::string const &config) {
        // let's write the config to a file and read it from there
        std::fstream file;
        file.open("../samples/read_config.json", std::ios_base::out);
        file << config;
        file.flush();
        openPMD::Series series(
            filename,
            openPMD::Access::READ_ONLY,
            "  @  ../samples/read_config.json    ");
        auto E_x = series.iterations[0].meshes["E"]["x"];
        REQUIRE(E_x.getDimensionality() == 1);
        REQUIRE(E_x.getExtent()[0] == 1000);
        auto chunk = E_x.loadChunk<int>({0}, {1000});
        series.flush();
        for (size_t i = 0; i < 1000; ++i)
        {
            REQUIRE(chunk.get()[i] == 0);
        }

        auto E_y = series.iterations[0].meshes["E"]["x"];
        REQUIRE(E_y.getDimensionality() == 1);
        REQUIRE(E_y.getExtent()[0] == 1000);
        chunk = E_y.loadChunk<int>({0}, {1000});
        series.flush();
        for (size_t i = 0; i < 1000; ++i)
        {
            REQUIRE(chunk.get()[i] == 0);
        }
    };
    read("../samples/jsonConfiguredBP3.bp", readConfigBP3);
    read("../samples/jsonConfiguredBP4.bp", readConfigBP4);
}

void bp4_steps(
    std::string const &file,
    std::string const &options_write,
    std::optional<Access> access = Access::READ_ONLY)
{
    {
        Series writeSeries(file, Access::CREATE, options_write);
        auto iterations = writeSeries.writeIterations();
        for (size_t i = 0; i < 10; ++i)
        {
            auto iteration = iterations[i];
            auto E = iteration.meshes["E"];
            auto E_x = E["x"];
            E.setAttribute(
                "vector_of_string",
                std::vector<std::string>{"vector", "of", "string"});
            E_x.resetDataset(openPMD::Dataset(openPMD::Datatype::INT, {10}));
            std::vector<int> data(10, i);
            E_x.storeChunk(data, {0}, {10});
            iteration.close();
        }
    }

    if (!access.has_value())
    {
        return;
    }

    Series readSeries(file, *access);

    size_t last_iteration_index = 0;
    for (auto iteration : readSeries.readIterations())
    {
        auto E = iteration.meshes["E"];
        auto E_x = E["x"];
        REQUIRE(
            E.getAttribute("vector_of_string")
                .get<std::vector<std::string>>() ==
            std::vector<std::string>{"vector", "of", "string"});
        REQUIRE(E_x.getDimensionality() == 1);
        REQUIRE(E_x.getExtent()[0] == 10);
        auto chunk = E_x.loadChunk<int>({0}, {10});
        iteration.close(); // @todo replace with ::close()
        for (size_t i = 0; i < 10; ++i)
        {
            REQUIRE(chunk.get()[i] == int(iteration.iterationIndex));
        }
        last_iteration_index = iteration.iterationIndex;
    }
    REQUIRE(last_iteration_index == 9);
}

TEST_CASE("bp4_steps", "[serial][adios2]")
{
    std::string bp4 = json::merge(
        R"(
    {
        "ADIOS2": {
            "engine": {
                "type": "bp4"
            }
        }
    }
    )",
#if openPMD_HAS_ADIOS_2_9
        R"({"ADIOS2":{"use_group_table": true}})"
#else
        R"({"ADIOS2":{"use_group_table": false}})"
#endif
    );
    std::string nullcore = R"(
    {
        "adios2": {
            "type": "nullcore"
        }
    }
    )";

    bp4_steps("../samples/bp4steps.bp", bp4);
    bp4_steps("../samples/nullcore.bp", nullcore, std::nullopt);
    bp4_steps("../samples/bp4steps_default.bp", "{}");
    // Can use READ_LINEAR with ADIOS2 v2.9 because then we have the group table
    // feature and can sensibly parse group-based encoding in step-based mode
    bp4_steps(
        "../samples/bp4steps.bp",
        bp4,
#if openPMD_HAS_ADIOS_2_9
        Access::READ_LINEAR
#else
        Access::READ_ONLY
#endif
    );

#if openPMD_HAS_ADIOS_2_9
    /*
     * Do this whole thing once more, but this time use the new attribute
     * layout.
     */
    bp4 = R"(
    {
        "adios2": {
            "use_group_table": true,
            "engine": {
                "type": "bp4"
            }
        }
    }
    )";
    bp4_steps("../samples/newlayout_bp4steps.bp", bp4, Access::READ_LINEAR);
    bp4_steps("../samples/newlayout_bp4steps.bp", bp4);
#endif
}
#endif

void serial_iterator(std::string const &file)
{
    constexpr Extent::value_type extent = 1000;
    {
        Series writeSeries(file, Access::CREATE);
        auto iterations = writeSeries.writeIterations();
        for (size_t i = 0; i < 10; ++i)
        {
            auto iteration = iterations[i];
            auto E_x = iteration.meshes["E"]["x"];
            E_x.resetDataset(openPMD::Dataset(openPMD::Datatype::INT, {1000}));
            std::vector<int> data(1000, i);
            E_x.storeChunk(data, {0}, {1000});
            iteration.close();
        }
    }

    Series readSeries(file, Access::READ_ONLY);

    size_t last_iteration_index = 0;
    size_t numberOfIterations = 0;
    for (auto iteration : readSeries.readIterations())
    {
        ++numberOfIterations;
        auto E_x = iteration.meshes["E"]["x"];
        REQUIRE(E_x.getDimensionality() == 1);
        REQUIRE(E_x.getExtent()[0] == extent);
        auto chunk = E_x.loadChunk<int>({0}, {extent});
        iteration.close();
        for (size_t i = 0; i < extent; ++i)
        {
            REQUIRE(chunk.get()[i] == int(iteration.iterationIndex));
        }
        last_iteration_index = iteration.iterationIndex;
    }
    REQUIRE(last_iteration_index == 9);
    REQUIRE(numberOfIterations == 10);
}

TEST_CASE("serial_iterator", "[serial][adios2]")
{
    for (auto const &t : testedFileExtensions())
    {
#ifdef _WIN32
        serial_iterator("../samples/serial_iterator_filebased_%T." + t);
        serial_iterator("../samples/serial_iterator_groupbased." + t);
#else
        // Add some regex characters into the file names to see that we can deal
        // with that. Don't do that on Windows because Windows does not like
        // those characters within file paths.
        serial_iterator("../samples/serial_iterator_filebased_+?_%T." + t);
        serial_iterator("../samples/serial_iterator_groupbased_+?." + t);
#endif
    }
}

void variableBasedSingleIteration(std::string const &file)
{
    constexpr Extent::value_type extent = 1000;
    {
        Series writeSeries(
            file,
            Access::CREATE,
            R"({"iteration_encoding": "variable_based"})");
        auto iterations = writeSeries.writeIterations();
        auto iteration = iterations[0];
        auto E_x = iteration.meshes["E"]["x"];
        E_x.resetDataset(openPMD::Dataset(openPMD::Datatype::INT, {1000}));
        std::vector<int> data(1000, 0);
        std::iota(data.begin(), data.end(), 0);
        E_x.storeChunk(data, {0}, {1000});
        writeSeries.flush();
        REQUIRE(
            writeSeries.iterationEncoding() ==
            IterationEncoding::variableBased);
    }

    {
        Series readSeries(file, Access::READ_LINEAR);
        readSeries.parseBase();

        auto E_x = readSeries.iterations[0].meshes["E"]["x"];
        REQUIRE(E_x.getDimensionality() == 1);
        REQUIRE(E_x.getExtent()[0] == extent);
        auto chunk = E_x.loadChunk<int>({0}, {extent});
        readSeries.flush();
        for (size_t i = 0; i < extent; ++i)
        {
            REQUIRE(chunk.get()[i] == int(i));
        }
    }
}

TEST_CASE("variableBasedSingleIteration", "[serial][adios2]")
{
    for (auto const &t : testedFileExtensions())
    {
        variableBasedSingleIteration(
            "../samples/variableBasedSingleIteration." + t);
    }
}

namespace epsilon
{
template <typename T>
struct AreEqual
{
    static bool areEqual(T float1, T float2)
    {
#if 0
        printf(
            "COMPARE %1.16e ~ %1.16e: %1.16e\tEQUAL: %d\n",
            float1,
            float2,
            std::abs( float1 - float2 ),
            std::abs( float1 - float2 ) <=
                std::numeric_limits< T >::epsilon() );
#endif
        return std::abs(float1 - float2) <= std::numeric_limits<T>::epsilon();
    }
};

template <typename T>
struct AreEqual<std::vector<T>>
{
    static bool areEqual(std::vector<T> v1, std::vector<T> v2)
    {
        return v1.size() == v2.size() &&
            std::equal(v1.begin(), v1.end(), v2.begin(), AreEqual<T>::areEqual);
    }
};

template <typename T>
bool areEqual(T a, T b)
{
    return AreEqual<T>::areEqual(std::move(a), std::move(b));
}
} // namespace epsilon

#if openPMD_HAVE_ADIOS2

#define openPMD_VERBOSE_CHUNKS 0

#if openPMD_VERBOSE_CHUNKS
static std::string format_chunk(ChunkInfo const &chunk_info)
{
    std::stringstream result;
    auto print_vector = [&result](auto const &vec) {
        if (vec.empty())
        {
            result << "[]";
        }
        else
        {
            auto it = vec.begin();
            result << '[' << *it++;
            auto end = vec.end();
            for (; it != end; ++it)
            {
                result << ',' << *it;
            }
            result << ']';
        }
    };
    result << '(';
    print_vector(chunk_info.offset);
    result << '|';
    print_vector(chunk_info.extent);
    result << ')';
    return result.str();
}
#endif

TEST_CASE("git_adios2_sample_test", "[serial][adios2]")
{
    using namespace epsilon;
    using vecstring = std::vector<std::string>;
    using vecdouble = std::vector<double>;
    using arr7 = std::array<double, 7>;

    std::string const samplePath =
        "../samples/git-sample/3d-bp4/example-3d-bp4.bp";
    std::string const samplePathFilebased =
        "../samples/git-sample/3d-bp4/example-3d-bp4_%T.bp";
    if (!auxiliary::directory_exists(samplePath))
    {
        std::cerr << "git sample '" << samplePath << "' not accessible \n";
        return;
    }

    /*
     * This checks a regression introduced by
     * https://github.com/openPMD/openPMD-api/pull/1498 and fixed by
     * https://github.com/openPMD/openPMD-api/pull/1586
     */
    for (auto const &[filepath, access] :
         {std::make_pair(samplePath, Access::READ_ONLY),
          std::make_pair(samplePathFilebased, Access::READ_ONLY),
          std::make_pair(samplePath, Access::READ_LINEAR),
          std::make_pair(samplePathFilebased, Access::READ_LINEAR)})
    {
        Series read(filepath, access);

        // false positive by clang-tidy?
        // NOLINTNEXTLINE(performance-for-range-copy)
        for (auto iteration : read.readIterations())
        {
            for (auto &mesh : iteration.meshes)
            {
                for (auto &component : mesh.second)
                {
#if openPMD_VERBOSE_CHUNKS
                    std::cout << "Chunks for '"
                              << component.second.myPath().openPMDPath()
                              << "':" << std::endl;
                    for (auto const &chunk : component.second.availableChunks())
                    {
                        std::cout << "\t" << format_chunk(chunk) << std::endl;
                    }
#else
                    component.second.availableChunks();
#endif
                }
            }
            for (auto &particle_species : iteration.particles)
            {
                for (auto &record : particle_species.second)
                {
                    for (auto &component : record.second)
                    {
#if openPMD_VERBOSE_CHUNKS
                        std::cout << "Chunks for '"
                                  << component.second.myPath().openPMDPath()
                                  << "':" << std::endl;
                        for (auto const &chunk :
                             component.second.availableChunks())
                        {
                            std::cout << "\t" << format_chunk(chunk)
                                      << std::endl;
                        }
#else
                        component.second.availableChunks();
#endif
                    }
                }
            }
        }
    }

    Series o(samplePath, Access::READ_ONLY, R"({"backend": "adios2"})");
    REQUIRE(o.openPMD() == "1.1.0");
    REQUIRE(o.openPMDextension() == 0);
    REQUIRE(o.basePath() == "/data/%T/");
    REQUIRE(o.meshesPath() == "fields/");
    REQUIRE(o.particlesPath() == "particles/");
    REQUIRE(o.iterationEncoding() == IterationEncoding::groupBased);
    REQUIRE(o.iterationFormat() == "/data/%T/");
    REQUIRE(o.name() == "example-3d-bp4");

    REQUIRE(o.iterations.size() == 1);
    REQUIRE(o.iterations.count(550) == 1);

    Iteration it = o.iterations[550];

    REQUIRE(areEqual(it.time<double>(), 5.5e+02));
    REQUIRE(areEqual(it.timeUnitSI(), 1.39e-16));
    REQUIRE(
        it.getAttribute("particleBoundary").get<vecstring>() ==
        vecstring(6, "absorbing"));
    REQUIRE(
        it.getAttribute("particleBoundaryParameters").get<vecstring>() ==
        vecstring(6, "without field correction"));
    REQUIRE(areEqual(
        it.getAttribute("mue0").get<float>(), 2.1550322708208114e-04f));
    REQUIRE(areEqual(
        it.getAttribute("eps0").get<float>(), 4.6403017578125000e+03f));
    REQUIRE(areEqual(it.dt<double>(), 1.));

    REQUIRE(it.meshes.size() == 9);
    REQUIRE(it.meshes.count("E") == 1);
    REQUIRE(it.meshes.count("B") == 1);
    REQUIRE(it.meshes.count("e_all_chargeDensity") == 1);
    REQUIRE(it.meshes.count("e_all_energyDensity") == 1);
    REQUIRE(it.meshes.count("e_all_particleMomentumComponent") == 1);
    REQUIRE(it.meshes.count("i_all_chargeDensity") == 1);
    REQUIRE(it.meshes.count("i_all_energyDensity") == 1);
    REQUIRE(it.meshes.count("i_all_particleMomentumComponent") == 1);
    // internal PIConGPU restarting information:
    REQUIRE(it.meshes.count("picongpu_idProvider") == 1);

    Mesh E = it.meshes["E"];
    REQUIRE(E.geometry() == Mesh::Geometry::cartesian);
    REQUIRE(E.dataOrder() == Mesh::DataOrder::C);
    REQUIRE(E.axisLabels() == vecstring{"z", "y", "x"});
    REQUIRE(areEqual(
        E.gridSpacing<double>(),
        vecdouble{
            1.7416797876358032e+00,
            1.7416797876358032e+00,
            1.7416797876358032e+00}));
    REQUIRE(areEqual(E.gridGlobalOffset(), vecdouble{0., 0., 0.}));
    REQUIRE(areEqual(E.gridUnitSI(), 5.3662849982000001e-08));
    REQUIRE(E.unitDimension() == arr7{{1, 1, -3, -1, 0, 0, 0}});
    REQUIRE(areEqual(E.timeOffset<double>(), 0.));

    REQUIRE(E.size() == 3);
    REQUIRE(E.count("x") == 1);
    REQUIRE(E.count("y") == 1);
    REQUIRE(E.count("z") == 1);

    MeshRecordComponent E_x = E["x"];
    REQUIRE(E_x.unitSI() == 9.5223987717519668e+12);
    REQUIRE(E_x.position<double>() == vecdouble{0.5, 0., 0.});
    REQUIRE(E_x.getDatatype() == Datatype::FLOAT);
    REQUIRE(E_x.getExtent() == Extent{32, 96, 64});
    REQUIRE(E_x.getDimensionality() == 3);

    float E_x_data[] = {-5.4223355837166309e-03, -5.5848993360996246e-03,
                        -5.7896804064512253e-03, -5.5147800594568253e-03,
                        -5.6304289028048515e-03, -5.8255749754607677e-03,
                        -5.5910930968821049e-03, -5.7385643012821674e-03,
                        -5.8903801254928112e-03, -5.3768581710755825e-03,
                        -5.5543538182973862e-03, -5.7734064757823944e-03,
                        -5.4399720393121243e-03, -5.5731507018208504e-03,
                        -5.7369144633412361e-03, -5.5461097508668900e-03,
                        -5.6645260192453861e-03, -5.8231339789927006e-03,
                        -5.4240114986896515e-03, -5.5798939429223537e-03,
                        -5.7610240764915943e-03, -5.4240110330283642e-03,
                        -5.5275037884712219e-03, -5.7047260925173759e-03,
                        -5.5050505325198174e-03, -5.6199040263891220e-03,
                        -5.7577718980610371e-03};
    auto E_x_loaded = E_x.loadChunk<float>({16, 32, 32}, {3, 3, 3});
    E_x.seriesFlush();
    for (size_t i = 0; i < 27; ++i)
    {
        REQUIRE(areEqual(E_x_data[i], E_x_loaded.get()[i]));
    }

    MeshRecordComponent E_y = E["y"];
    REQUIRE(E_y.unitSI() == 9.5223987717519668e+12);
    REQUIRE(E_y.position<double>() == vecdouble{0., 0.5, 0.});
    REQUIRE(E_y.getDatatype() == Datatype::FLOAT);
    REQUIRE(E_y.getExtent() == Extent{32, 96, 64});
    REQUIRE(E_y.getDimensionality() == 3);
    float E_y_data[] = {1.9600236555561423e-04,  1.9210868049412966e-04,
                        1.1112097854493186e-04,  9.0100722445640713e-05,
                        1.2735779455397278e-04,  1.2597699242178351e-04,
                        -4.5422813855111599e-05, 2.8805377951357514e-05,
                        8.3214777987450361e-05,  1.3271786156110466e-04,
                        1.0011527047026902e-04,  5.8875859394902363e-05,
                        2.5147232008748688e-05,  7.1912618295755237e-05,
                        6.2157545471563935e-05,  -8.6973857833072543e-05,
                        -8.1858233897946775e-06, -2.2509128029923886e-05,
                        6.0511985793709755e-05,  4.9726430006558076e-05,
                        -1.7196462067659013e-05, -3.0460794732789509e-05,
                        5.9892886383750010e-06,  -1.4382616200236953e-06,
                        -1.3747414050158113e-04, -8.0163808888755739e-05,
                        -3.5486038541421294e-05};
    auto E_y_loaded = E_y.loadChunk<float>({16, 32, 32}, {3, 3, 3});
    E_y.seriesFlush();
    for (size_t i = 0; i < 27; ++i)
    {
        REQUIRE(areEqual(E_y_data[i], E_y_loaded.get()[i]));
    }

    MeshRecordComponent E_z = E["z"];
    REQUIRE(E_z.unitSI() == 9.5223987717519668e+12);
    REQUIRE(E_z.position<double>() == vecdouble{0., 0., 0.5});
    REQUIRE(E_z.getDatatype() == Datatype::FLOAT);
    REQUIRE(E_z.getExtent() == Extent{32, 96, 64});
    REQUIRE(E_z.getDimensionality() == 3);
    float E_z_data[] = {-1.3665637234225869e-03, -1.3941071229055524e-03,
                        -1.4618652639910579e-03, -1.4528072206303477e-03,
                        -1.4355779858306050e-03, -1.4925430295988917e-03,
                        -1.6604729462414980e-03, -1.5911811497062445e-03,
                        -1.6420837491750717e-03, -1.1975304223597050e-03,
                        -1.2183464132249355e-03, -1.3470118865370750e-03,
                        -1.2645993847399950e-03, -1.2775690993294120e-03,
                        -1.3621025718748569e-03, -1.4198675053194165e-03,
                        -1.3927087420597672e-03, -1.3995743356645107e-03,
                        -9.9509279243648052e-04, -1.0950352298095822e-03,
                        -1.2131386902183294e-03, -1.0829739039763808e-03,
                        -1.1384176323190331e-03, -1.2189601548016071e-03,
                        -1.2028686469420791e-03, -1.1917919619008899e-03,
                        -1.2309787562116981e-03};
    auto E_z_loaded = E_z.loadChunk<float>({16, 32, 32}, {3, 3, 3});
    E_z.seriesFlush();
    for (size_t i = 0; i < 27; ++i)
    {
        REQUIRE(areEqual(E_z_data[i], E_z_loaded.get()[i]));
    }

    REQUIRE(it.particles.size() == 2);

    REQUIRE(it.particles.count("e") == 1);
    REQUIRE(it.particles.count("i") == 1);

    ParticleSpecies electrons = it.particles["e"];

    REQUIRE(electrons.size() == 6);
    REQUIRE(electrons.count("charge") == 1);
    REQUIRE(electrons.count("mass") == 1);
    REQUIRE(electrons.count("momentum") == 1);
    REQUIRE(electrons.count("position") == 1);
    REQUIRE(electrons.count("positionOffset") == 1);
    REQUIRE(electrons.count("weighting") == 1);

    Record charge = electrons["charge"];
    REQUIRE(charge.unitDimension() == arr7{{0., 0., 1., 1., 0., 0., 0.}});
    REQUIRE(charge.timeOffset<double>() == 0.0);

    REQUIRE(charge.size() == 1);
    REQUIRE(charge.count(RecordComponent::SCALAR) == 1);

    RecordComponent &charge_scalar = charge[RecordComponent::SCALAR];
    REQUIRE(areEqual(charge_scalar.unitSI(), 5.2323446053125002e-17));
    REQUIRE(charge_scalar.getDatatype() == Datatype::DOUBLE);
    REQUIRE(charge_scalar.getDimensionality() == 1);
    REQUIRE(charge_scalar.getExtent() == Extent{96781});
    double const charge_value = -3.0620612669736147e-3;
    REQUIRE(charge_scalar.getAttribute("value").get<double>() == charge_value);

    Record &mass = electrons["mass"];
    REQUIRE(mass.unitDimension() == arr7{{0., 1., 0., 0., 0., 0., 0.}});
    REQUIRE(mass.timeOffset<double>() == 0.0);

    REQUIRE(mass.size() == 1);
    REQUIRE(mass.count(RecordComponent::SCALAR) == 1);

    RecordComponent &mass_scalar = mass[RecordComponent::SCALAR];
    REQUIRE(areEqual(mass_scalar.unitSI(), 2.9749182215581054e-28));
    REQUIRE(mass_scalar.getDatatype() == Datatype::DOUBLE);
    REQUIRE(mass_scalar.getDimensionality() == 1);
    REQUIRE(mass_scalar.getExtent() == Extent{96781});
    double const mass_value = 3.0620612669736147e-3;
    REQUIRE(mass_scalar.getAttribute("value").get<double>() == mass_value);

    float position_x_data[] = {
        5.4244494438171387e-01,
        8.4322524070739746e-01,
        4.7133469581604004e-01,
        6.3491880893707275e-01,
        9.5489001274108887e-01,
        3.0412188172340393e-01,
        5.9818041324615479e-01,
        8.8785779476165771e-01,
        4.2273962497711182e-01};
    auto position_x_loaded =
        electrons["position"]["x"].loadChunk<float>({32}, {9});
    auto charge_loaded = charge_scalar.loadChunk<double>({32}, {9});
    auto mass_loaded = mass_scalar.loadChunk<double>({32}, {9});
    electrons.seriesFlush();
    for (size_t i = 0; i < 9; ++i)
    {
        REQUIRE(areEqual(position_x_data[i], position_x_loaded.get()[i]));
        REQUIRE(areEqual(charge_value, charge_loaded.get()[i]));
        REQUIRE(areEqual(mass_value, mass_loaded.get()[i]));
    }

    float position_y_data[] = {
        3.8391970098018646e-02,
        7.0238715410232544e-01,
        5.0294065475463867e-01,
        6.7421793937683105e-01,
        2.3211851716041565e-01,
        2.6738378405570984e-01,
        8.2502347230911255e-01,
        9.2121642827987671e-01,
        9.0402549505233765e-01};
    auto position_y_loaded =
        electrons["position"]["y"].loadChunk<float>({32}, {9});
    electrons.seriesFlush();
    for (size_t i = 0; i < 9; ++i)
    {
        REQUIRE(areEqual(position_y_data[i], position_y_loaded.get()[i]));
    }

    float position_z_data[] = {
        4.3061500787734985e-01,
        7.9804927110671997e-01,
        3.8822534680366516e-01,
        8.4099531173706055e-01,
        6.3664638996124268e-01,
        7.4185878038406372e-01,
        4.5986607670783997e-01,
        2.2350004315376282e-01,
        5.4723143577575684e-01};
    auto position_z_loaded =
        electrons["position"]["z"].loadChunk<float>({32}, {9});
    electrons.seriesFlush();
    for (size_t i = 0; i < 9; ++i)
    {
        REQUIRE(areEqual(position_z_data[i], position_z_loaded.get()[i]));
    }
}

#if openPMD_HAS_ADIOS_2_9
void adios2_group_table(
    std::string const &jsonWrite,
    std::string const &jsonRead,
    bool canDeleteGroups)
{
    Series write(
        "../samples/group_table.bp",
        Access::CREATE,
        json::merge(R"(iteration_encoding = "variable_based")", jsonWrite));
    // write E_x and E_y in iteration 0, only E_x in iteration 1
    write.writeIterations()[0].meshes["E"]["x"].makeEmpty(Datatype::FLOAT, 1);
    write.writeIterations()[0].meshes["E"]["y"].makeEmpty(Datatype::FLOAT, 1);
    write.writeIterations()[1].meshes["E"]["x"].makeEmpty(Datatype::FLOAT, 1);
    write.close();

    Series read("../samples/group_table.bp", Access::READ_LINEAR, jsonRead);
    // NOLINTNEXTLINE(performance-for-range-copy)
    for (auto iteration : read.readIterations())
    {
        switch (iteration.iterationIndex)
        {
        case 0:
            REQUIRE(iteration.meshes["E"].contains("x"));
            REQUIRE(iteration.meshes["E"].contains("y"));
            REQUIRE(iteration.meshes["E"].size() == 2);
            break;
        case 1:
            if (canDeleteGroups)
            {
                REQUIRE(iteration.meshes["E"].contains("x"));
                REQUIRE(iteration.meshes["E"].size() == 1);
            }
            else
            {
                REQUIRE(iteration.meshes["E"].contains("x"));
                REQUIRE(iteration.meshes["E"].contains("y"));
                REQUIRE(iteration.meshes["E"].size() == 2);
            }
            break;
        }
    }
}

TEST_CASE("adios2_group_table", "[serial]")
{
    std::string useGroupTable = R"(adios2.use_group_table = true)";
    std::string noGroupTable = R"(adios2.use_group_table = false)";
    adios2_group_table(useGroupTable, useGroupTable, true);
    adios2_group_table(noGroupTable, useGroupTable, false);
    adios2_group_table(useGroupTable, noGroupTable, false);
    adios2_group_table(noGroupTable, noGroupTable, false);
}

void variableBasedSeries(std::string const &file)
{
    constexpr Extent::value_type extent = 1000;
    auto testWrite = [&file](std::string const &jsonConfig) {
        Series writeSeries(file, Access::CREATE, jsonConfig);
        writeSeries.setAttribute("some_global", "attribute");
        writeSeries.setIterationEncoding(IterationEncoding::variableBased);
        REQUIRE(
            writeSeries.iterationEncoding() ==
            IterationEncoding::variableBased);
        auto iterations = writeSeries.writeIterations();
        bool is_not_adios2 = writeSeries.backend() != "ADIOS2";
        for (size_t i = 0; i < 10; ++i)
        {
            if (i > 0 && is_not_adios2)
            {
                REQUIRE_THROWS_AS(
                    iterations[i], error::OperationUnsupportedInBackend);
                return;
            }
            auto iteration = iterations[i];
            auto E_x = iteration.meshes["E"]["x"];
            E_x.resetDataset({openPMD::Datatype::INT, {1000}});
            std::vector<int> data(1000, i);
            E_x.storeChunk(data, {0}, {1000});

            if (i > 2)
            {
                iteration.setAttribute(
                    "iteration_is_larger_than_two", "it truly is");
            }

            iteration.setAttribute("changing_value", i);

            // this tests changing extents and dimensionalities
            // across iterations
            auto E_y = iteration.meshes["E"]["y"];
            unsigned dimensionality = i % 3 + 1;
            unsigned len = i + 1;
            Extent changingExtent(dimensionality, len);
            E_y.resetDataset({openPMD::Datatype::INT, changingExtent});
            std::vector<int> changingData(
                std::pow(len, dimensionality), dimensionality);
            E_y.storeChunk(
                changingData, Offset(dimensionality, 0), changingExtent);

            // this tests datasets that are present in one iteration, but not
            // in others
            auto E_z = iteration.meshes["E"][std::to_string(i)];
            E_z.resetDataset({Datatype::INT, {1}});
            E_z.makeConstant(i);
            // this tests attributes that are present in one iteration, but not
            // in others
            iteration.meshes["E"].setAttribute("attr_" + std::to_string(i), i);

            auto constantMesh =
                iteration.meshes["changing_constant"][RecordComponent::SCALAR];
            constantMesh.resetDataset({Datatype::INT, {i}});
            constantMesh.makeConstant(i);

            auto constantParticles =
                iteration.particles["changing_constant"]["position"]
                                   [RecordComponent::SCALAR];
            constantParticles.resetDataset({Datatype::INT, {i}});
            constantParticles.makeConstant(i);

            iteration.close();
        }
        REQUIRE(auxiliary::directory_exists(file));
    };

    auto testRead = [&file, &extent](
                        std::string const &parseMode,
                        bool supportsModifiableAttributes) {
        /*
         * Need linear read mode to access more than a single iteration in
         * variable-based iteration encoding.
         */
        Series readSeries(file, Access::READ_LINEAR, parseMode);
        bool is_adios2 = readSeries.backend() == "ADIOS2";

        size_t last_iteration_index = 0;
        REQUIRE(!readSeries.containsAttribute("some_global"));
        readSeries.parseBase();
        REQUIRE(
            readSeries.getAttribute("some_global").get<std::string>() ==
            "attribute");
        for (auto iteration : readSeries.readIterations())
        {
            if (iteration.iterationIndex > 2)
            {
                REQUIRE(
                    iteration.getAttribute("iteration_is_larger_than_two")
                        .get<std::string>() == "it truly is");
            }
            else
            {
                REQUIRE_FALSE(iteration.containsAttribute(
                    "iteration_is_larger_than_two"));
            }

            // If modifiable attributes are unsupported, the attribute is
            // written once in step 0 and then never changed
            // A warning is printed upon trying to write
            if (is_adios2)
            {
                REQUIRE(
                    iteration.getAttribute("changing_value").get<unsigned>() ==
                    (supportsModifiableAttributes ? iteration.iterationIndex
                                                  : 0));
            }
            auto E_x = iteration.meshes["E"]["x"];
            REQUIRE(E_x.getDimensionality() == 1);
            REQUIRE(E_x.getExtent()[0] == extent);
            auto chunk = E_x.loadChunk<int>({0}, {extent});
            iteration.close();
            for (size_t i = 0; i < extent; ++i)
            {
                REQUIRE(chunk.get()[i] == int(iteration.iterationIndex));
            }

            auto E_y = iteration.meshes["E"]["y"];
            unsigned dimensionality = iteration.iterationIndex % 3 + 1;
            unsigned len = iteration.iterationIndex + 1;
            Extent changingExtent(dimensionality, len);
            REQUIRE(E_y.getExtent() == changingExtent);

            // this loop ensures that only the recordcomponent ["E"]["i"] is
            // present where i == iteration.iterationIndex
            for (uint64_t otherIteration = 0; otherIteration < 10;
                 ++otherIteration)
            {
                // component is present <=> (otherIteration == i)
                REQUIRE(
                    iteration.meshes["E"].contains(
                        std::to_string(otherIteration)) ==
                    (otherIteration == iteration.iterationIndex));
                REQUIRE(
                    iteration.meshes["E"].containsAttribute(
                        "attr_" + std::to_string(otherIteration)) ==
                    (otherIteration <= iteration.iterationIndex));
            }
            REQUIRE(
                iteration.meshes["E"][std::to_string(iteration.iterationIndex)]
                    .getAttribute("value")
                    .get<int>() == int(iteration.iterationIndex));
            REQUIRE(
                iteration.meshes["E"]
                    .getAttribute(
                        "attr_" + std::to_string(iteration.iterationIndex))
                    .get<int>() == int(iteration.iterationIndex));

            auto constantMesh =
                iteration.meshes["changing_constant"][RecordComponent::SCALAR];
            REQUIRE(
                constantMesh.getExtent() ==
                std::vector{iteration.iterationIndex});
            REQUIRE(
                constantMesh.getAttribute("value").get<unsigned>() ==
                iteration.iterationIndex);

            auto constantParticles =
                iteration.particles["changing_constant"]["position"]
                                   [RecordComponent::SCALAR];
            REQUIRE(
                constantParticles.getExtent() ==
                std::vector{iteration.iterationIndex});
            REQUIRE(
                constantParticles.getAttribute("value").get<unsigned>() ==
                iteration.iterationIndex);

            last_iteration_index = iteration.iterationIndex;
        }
        REQUIRE(last_iteration_index == (is_adios2 ? 9 : 0));
    };

    std::string jsonConfig = R"(
{
  "adios2": {
    "modifiable_attributes": true
  }
})";
    testWrite(jsonConfig);
    REQUIRE(
        (auxiliary::directory_exists(file) || auxiliary::file_exists(file)));
    testRead(
        "{\"defer_iteration_parsing\": true}",
        /*supportsModifiableAttributes = */ true);
    testRead(
        "{\"defer_iteration_parsing\": false}",
        /*supportsModifiableAttributes = */ true);

    jsonConfig = "{}";
    testWrite(jsonConfig);
    testRead(
        "{\"defer_iteration_parsing\": true}",
        /*supportsModifiableAttributes = */ true);
    testRead(
        "{\"defer_iteration_parsing\": false}",
        /*supportsModifiableAttributes = */ true);

    jsonConfig = R"(
{
  "adios2": {
    "modifiable_attributes": false
  }
})";
    testWrite(jsonConfig);
    testRead(
        "{\"defer_iteration_parsing\": true}",
        /*supportsModifiableAttributes = */ false);
    testRead(
        "{\"defer_iteration_parsing\": false}",
        /*supportsModifiableAttributes = */ false);
}

TEST_CASE("variableBasedSeries", "[serial][adios2]")
{
    for (auto const &t : testedFileExtensions())
    {
        variableBasedSeries("../samples/variableBasedSeries." + t);
    }
}

void variableBasedParticleData()
{
    using position_t = double;
    constexpr unsigned long length = 10ul;

    {
        // open file for writing
        Series series =
            Series("../samples/variableBasedParticles.bp", Access::CREATE);
        series.setIterationEncoding(IterationEncoding::variableBased);

        Datatype datatype = determineDatatype<position_t>();
        Extent global_extent = {length};
        Dataset dataset = Dataset(datatype, global_extent);
        std::shared_ptr<position_t> local_data(
            new position_t[length],
            [](position_t const *ptr) { delete[] ptr; });

        WriteIterations iterations = series.writeIterations();
        for (size_t i = 0; i < 10; ++i)
        {
            Iteration iteration = iterations[i];
            Record electronPositions = iteration.particles["e"]["position"];

            std::iota(local_data.get(), local_data.get() + length, i * length);
            for (auto const &dim : {"x", "y", "z"})
            {
                RecordComponent pos = electronPositions[dim];
                pos.resetDataset(dataset);
                pos.storeChunk(local_data, Offset{0}, global_extent);
            }
            iteration.close();
        }
    }

    {
        // open file for reading
        Series series =
            Series("../samples/variableBasedParticles.bp", Access::READ_LINEAR);

        for (IndexedIteration iteration : series.readIterations())
        {
            Record electronPositions = iteration.particles["e"]["position"];
            std::array<std::shared_ptr<position_t>, 3> loadedChunks;
            std::array<Extent, 3> extents;
            std::array<std::string, 3> const dimensions{{"x", "y", "z"}};

            for (size_t i = 0; i < 3; ++i)
            {
                std::string const &dim = dimensions[i];
                RecordComponent rc = electronPositions[dim];
                loadedChunks[i] = rc.loadChunk<position_t>(
                    Offset(rc.getDimensionality(), 0), rc.getExtent());
                extents[i] = rc.getExtent();
            }

            iteration.close();

            for (size_t i = 0; i < 3; ++i)
            {
                Extent const &extent = extents[i];
                auto chunk = loadedChunks[i];
                for (size_t j = 0; j < extent[0]; ++j)
                {
                    REQUIRE(
                        chunk.get()[j] ==
                        iteration.iterationIndex * length + j);
                }
            }
        }
    }
}

TEST_CASE("variableBasedParticleData", "[serial][adios2]")
{
    variableBasedParticleData();
}
#endif // openPMD_HAS_ADIOS_2_9
#endif // openPMD_HAS_ADIOS2

#if openPMD_HAVE_ADIOS2
#ifdef ADIOS2_HAVE_BZIP2
TEST_CASE("automatically_deactivate_span", "[serial][adios2]")
{
    // automatically (de)activate span-based storeChunking
    {
        Series write("../samples/span_based.bp", Access::CREATE);
        auto E_uncompressed = write.iterations[0].meshes["E"]["x"];
        auto E_compressed = write.iterations[0].meshes["E"]["y"];

        Dataset ds{Datatype::INT, {10}};

        E_uncompressed.resetDataset(ds);

        std::string compression = R"END(
{
  "adios2": {
    "dataset": {
      "operators": [
        {
          "type": "bzip2"
        }
      ]
    }
  }
})END";

        ds.options = compression;
        E_compressed.resetDataset(ds);

        bool spanWorkaround = false;
        E_uncompressed.storeChunk<int>(
            {0}, {10}, [&spanWorkaround](size_t size) {
                spanWorkaround = true;
                return std::shared_ptr<int>(
                    new int[size]{}, [](auto *ptr) { delete[] ptr; });
            });

        REQUIRE(!spanWorkaround);

        E_compressed.storeChunk<int>({0}, {10}, [&spanWorkaround](size_t size) {
            spanWorkaround = true;
            return std::shared_ptr<int>(
                new int[size]{}, [](auto *ptr) { delete[] ptr; });
        });

        REQUIRE(spanWorkaround);
    }

    // enable span-based API indiscriminately
    {
        std::string enable = R"END(
{
  "adios2": {
    "use_span_based_put": true
  }
})END";
        Series write("../samples/span_based.bp", Access::CREATE, enable);
        auto E_uncompressed = write.iterations[0].meshes["E"]["x"];
        auto E_compressed = write.iterations[0].meshes["E"]["y"];

        Dataset ds{Datatype::INT, {10}};

        E_uncompressed.resetDataset(ds);

        std::string compression = R"END(
{
  "adios2": {
    "dataset": {
      "operators": [
        {
          "type": "bzip2"
        }
      ]
    }
  }
})END";

        ds.options = compression;
        E_compressed.resetDataset(ds);

        bool spanWorkaround = false;
        E_uncompressed.storeChunk<int>(
            {0}, {10}, [&spanWorkaround](size_t size) {
                spanWorkaround = true;
                return std::shared_ptr<int>(
                    new int[size]{}, [](auto *ptr) { delete[] ptr; });
            });

        REQUIRE(!spanWorkaround);

        try
        {
            E_compressed.storeChunk<int>(
                {0}, {10}, [&spanWorkaround](size_t size) {
                    spanWorkaround = true;
                    return std::shared_ptr<int>(
                        new int[size]{}, [](auto *ptr) { delete[] ptr; });
                });
        }
        catch (std::invalid_argument const &e)
        {
            /*
             * Using the span-based API in combination with compression is
             * unsupported in ADIOS2.
             * In newer versions of ADIOS2, an error is thrown.
             */
            std::cerr << "Ignoring expected error: " << e.what() << std::endl;
        }

        REQUIRE(!spanWorkaround);
    }

    // disable span-based API indiscriminately
    {
        std::string disable = R"END(
{
  "adios2": {
    "use_span_based_put": false
  }
})END";
        Series write("../samples/span_based.bp", Access::CREATE, disable);
        auto E_uncompressed = write.iterations[0].meshes["E"]["x"];
        auto E_compressed = write.iterations[0].meshes["E"]["y"];

        Dataset ds{Datatype::INT, {10}};

        E_uncompressed.resetDataset(ds);

        std::string compression = R"END(
{
  "adios2": {
    "dataset": {
      "operators": [
        {
          "type": "bzip2"
        }
      ]
    }
  }
})END";

        ds.options = compression;
        E_compressed.resetDataset(ds);

        bool spanWorkaround = false;
        E_uncompressed.storeChunk<int>(
            {0}, {10}, [&spanWorkaround](size_t size) {
                spanWorkaround = true;
                return std::shared_ptr<int>(
                    new int[size]{}, [](auto *ptr) { delete[] ptr; });
            });

        REQUIRE(spanWorkaround);

        spanWorkaround = false;
        E_compressed.storeChunk<int>({0}, {10}, [&spanWorkaround](size_t size) {
            spanWorkaround = true;
            return std::shared_ptr<int>(
                new int[size]{}, [](auto *ptr) { delete[] ptr; });
        });

        REQUIRE(spanWorkaround);
    }
}
#endif
#endif

// @todo Upon switching to ADIOS2 2.7.0, test this the other way around also
void iterate_nonstreaming_series(
    std::string const &file,
    bool variableBasedLayout,
    std::string const &jsonConfig)
{
    constexpr size_t extent = 100;
    {
        Series writeSeries(file, Access::CREATE, jsonConfig);
        if (variableBasedLayout)
        {
            writeSeries.setIterationEncoding(IterationEncoding::variableBased);
        }
        // use conventional API to write iterations
        auto iterations = writeSeries.iterations;
        for (size_t i = 0; i < 10; ++i)
        {
            auto iteration = iterations[i];
            auto E_x = iteration.meshes["E"]["x"];
            E_x.resetDataset(
                openPMD::Dataset(openPMD::Datatype::INT, {2, extent}));
            int value = variableBasedLayout ? 0 : i;
            std::vector<int> data(extent, value);
            E_x.storeChunk(data, {0, 0}, {1, extent});
            bool taskSupportedByBackend = true;
            DynamicMemoryView<int> memoryView;
            {
                auto currentBuffer = memoryView.currentBuffer();
                REQUIRE(currentBuffer.data() == nullptr);
                REQUIRE(currentBuffer.size() == 0);
            }
            memoryView = E_x.storeChunk<int>(
                {1, 0},
                {1, extent},
                /*
                 * Hijack the functor that is called for buffer creation.
                 * This allows us to check if the backend has explicit support
                 * for buffer creation or if the fallback implementation is
                 * used.
                 */
                [&taskSupportedByBackend](size_t size) {
                    taskSupportedByBackend = false;
                    return std::shared_ptr<int>{
                        new int[size], [](auto *ptr) { delete[] ptr; }};
                });
            if (writeSeries.backend() == "ADIOS2")
            {
                // that backend must support span creation
                REQUIRE(taskSupportedByBackend);
            }
            auto span = memoryView.currentBuffer();
            for (size_t j = 0; j < span.size(); ++j)
            {
                span[j] = j;
            }

            /*
             * This is to test whether defaults are correctly written for
             * scalar record components since there previously was a bug.
             */
            auto scalarMesh =
                iteration
                    .meshes["i_energyDensity"][MeshRecordComponent::SCALAR];
            scalarMesh.resetDataset(Dataset(Datatype::INT, {5}));
            auto scalarSpan =
                scalarMesh.storeChunk<int>({0}, {5}).currentBuffer();
            for (size_t j = 0; j < scalarSpan.size(); ++j)
            {
                scalarSpan[j] = j;
            }
            // we encourage manually closing iterations, but it should not
            // matter so let's do the switcharoo for this test
            if (i % 2 == 0)
            {
                writeSeries.flush();
            }
            else
            {
                iteration.close();
            }
        }
    }

    for (auto access : {Access::READ_LINEAR, Access::READ_ONLY})
    {
        Series readSeries(
            file,
            access,
            json::merge(jsonConfig, R"({"defer_iteration_parsing": true})"));

        size_t last_iteration_index = 0;
        // conventionally written Series must be readable with streaming-aware
        // API!
        for (auto iteration : readSeries.readIterations())
        {
            // ReadIterations takes care of Iteration::open()ing iterations
            auto E_x = iteration.meshes["E"]["x"];
            REQUIRE(E_x.getDimensionality() == 2);
            REQUIRE(E_x.getExtent()[0] == 2);
            REQUIRE(E_x.getExtent()[1] == extent);
            auto chunk = E_x.loadChunk<int>({0, 0}, {1, extent});
            auto chunk2 = E_x.loadChunk<int>({1, 0}, {1, extent});
            // we encourage manually closing iterations, but it should not
            // matter so let's do the switcharoo for this test
            if (last_iteration_index % 2 == 0)
            {
                readSeries.flush();
            }
            else
            {
                iteration.close();
            }

            int value = variableBasedLayout ? 0 : iteration.iterationIndex;
            for (size_t i = 0; i < extent; ++i)
            {
                REQUIRE(chunk.get()[i] == value);
                REQUIRE(chunk2.get()[i] == int(i));
            }
            last_iteration_index = iteration.iterationIndex;
        }
        REQUIRE(last_iteration_index == 9);
    }
}

TEST_CASE("iterate_nonstreaming_series", "[serial][adios2]")
{
    for (auto const &backend : testedBackends())
    {
        iterate_nonstreaming_series(
            "../samples/iterate_nonstreaming_series_filebased_%T." +
                backend.extension,
            false,
            backend.jsonBaseConfig());
        iterate_nonstreaming_series(
            "../samples/iterate_nonstreaming_series_groupbased." +
                backend.extension,
            false,
            backend.jsonBaseConfig());
#if openPMD_HAVE_ADIOS2 && openPMD_HAVE_ADIOS2_BP5
        if (backend.extension == "bp")
        {
            iterate_nonstreaming_series(
                "../samples/iterate_nonstreaming_series_filebased_bp5_%T." +
                    backend.extension,
                false,
                json::merge(
                    backend.jsonBaseConfig(), "adios2.engine.type = \"bp5\""));
            iterate_nonstreaming_series(
                "../samples/iterate_nonstreaming_series_groupbased_bp5." +
                    backend.extension,
                false,
                json::merge(
                    backend.jsonBaseConfig(), "adios2.engine.type = \"bp5\""));
        }
#endif
    }
#if openPMD_HAVE_ADIOS2 && openPMD_HAS_ADIOS_2_9
    iterate_nonstreaming_series(
        "../samples/iterate_nonstreaming_series_variablebased.bp",
        true,
        R"({"backend": "adios2"})");
#endif
}

#if openPMD_HAVE_ADIOS2 && openPMD_HAVE_ADIOS2_BP5
void adios2_bp5_no_steps(bool usesteps)
{
    std::string const config = R"END(
{
    "adios2":
    {
        "use_group_table": true
    }
})END";
    {
        adios2::ADIOS adios;
        auto IO = adios.DeclareIO("IO");
        IO.SetEngine("bp5");
        auto engine =
            IO.Open("../samples/bp5_no_steps.bp", adios2::Mode::Write);
        if (usesteps)
        {
            engine.BeginStep();
        }

        // write default openPMD attributes
        IO.DefineAttribute("/basePath", std::string("/data/%T/"));
        IO.DefineAttribute("/date", std::string("2021-02-22 11:14:00 +0000"));
        IO.DefineAttribute("/iterationEncoding", std::string("groupBased"));
        IO.DefineAttribute("/iterationFormat", std::string("/data/%T/"));
        IO.DefineAttribute("/meshesPath", std::string("meshes/"));
        IO.DefineAttribute("/openPMD", std::string("1.1.0"));
        IO.DefineAttribute("/openPMDextension", uint32_t(0));
        IO.DefineAttribute("/software", std::string("openPMD-api"));
        IO.DefineAttribute("/softwareVersion", std::string("0.17.0-dev"));

        IO.DefineAttribute("/data/0/dt", double(1));
        IO.DefineAttribute(
            "/data/0/meshes/theta/axisLabels",
            std::vector<std::string>{"x"}.data(),
            1);
        IO.DefineAttribute("/data/0/meshes/theta/dataOrder", std::string("C"));
        IO.DefineAttribute(
            "/data/0/meshes/theta/geometry", std::string("cartesian"));
        IO.DefineAttribute("/data/0/meshes/theta/gridGlobalOffset", double(0));
        IO.DefineAttribute("/data/0/meshes/theta/gridSpacing", double(1));
        IO.DefineAttribute("/data/0/meshes/theta/gridUnitSI", double(1));
        IO.DefineAttribute("/data/0/meshes/theta/position", double(0));
        IO.DefineAttribute("/data/0/meshes/theta/timeOffset", double(0));
        IO.DefineAttribute(
            "/data/0/meshes/theta/unitDimension",
            std::vector<double>(7, 0).data(),
            7);
        IO.DefineAttribute("/data/0/meshes/theta/unitSI", double(1));
        IO.DefineAttribute("/data/0/time", double(0));
        IO.DefineAttribute("/data/0/timeUnitSI", double(1));

        IO.DefineAttribute<uint64_t>(
            "__openPMD_internal/openPMD2_adios2_schema", 0);
        IO.DefineAttribute<unsigned char>("__openPMD_internal/useSteps", 0);

        std::vector<int> data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        engine.Put(
            IO.DefineVariable<int>("/data/0/meshes/theta", {10}, {0}, {10}),
            data.data());

        if (usesteps)
        {
            engine.EndStep();
        }
        engine.Close();
    }

    {
        Series read(
            "../samples/bp5_no_steps.bp",
            Access::READ_ONLY,
            "adios2.engine.type = \"bp5\"");
        auto data = read.iterations[0]
                        .meshes["theta"][RecordComponent::SCALAR]
                        .loadChunk<int>({0}, {10});
        read.flush();
        for (size_t i = 0; i < 10; ++i)
        {
            REQUIRE(data.get()[i] == int(i));
        }
    }
}

TEST_CASE("adios2_bp5_no_steps", "[serial][adios2]")
{
    adios2_bp5_no_steps(/* usesteps = */ false);
    adios2_bp5_no_steps(/* usesteps = */ true);
}
#endif

void extendDataset(std::string const &ext, std::string const &jsonConfig)
{
    std::string filename = "../samples/extendDataset." + ext;
    std::vector<int> data1(25);
    std::vector<int> data2(25);
    std::iota(data1.begin(), data1.end(), 0);
    std::iota(data2.begin(), data2.end(), 25);
    {
        Series write(filename, Access::CREATE, jsonConfig);
        // only one iteration written anyway
        write.setIterationEncoding(IterationEncoding::variableBased);

        Dataset ds1{
            Datatype::INT,
            {5, 5},
            R"({ "resizable": true, "resizeble": "typo" })"};
        Dataset ds2{Datatype::INT, {10, 5}};

        // array record component -> array record component
        // should work
        auto E_x = write.iterations[0].meshes["E"]["x"];
        E_x.resetDataset(ds1);
        E_x.storeChunk(data1, {0, 0}, {5, 5});
        write.flush();

        E_x.resetDataset(ds2);
        E_x.storeChunk(data2, {5, 0}, {5, 5});

        // constant record component -> constant record component
        // should work
        auto E_y = write.iterations[0].meshes["E"]["y"];
        E_y.resetDataset(ds1);
        E_y.makeConstant(10);
        write.flush();

        E_y.resetDataset(ds2);
        write.flush();

        // empty record component -> empty record component
        // should work
        // this does not make a lot of sense since we don't allow shrinking,
        // but let's just reset it to itself
        auto E_z = write.iterations[0].meshes["E"]["z"];
        E_z.makeEmpty<int>(3);
        write.flush();

        E_z.makeEmpty<int>(3);
        write.flush();

        // empty record component -> empty record component
        // (created by resetDataset)
        // should work
        auto E_a = write.iterations[0].meshes["E"]["a"];
        E_a.makeEmpty<int>(3);
        write.flush();

        E_a.resetDataset(Dataset(Datatype::UNDEFINED, {0, 1, 2}));
        write.flush();

        // constant record component -> empty record component
        // should fail, since this implies shrinking
        auto E_b = write.iterations[0].meshes["E"]["b"];
        E_b.resetDataset(ds1);
        E_b.makeConstant(10);
        write.flush();

        REQUIRE_THROWS(E_b.makeEmpty<int>(2));

        // empty record component -> constant record component
        // should work
        auto E_c = write.iterations[0].meshes["E"]["c"];
        E_c.makeEmpty<int>(3);
        write.flush();

        E_c.resetDataset(Dataset({1, 1, 2}));
        write.flush();

        // array record component -> constant record component
        // should fail
        auto E_d = write.iterations[0].meshes["E"]["d"];
        E_d.resetDataset(ds1);
        E_d.storeChunk(data1, {0, 0}, {5, 5});
        write.flush();

        REQUIRE_THROWS(E_d.makeConstant(5));

        // array record component -> empty record component
        // should fail
        auto E_e = write.iterations[0].meshes["E"]["e"];
        E_e.resetDataset(ds1);
        E_e.storeChunk(data1, {0, 0}, {5, 5});
        write.flush();

        REQUIRE_THROWS(E_e.makeEmpty<int>(5));
    }

    {
        Series read(filename, Access::READ_ONLY, jsonConfig);
        auto E_x = read.iterations[0].meshes["E"]["x"];
        REQUIRE(E_x.getExtent() == Extent{10, 5});
        auto chunk = E_x.loadChunk<int>({0, 0}, {10, 5});
        read.flush();

        for (size_t i = 0; i < 50; ++i)
        {
            REQUIRE(chunk.get()[i] == int(i));
        }

        auto E_y = read.iterations[0].meshes["E"]["y"];
        REQUIRE(E_y.getExtent() == Extent{10, 5});

        auto E_z = read.iterations[0].meshes["E"]["z"];
        REQUIRE(E_z.getExtent() == Extent{0, 0, 0});

        auto E_a = read.iterations[0].meshes["E"]["a"];
        REQUIRE(E_a.getExtent() == Extent{0, 1, 2});

        // E_b could not be changed

        auto E_c = read.iterations[0].meshes["E"]["c"];
        REQUIRE(E_c.getExtent() == Extent{1, 1, 2});
        REQUIRE(!E_c.empty());
    }
}

TEST_CASE("extend_dataset", "[serial]")
{
    extendDataset("json", R"({"backend": "json"})");
#if openPMD_HAVE_ADIOS2
    extendDataset("bp", R"({"backend": "adios2"})");
#endif
#if openPMD_HAVE_HDF5
    // extensible datasets require chunking
    // skip this test for if chunking is disabled
    if (auxiliary::getEnvString("OPENPMD_HDF5_CHUNKS", "auto") != "none")
    {
        extendDataset("h5", R"({"backend": "hdf5"})");
    }
#endif
}

void deferred_parsing(std::string const &extension)
{
    if (auxiliary::directory_exists("../samples/lazy_parsing"))
        auxiliary::remove_directory("../samples/lazy_parsing");
    std::string basename = "../samples/lazy_parsing/lazy_parsing_";
    // create a single iteration
    {
        Series series(basename + "%06T." + extension, Access::CREATE);
        std::vector<float> buffer(20);
        std::iota(buffer.begin(), buffer.end(), 0.f);
        auto dataset = series.iterations[0].meshes["E"]["x"];
        dataset.resetDataset({Datatype::FLOAT, {20}});
        dataset.storeChunk(buffer, {0}, {20});
        series.flush();
    }
    // create some empty pseudo files
    // if the reader tries accessing them it's game over
    {
        for (size_t i = 1; i < 1000; i += 100)
        {
            std::string infix = std::to_string(i);
            std::string padding;
            for (size_t j = 0; j < 6 - infix.size(); ++j)
            {
                padding += "0";
            }
            infix = padding.append(infix);
            std::ofstream file;
            file.open(std::string(basename).append(infix).append(".").append(
                extension));
            file.close();
        }
    }
    {
        Series series(
            basename + "%06T." + extension,
            Access::READ_ONLY,
            "{\"defer_iteration_parsing\": true}");
        auto dataset =
            series.iterations[0].open().meshes["E"]["x"].loadChunk<float>(
                {0}, {20});
        series.flush();
        for (size_t i = 0; i < 20; ++i)
        {
            REQUIRE(
                std::abs(dataset.get()[i] - float(i)) <=
                std::numeric_limits<float>::epsilon());
        }
    }
    {
        Series series(
            basename + "%06T." + extension,
            Access::READ_WRITE,
            "{\"defer_iteration_parsing\": true}");
        auto dataset =
            series.iterations[0].open().meshes["E"]["x"].loadChunk<float>(
                {0}, {20});
        series.flush();
        for (size_t i = 0; i < 20; ++i)
        {
            REQUIRE(
                std::abs(dataset.get()[i] - float(i)) <=
                std::numeric_limits<float>::epsilon());
        }

        // create a new iteration
        std::vector<float> buffer(20);
        std::iota(buffer.begin(), buffer.end(), 0.f);
        auto writeDataset = series.iterations[1001].meshes["E"]["x"];
        writeDataset.resetDataset({Datatype::FLOAT, {20}});
        writeDataset.storeChunk(buffer, {0}, {20});
        series.flush();
    }
    {
        Series series(
            basename + "%06T." + extension,
            Access::READ_ONLY,
            "{\"defer_iteration_parsing\": true}");
        auto dataset =
            series.iterations[1001].open().meshes["E"]["x"].loadChunk<float>(
                {0}, {20});
        series.flush();
        for (size_t i = 0; i < 20; ++i)
        {
            REQUIRE(
                std::abs(dataset.get()[i] - float(i)) <=
                std::numeric_limits<float>::epsilon());
        }
    }

    basename += "groupbased";

    {
        Series series(basename + "." + extension, Access::CREATE);
        std::vector<float> buffer(20);
        std::iota(buffer.begin(), buffer.end(), 0.f);
        for (unsigned i = 0; i < 10; ++i)
        {
            auto dataset = series.iterations[i].meshes["E"]["x"];
            dataset.resetDataset({Datatype::FLOAT, {20}});
            dataset.storeChunk(buffer, {0}, {20});
        }
        series.iterations[9].setAttribute("time", "this is deliberately wrong");
        series.flush();
    }

    {
        Series series(
            basename + "." + extension,
            Access::READ_ONLY,
            "{\"defer_iteration_parsing\": true}");
        for (auto iteration : series.readIterations())
        {
            auto dataset =
                iteration.meshes["E"]["x"].loadChunk<float>({0}, {20});
            iteration.close();
            for (size_t i = 0; i < 20; ++i)
            {
                REQUIRE(
                    std::abs(dataset.get()[i] - float(i)) <=
                    std::numeric_limits<float>::epsilon());
            }
            if (iteration.iterationIndex == 8)
            {
                // reading up until iteration 8 should work
                break;
            }
        }
    }
}

TEST_CASE("deferred_parsing", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        deferred_parsing(t);
    }
}

#if openPMD_HAS_ADIOS_2_9
void chaotic_stream(std::string const &filename, bool variableBased)
{
    /*
     * We will write iterations in the following order.
     */
    std::vector<uint64_t> iterations{5, 9, 1, 3, 4, 6, 7, 8, 2, 0};
    std::string jsonConfig = R"(
{
    "adios2": {
        "use_group_table": true
    }
})";

    bool weirdOrderWhenReading{};

    Series series(filename, Access::CREATE, jsonConfig);
    /*
     * When using ADIOS2 steps, iterations are read not by logical order
     * (iteration index), but by order of writing.
     */
    weirdOrderWhenReading = series.backend() == "ADIOS2" &&
        series.iterationEncoding() != IterationEncoding::fileBased;
    if (variableBased)
    {
        if (series.backend() != "ADIOS2")
        {
            return;
        }
        series.setIterationEncoding(IterationEncoding::variableBased);
    }
    for (auto currentIteration : iterations)
    {
        auto dataset =
            series.writeIterations()[currentIteration]
                .meshes["iterationOrder"][MeshRecordComponent::SCALAR];
        dataset.resetDataset({determineDatatype<uint64_t>(), {10}});
        dataset.storeChunk(iterations, {0}, {10});
        // series.writeIterations()[ currentIteration ].close();
    }
    REQUIRE(series.operator bool());
    series.close();
    REQUIRE(!series.operator bool());

    /*
     * Random-access read mode would go by the openPMD group names instead
     * of the ADIOS2 steps.
     * Hence, the order would be ascending.
     */
    Series read(filename, Access::READ_LINEAR);
    size_t index = 0;
    for (const auto &iteration : read.readIterations())
    {
        if (weirdOrderWhenReading)
        {
            REQUIRE(iteration.iterationIndex == iterations[index]);
        }
        else
        {
            REQUIRE(iteration.iterationIndex == index);
        }
        ++index;
    }
    REQUIRE(index == iterations.size());
}

TEST_CASE("chaotic_stream", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        chaotic_stream("../samples/chaotic_stream_filebased_%T." + t, false);
        chaotic_stream("../samples/chaotic_stream." + t, false);
        chaotic_stream("../samples/chaotic_stream_vbased." + t, true);
    }
}
#endif // openPMD_HAS_ADIOS_2_9

#ifdef openPMD_USE_INVASIVE_TESTS
void unfinished_iteration_test(
    std::string const &ext,
    IterationEncoding encoding,
    std::string const &config = "{}",
    bool test_linear_access = true)
{
    std::cout << "\n\nTESTING " << ext << "\n\n" << std::endl;
    std::string file = std::string("../samples/unfinished_iteration") +
        (encoding == IterationEncoding::fileBased ? "_%T." : ".") + ext;
    {
        std::vector<int> data{0, 1, 2, 3, 4};
        Series write(file, Access::CREATE, config);
        auto it0 = write.writeIterations()[0];
        it0.meshes["E"]["x"].resetDataset({Datatype::INT, {5}});
        it0.meshes["E"]["x"].storeChunk(data, {0}, {5});
        auto it5 = write.writeIterations()[5];
        it5.meshes["E"]["x"].resetDataset({Datatype::INT, {5}});
        it5.meshes["E"]["x"].storeChunk(data, {0}, {5});
        ;
        /*
         * With enabled invasive tests, this attribute will let the Iteration
         * fail parsing.
         */
        it5.setAttribute("__openPMD_internal_fail", "asking for trouble");
        auto it10 = write.writeIterations()[10];
        Dataset ds(Datatype::INT, {10});
        it10.meshes["E"]["x"].resetDataset({Datatype::INT, {5}});
        it10.meshes["E"]["x"].storeChunk(data, {0}, {5});
        it10.setAttribute("__openPMD_internal_fail", "playing nice again");
        auto e_density = it10.meshes["e_density"][RecordComponent::SCALAR];
        auto electron_x = it10.particles["e"]["position"]["x"];
        auto electron_mass =
            it10.particles["e"]["mass"][RecordComponent::SCALAR];

        RecordComponent *resetThese[] = {
            &e_density, &electron_x, &electron_mass};
        for (RecordComponent *rc : resetThese)
        {
            rc->resetDataset(ds);
        }
    }
    auto tryReading = [&config, file, encoding](
                          Access access,
                          std::string const &additionalConfig = "{}") {
        {
            Series read(file, access, json::merge(config, additionalConfig));

            std::vector<decltype(Series::iterations)::key_type> iterations;
            std::cout << "Going to list iterations in " << file << ":"
                      << std::endl;
            for (auto iteration : read.readIterations())
            {
                std::cout << "Seeing iteration " << iteration.iterationIndex
                          << std::endl;
                iterations.push_back(iteration.iterationIndex);

                Parameter<Operation::READ_ATT> readAttribute;
                readAttribute.name = "this_does_definitely_not_exist";
                read.IOHandler()->enqueue(IOTask(&iteration, readAttribute));
                // enqueue a second time to check that the queue is cleared upon
                // exception
                read.IOHandler()->enqueue(IOTask(&iteration, readAttribute));

                REQUIRE_THROWS_AS(
                    read.IOHandler()->flush({FlushLevel::InternalFlush}),
                    error::ReadError);
                REQUIRE(read.IOHandler()->m_work.empty());
            }
            REQUIRE(
                (iterations ==
                 std::vector<decltype(Series::iterations)::key_type>{0, 10}));
        }

        if (encoding == IterationEncoding::fileBased &&
            access == Access::READ_ONLY)
        {
            Series read(file, access, json::merge(config, additionalConfig));
            if (additionalConfig == "{}")
            {
                // Eager parsing, defective iteration has already been removed
                REQUIRE(!read.iterations.contains(5));
                read.iterations[0].open();
                read.iterations[10].open();
            }
            else
            {
                REQUIRE_THROWS_AS(read.iterations[5].open(), error::ReadError);
                read.iterations[0].open();
                read.iterations[10].open();
            }
        }
    };

    if (test_linear_access)
    {
        tryReading(Access::READ_LINEAR);
        tryReading(Access::READ_LINEAR, R"({"defer_iteration_parsing": true})");
    }
    if (encoding != IterationEncoding::variableBased)
    {
        /*
         * In variable-based iteration encoding, READ_ONLY mode will make
         * iteration metadata leak into other iterations, causing iteration 0
         * to fail being parsed.
         * (See also the warning that occurs when trying to access a variable-
         * based Series in READ_ONLY mode)
         */
        tryReading(Access::READ_ONLY);
        tryReading(Access::READ_ONLY, R"({"defer_iteration_parsing": true})");
    }
}

TEST_CASE("unfinished_iteration_test", "[serial]")
{
#if openPMD_HAVE_ADIOS2
    unfinished_iteration_test(
        "bp",
        IterationEncoding::groupBased,
        R"({"backend": "adios2"})",
        /* test_linear_access = */ false);
#if openPMD_HAS_ADIOS_2_9
    unfinished_iteration_test(
        "bp5",
        IterationEncoding::variableBased,
        R"(
    {
      "backend": "adios2",
      "iteration_encoding": "variable_based",
      "adios2": {
        "use_group_table": true
      }
    }
    )");
#endif // openPMD_HAS_ADIOS_2_9
    unfinished_iteration_test(
        "bp", IterationEncoding::fileBased, R"({"backend": "adios2"})");
#endif
#if openPMD_HAVE_HDF5
    unfinished_iteration_test("h5", IterationEncoding::groupBased);
    unfinished_iteration_test("h5", IterationEncoding::fileBased);
#endif
    unfinished_iteration_test("json", IterationEncoding::groupBased);
    unfinished_iteration_test("json", IterationEncoding::fileBased);
}
#endif

TEST_CASE("late_setting_of_iterationencoding", "[serial]")
{
    {
        ::openPMD::Series series = ::openPMD::Series(
            "../samples/error.json", ::openPMD::Access::CREATE);
        series.iterations[10];
        REQUIRE_THROWS_WITH(
            series.setIterationEncoding(
                ::openPMD::IterationEncoding::fileBased),
            Catch::Equals("Wrong API usage: For fileBased formats the "
                          "iteration expansion pattern %T must "
                          "be included in the file name"));
        series.flush();
    }
    {
        ::openPMD::Series series = ::openPMD::Series(
            "../samples/asdf_%T.json", ::openPMD::Access::CREATE);
        series.iterations[10];
        series.setName("change_name_%T");
        series.flush();
    }
    {
        ::openPMD::Series series = ::openPMD::Series(
            "../samples/change_name_keep_filename_%T.json",
            ::openPMD::Access::CREATE);
        series.iterations[10];
        series.setName(
            "expansion_pattern_was_specified_previously_filename_will_stay");
        series.flush();
    }
    {
        ::openPMD::Series series = ::openPMD::Series(
            "../samples/asdf.json", ::openPMD::Access::CREATE);
        series.iterations[10];
        series.setName("change_name_and_encoding_%T");
        series.setIterationEncoding(IterationEncoding::fileBased);
        series.flush();
    }

    REQUIRE(auxiliary::file_exists("../samples/change_name_10.json"));
    REQUIRE(
        auxiliary::file_exists("../samples/change_name_keep_filename_10.json"));
    REQUIRE(
        auxiliary::file_exists("../samples/change_name_and_encoding_10.json"));
}

void varying_pattern(std::string const &file_ending)
{
    {
        std::string filename = "../samples/varying_pattern_%06T." + file_ending;
        ::openPMD::Series series =
            ::openPMD::Series(filename, ::openPMD::Access::CREATE);

        for (auto i : {0, 8000, 10000, 100000, 2000000})
        {
            auto it = series.iterations[i];
            it.setAttribute("my_step", i);
        }
        series.flush();
    }
    {
        std::string filename = "../samples/varying_pattern_%T." + file_ending;
        ::openPMD::Series series =
            ::openPMD::Series(filename, ::openPMD::Access::READ_ONLY);

        REQUIRE(series.iterations.size() == 5);
        for (auto const &[step, it] : series.iterations)
        {
            std::cout << "Iteration: " << step << "\n";
            REQUIRE(it.getAttribute("my_step").get<int>() == int(step));
        }

        helper::listSeries(series, true, std::cout);

        for (auto i : {0, 8000, 10000, 100000, 2000000})
        {
            auto it = series.iterations[i];
            REQUIRE(it.getAttribute("my_step").get<int>() == i);
        }
    }
}

TEST_CASE("varying_zero_pattern", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        varying_pattern(t);
    }
}

enum class ParseMode
{
    /*
     * Conventional workflow. Just parse the whole thing and yield iterations
     * in rising order.
     */
    NoSteps,
    /*
     * NOTE: This mode is only temporary until the topic-linear-read PR,
     * no longer necessary after that.
     * The Series is parsed ahead of time upon opening, but it has steps.
     * Parsing ahead of time is the conventional workflow to support
     * random-access.
     * Reading such a Series with the streaming API is only possible if all
     * steps are in ascending order, otherwise the openPMD-api has no way of
     * associating IO steps with interation indices.
     * Reading such a Series with the Streaming API will become possible with
     * the Linear read mode to be introduced by #1291.
     */
    AheadOfTimeWithoutSnapshot,
    /*
     * In Linear read mode, a Series is not parsed ahead of time, but
     * step-by-step, giving the openPMD-api a way to associate IO steps with
     * iterations. No snapshot attribute exists, so the fallback mode is chosen:
     * Iterations are returned in ascending order.
     * If an IO step returns an iteration whose index is lower than the
     * last one, it will be skipped.
     * This mode of parsing is not available for the BP4 engine without the
     * group table feature, since BP4 does not associate attributes with the
     * step in which they were created, making it impossible to separate parsing
     * into single steps.
     */
    LinearWithoutSnapshot,
    /*
     * Snapshot attribute exists and dictates the iteration index returned by
     * an IO step. Duplicate iterations will be skipped.
     */
    WithSnapshot
};

void append_mode(
    std::string const &filename,
    bool variableBased,
    ParseMode parseMode,
    std::string const &jsonConfig = "{}",
    bool test_read_linear = true)
{
    if (auxiliary::directory_exists("../samples/append"))
    {
        auxiliary::remove_directory("../samples/append");
    }
    std::vector<int> data(10, 999);
    auto writeSomeIterations = [&data](
                                   WriteIterations &&writeIterations,
                                   std::vector<uint64_t> const &indices) {
        for (auto index : indices)
        {
            auto it = writeIterations[index];
            auto dataset = it.meshes["E"]["x"];
            dataset.resetDataset({Datatype::INT, {10}});
            dataset.storeChunk(data, {0}, {10});
            // test that it works without closing too
            it.close();
        }
    };
    {
        Series write(filename, Access::APPEND, jsonConfig);
        if (variableBased)
        {
            if (write.backend() != "ADIOS2")
            {
                return;
            }
            write.setIterationEncoding(IterationEncoding::variableBased);
        }
        writeSomeIterations(
            write.writeIterations(), std::vector<uint64_t>{0, 1});
    }
    {
        Series write(filename, Access::APPEND, jsonConfig);
        if (variableBased)
        {
            write.setIterationEncoding(IterationEncoding::variableBased);
        }

        writeSomeIterations(
            write.writeIterations(), std::vector<uint64_t>{3, 2});
        write.flush();
    }
    {
        using namespace std::chrono_literals;
        /*
         * Put a little sleep here to trigger writing of a different /date
         * attribute. ADIOS2 v2.7 does not like that so this test ensures that
         * we deal with it.
         */
        std::this_thread::sleep_for(1s);
        Series write(filename, Access::APPEND, jsonConfig);
        if (variableBased)
        {
            write.setIterationEncoding(IterationEncoding::variableBased);
        }

        writeSomeIterations(
            write.writeIterations(), std::vector<uint64_t>{4, 3, 10});
        write.flush();
    }
    {
        Series write(filename, Access::APPEND, jsonConfig);
        if (variableBased)
        {
            write.setIterationEncoding(IterationEncoding::variableBased);
        }

        writeSomeIterations(
            write.writeIterations(), std::vector<uint64_t>{7, 1, 11});
        write.flush();
    }

    auto verifyIteration = [](auto &&it) {
        auto chunk = it.meshes["E"]["x"].template loadChunk<int>({0}, {10});
        it.seriesFlush();
        for (size_t i = 0; i < 10; ++i)
        {
            REQUIRE(chunk.get()[i] == 999);
        }
    };

    if (test_read_linear)
    {
        switch (parseMode)
        {
        case ParseMode::NoSteps: {
            Series read(filename, Access::READ_LINEAR);
            unsigned counter = 0;
            uint64_t iterationOrder[] = {0, 1, 2, 3, 4, 7, 10, 11};
            for (auto iteration : read.readIterations())
            {
                REQUIRE(iteration.iterationIndex == iterationOrder[counter]);
                verifyIteration(iteration);
                ++counter;
            }
            REQUIRE(counter == 8);
        }
        break;
        case ParseMode::LinearWithoutSnapshot: {
            Series read(filename, Access::READ_LINEAR);
            unsigned counter = 0;
            uint64_t iterationOrder[] = {0, 1, 3, 4, 10, 11};
            for (auto iteration : read.readIterations())
            {
                REQUIRE(iteration.iterationIndex == iterationOrder[counter]);
                verifyIteration(iteration);
                ++counter;
            }
            REQUIRE(counter == 6);
        }
        break;
        case ParseMode::WithSnapshot: {
            // in variable-based encodings, iterations are not parsed ahead of
            // time but as they go
            Series read(filename, Access::READ_LINEAR);
            unsigned counter = 0;
            uint64_t iterationOrder[] = {0, 1, 3, 2, 4, 10, 7, 11};
            for (auto iteration : read.readIterations())
            {
                REQUIRE(iteration.iterationIndex == iterationOrder[counter]);
                verifyIteration(iteration);
                ++counter;
            }
            REQUIRE(counter == 8);
            // listSeries will not see any iterations since they have already
            // been read
            helper::listSeries(read);
        }
        break;
        case ParseMode::AheadOfTimeWithoutSnapshot: {
            Series read(filename, Access::READ_LINEAR);
            unsigned counter = 0;
            uint64_t iterationOrder[] = {0, 1, 2, 3, 4, 7, 10, 11};
            /*
             * This one is a bit tricky:
             * The BP4 engine has no way of parsing a Series step-by-step in
             * ADIOS2 without group tables, since attributes are not
             * associated with the step in which they were created.
             * As a result, when readIterations() is called, the whole thing
             * is parsed immediately ahead-of-time.
             * We can then iterate through the iterations and access metadata,
             * but since the IO steps don't correspond with the order of
             * iterations returned (there is no way to figure out that order),
             * we cannot load data in here.
             * BP4 in ADIOS2 without group table only supports either of the
             * following: 1) A Series in which the iterations are present in
             * ascending order. 2) Or accessing the Series in READ_ONLY mode.
             */
            for (auto const &iteration : read.readIterations())
            {
                REQUIRE(iteration.iterationIndex == iterationOrder[counter]);
                ++counter;
            }
            REQUIRE(counter == 8);
            /*
             * Roadmap: for now, reading this should work by ignoring the last
             * duplicate iteration.
             * After merging https://github.com/openPMD/openPMD-api/pull/949, we
             * should see both instances when reading.
             * Final goal: Read only the last instance.
             */
            // listSeries will not see any iterations since they have already
            // been read
            helper::listSeries(read);
        }
        break;
        }
    }
    if (!variableBased)
    {
        Series read(filename, Access::READ_ONLY);
        REQUIRE(read.iterations.size() == 8);
        unsigned counter = 0;
        uint64_t iterationOrder[] = {0, 1, 2, 3, 4, 7, 10, 11};
        for (auto iteration : read.readIterations())
        {
            REQUIRE(iteration.iterationIndex == iterationOrder[counter]);
            verifyIteration(iteration);
            ++counter;
        }
        REQUIRE(counter == 8);
    }
    // AppendAfterSteps has a bug before that version
#if 100000000 * ADIOS2_VERSION_MAJOR + 1000000 * ADIOS2_VERSION_MINOR +        \
        10000 * ADIOS2_VERSION_PATCH + 100 * ADIOS2_VERSION_TWEAK >=           \
    208002700
    if (auxiliary::ends_with(filename, ".bp5"))
    {
        {
            Series write(
                filename,
                Access::APPEND,
                json::merge(
                    jsonConfig,
                    R"({"adios2":{"engine":{"parameters":{"AppendAfterSteps":-3}}}})"));
            if (variableBased)
            {
                write.setIterationEncoding(IterationEncoding::variableBased);
            }

            writeSomeIterations(
                write.writeIterations(), std::vector<uint64_t>{4, 5});
            write.flush();
        }
        if (test_read_linear)
        {
            Series read(filename, Access::READ_LINEAR);
            switch (parseMode)
            {
            case ParseMode::LinearWithoutSnapshot: {
                uint64_t iterationOrder[] = {0, 1, 3, 4, 10};
                unsigned counter = 0;
                for (auto iteration : read.readIterations())
                {
                    REQUIRE(
                        iteration.iterationIndex == iterationOrder[counter]);
                    verifyIteration(iteration);
                    ++counter;
                }
                REQUIRE(counter == 5);
            }
            break;
            case ParseMode::WithSnapshot: {
                // in variable-based encodings, iterations are not parsed ahead
                // of time but as they go
                unsigned counter = 0;
                uint64_t iterationOrder[] = {0, 1, 3, 2, 4, 10, 7, 5};
                for (auto iteration : read.readIterations())
                {
                    REQUIRE(
                        iteration.iterationIndex == iterationOrder[counter]);
                    verifyIteration(iteration);
                    ++counter;
                }
                REQUIRE(counter == 8);
            }
            break;
            default:
                throw std::runtime_error("Test configured wrong.");
                break;
            }
        }
        if (!variableBased)
        {
            Series read(filename, Access::READ_ONLY);
            uint64_t iterationOrder[] = {0, 1, 2, 3, 4, 5, 7, 10};
            unsigned counter = 0;
            for (auto const &iteration : read.readIterations())
            {
                REQUIRE(iteration.iterationIndex == iterationOrder[counter]);
                ++counter;
            }
            REQUIRE(counter == 8);
            // listSeries will not see any iterations since they have already
            // been read
            helper::listSeries(read);
        }
    }
#endif
}

TEST_CASE("append_mode", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        std::string jsonConfigOld = R"END(
{
    "adios2":
    {
        "use_group_table": false
    }
})END";
        std::string jsonConfigNew = R"END(
{
    "adios2":
    {
        "use_group_table": true
    }
})END";
        if (t == "bp" || t == "bp4" || t == "bp5")
        {
            append_mode(
                "../samples/append/append_groupbased." + t,
                false,
                ParseMode::LinearWithoutSnapshot,
                jsonConfigOld,
                /* test_read_linear = */ false);
#if openPMD_HAS_ADIOS_2_9
            append_mode(
                "../samples/append/append_groupbased." + t,
                false,
                ParseMode::WithSnapshot,
                jsonConfigNew);
            // This test config does not make sense
            // append_mode(
            //     "../samples/append/append_variablebased." + t,
            //     true,
            //     ParseMode::WithSnapshot,
            //     jsonConfigOld);
            append_mode(
                "../samples/append/append_variablebased." + t,
                true,
                ParseMode::WithSnapshot,
                jsonConfigNew);
#endif
        }
        else
        {
            append_mode(
                "../samples/append/append_groupbased." + t,
                false,
                ParseMode::NoSteps);
        }
    }
}

#if openPMD_HAS_ADIOS_2_9
void append_mode_filebased(std::string const &extension)
{
    std::string jsonConfig = R"END(
{
    "adios2":
    {
        "use_group_table": true
    }
})END";
    auto writeSomeIterations = [](WriteIterations &&writeIterations,
                                  std::vector<uint64_t> const &indices) {
        for (auto index : indices)
        {
            auto it = writeIterations[index];
            auto dataset = it.meshes["E"]["x"];
            dataset.resetDataset({Datatype::INT, {1}});
            dataset.makeConstant<int>(0);
            // test that it works without closing too
            it.close();
        }
    };
    if (auxiliary::directory_exists("../samples/append"))
    {
        auxiliary::remove_directory("../samples/append");
    }
    {
        Series write(
            "../samples/append/append_%06T." + extension,
            Access::APPEND,
            jsonConfig);
        writeSomeIterations(
            write.writeIterations(), std::vector<uint64_t>{0, 1});
    }
    {
        Series write(
            "../samples/append/append_%T." + extension,
            Access::APPEND,
            jsonConfig);
        writeSomeIterations(
            write.writeIterations(), std::vector<uint64_t>{4, 5});
        write.flush();
    }
    {
        Series write(
            "../samples/append/append_%T." + extension,
            Access::APPEND,
            jsonConfig);
        writeSomeIterations(
            write.writeIterations(), std::vector<uint64_t>{2, 3});
        write.flush();
    }
    {
        Series write(
            "../samples/append/append_%T." + extension,
            Access::APPEND,
            jsonConfig);
        // overwrite a previous iteration
        writeSomeIterations(
            write.writeIterations(), std::vector<uint64_t>{4, 123});
        write.flush();
    }
    {
        Series read(
            "../samples/append/append_%T." + extension, Access::READ_ONLY);
        REQUIRE(read.iterations.size() == 7);
    }
}

TEST_CASE("append_mode_filebased", "[serial]")
{
    for (auto const &t : testedFileExtensions())
    {
        append_mode_filebased(t);
    }
}
#endif // openPMD_HAS_ADIOS_2_8

void groupbased_read_write(std::string const &ext)
{
    int data = 0;
    Dataset ds(Datatype::INT, {1});
    std::string filename = "../samples/groupbased_read_write." + ext;

    {
        Series write(filename, Access::CREATE);
        auto E_x = write.iterations[0].meshes["E"]["x"];
        auto E_y = write.iterations[0].meshes["E"]["y"];
        E_x.resetDataset(ds);
        E_y.resetDataset(ds);
        E_x.storeChunkRaw(&data, {0}, {1});
        E_y.storeChunkRaw(&data, {0}, {1});

        E_x.setAttribute("updated_in_run", 0);
        E_y.setAttribute("updated_in_run", 0);
    }

    {
        Series write(filename, Access::READ_WRITE);
        // create a new iteration
        auto E_x = write.iterations[1].meshes["E"]["x"];
        E_x.resetDataset(ds);

        // overwrite old dataset
        auto E_y = write.iterations[0].meshes["E"]["y"];

        data = 1;

        E_x.storeChunkRaw(&data, {0}, {1});
        E_y.storeChunkRaw(&data, {0}, {1});

        E_x.setAttribute("updated_in_run", 1);
        E_y.setAttribute("updated_in_run", 1);
    }

    {
        Series read(filename, Access::READ_ONLY);
        auto E_x_0_fromRun0 = read.iterations[0].meshes["E"]["x"];
        auto E_x_1_fromRun1 = read.iterations[1].meshes["E"]["x"];
        auto E_y_0_fromRun1 = read.iterations[0].meshes["E"]["y"];

        REQUIRE(E_x_0_fromRun0.getAttribute("updated_in_run").get<int>() == 0);
        REQUIRE(E_x_1_fromRun1.getAttribute("updated_in_run").get<int>() == 1);
        REQUIRE(E_y_0_fromRun1.getAttribute("updated_in_run").get<int>() == 1);

        auto chunk_E_x_0_fromRun0 = E_x_0_fromRun0.loadChunk<int>({0}, {1});
        auto chunk_E_x_1_fromRun1 = E_x_1_fromRun1.loadChunk<int>({0}, {1});
        auto chunk_E_y_0_fromRun1 = E_y_0_fromRun1.loadChunk<int>({0}, {1});

        read.flush();

        REQUIRE(*chunk_E_x_0_fromRun0 == 0);
        REQUIRE(*chunk_E_x_1_fromRun1 == 1);
        REQUIRE(*chunk_E_y_0_fromRun1 == 1);
    }

    // check that truncation works correctly
    {
        Series write(filename, Access::CREATE);
        // create a new iteration
        auto E_x = write.iterations[2].meshes["E"]["x"];
        E_x.resetDataset(ds);

        data = 2;

        E_x.storeChunkRaw(&data, {0}, {1});
        E_x.setAttribute("updated_in_run", 2);
    }

    {
        Series read(filename, Access::READ_ONLY);
        REQUIRE(read.iterations.size() == 1);
        REQUIRE(read.iterations.count(2) == 1);
    }
}

TEST_CASE("groupbased_read_write", "[serial]")
{
    constexpr char const *supportsGroupbasedRW[] = {"h5", "json"};
    for (auto const &t : testedFileExtensions())
    {
        for (auto const supported : supportsGroupbasedRW)
        {
            if (t == supported)
            {
                groupbased_read_write(t);
                break;
            }
        }
    }
    if (auto extensions = getFileExtensions();
        std::find(extensions.begin(), extensions.end(), "toml") !=
        extensions.end())
    { /*
       * TOML backend is not generally tested for performance reasons, opt in to
       * testing it here.
       */
        groupbased_read_write("toml");
    }
}

void joined_dim(std::string const &ext)
{
    using type = float;
    using patchType = uint64_t;
    constexpr size_t patches_per_rank = 5;
    constexpr size_t length_of_patch = 10;

    {
        Series s("../samples/joinedDimParallel." + ext, Access::CREATE);
        std::vector<UniquePtrWithLambda<type>> writeFrom(patches_per_rank);

        auto it = s.writeIterations()[100];

        Dataset numParticlesDS(
            determineDatatype<patchType>(), {Dataset::JOINED_DIMENSION});
        auto numParticles =
            it.particles["e"]
                .particlePatches["numParticles"][RecordComponent::SCALAR];
        auto numParticlesOffset =
            it.particles["e"]
                .particlePatches["numParticlesOffset"][RecordComponent::SCALAR];
        numParticles.resetDataset(numParticlesDS);
        numParticlesOffset.resetDataset(numParticlesDS);

        auto patchOffset = it.particles["e"].particlePatches["offset"]["x"];
        auto patchExtent = it.particles["e"].particlePatches["extent"]["x"];
        Dataset particlePatchesDS(
            determineDatatype<float>(), {Dataset::JOINED_DIMENSION});
        patchOffset.resetDataset(particlePatchesDS);
        patchExtent.resetDataset(particlePatchesDS);

        for (size_t i = 0; i < 5; ++i)
        {
            writeFrom[i] = UniquePtrWithLambda<type>(
                new type[length_of_patch],
                [](auto const *ptr) { delete[] ptr; });
            std::iota(
                writeFrom[i].get(),
                writeFrom[i].get() + 10,
                length_of_patch * i);
            patchOffset.store<type>(length_of_patch * i);
        }

        auto epx = it.particles["e"]["position"]["x"];
        Dataset ds(determineDatatype<type>(), {Dataset::JOINED_DIMENSION});
        epx.resetDataset(ds);

        size_t counter = 0;
        for (auto &chunk : writeFrom)
        {
            epx.storeChunk(std::move(chunk), {}, {length_of_patch});
            numParticles.store<patchType>(length_of_patch);
            /*
             * For the sake of the test case, we know that the
             * numParticlesOffset has this value. In general, the purpose of the
             * joined array is that we don't need to know these values, so the
             * specification of particle patches is somewhat difficult.
             */
            numParticlesOffset.store<patchType>(counter++ * length_of_patch);
            patchExtent.store<type>(10);
        }
        writeFrom.clear();
        it.close();
        s.close();
    }

    {
        Series s("../samples/joinedDimParallel." + ext, Access::READ_ONLY);
        auto it = s.iterations[100];
        auto e = it.particles["e"];

        auto particleData = e["position"]["x"].loadChunk<type>();
        auto numParticles =
            e.particlePatches["numParticles"][RecordComponent::SCALAR]
                .load<patchType>();
        auto numParticlesOffset =
            e.particlePatches["numParticlesOffset"][RecordComponent::SCALAR]
                .load<patchType>();
        auto patchOffset = e.particlePatches["offset"]["x"].load<type>();
        auto patchExtent = e.particlePatches["extent"]["x"].load<type>();

        it.close();

        // check validity of particle patches
        auto numPatches =
            e.particlePatches["numParticlesOffset"][RecordComponent::SCALAR]
                .getExtent()[0];
        REQUIRE(
            e.particlePatches["numParticles"][RecordComponent::SCALAR]
                .getExtent()[0] == numPatches);
        for (size_t i = 0; i < numPatches; ++i)
        {
            for (size_t j = 0; j < numParticles.get()[i]; ++j)
            {
                REQUIRE(
                    patchOffset.get()[i] <=
                    particleData.get()[numParticlesOffset.get()[i] + j]);
                REQUIRE(
                    particleData.get()[numParticlesOffset.get()[i] + j] <
                    patchOffset.get()[i] + patchExtent.get()[i]);
            }
        }

        /*
         * Check that:
         * 1. Joined array joins writes from lower ranks before higher ranks
         * 2. Joined array joins early writes before later writes from the same
         *    rank
         */
        for (size_t i = 0; i < length_of_patch * patches_per_rank; ++i)
        {
            REQUIRE(float(i) == particleData.get()[i]);
        }
        for (size_t i = 0; i < patches_per_rank; ++i)
        {
            REQUIRE(length_of_patch * i == numParticlesOffset.get()[i]);
            REQUIRE(type(length_of_patch * i) == patchOffset.get()[i]);
        }
    }
}

TEST_CASE("joined_dim", "[serial]")
{
#if 100000000 * ADIOS2_VERSION_MAJOR + 1000000 * ADIOS2_VERSION_MINOR +        \
        10000 * ADIOS2_VERSION_PATCH + 100 * ADIOS2_VERSION_TWEAK >=           \
    209000000
    constexpr char const *supportsJoinedDims[] = {"bp", "bp4", "bp5"};
#else
    // no zero-size arrays
    std::vector<char const *> supportsJoinedDims;
#endif
    for (auto const &t : testedFileExtensions())
    {
        for (auto const supported : supportsJoinedDims)
        {
            if (t == supported)
            {
                joined_dim(t);
                break;
            }
        }
    }
}
