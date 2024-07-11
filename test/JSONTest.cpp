#include "openPMD/auxiliary/JSON.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"
#include "openPMD/helper/list_series.hpp"
#include "openPMD/openPMD.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

using namespace openPMD;

TEST_CASE("json_parsing", "[auxiliary]")
{
    std::string wrongValue = R"END(
{
  "ADIOS2": {
    "duplicate key": 1243,
    "DUPLICATE KEY": 234
  }
})END";
    REQUIRE_THROWS_WITH(
        json::parseOptions(wrongValue, false),
        error::BackendConfigSchema(
            {"adios2", "duplicate key"}, "JSON config: duplicate keys.")
            .what());
    std::string same1 = R"(
{
  "ADIOS2": {
    "type": "nullcore",
    "engine": {
      "type": "bp4",
      "usesteps": true
    }
  }
})";
    std::string same2 = R"(
{
  "adios2": {
    "type": "nullcore",
    "ENGINE": {
      "type": "bp4",
      "usesteps": true
    }
  }
})";
    std::string different = R"(
{
  "adios2": {
    "type": "NULLCORE",
    "ENGINE": {
      "type": "bp4",
      "usesteps": true
    }
  }
})";
    REQUIRE(
        json::parseOptions(same1, false).config.dump() ==
        json::parseOptions(same2, false).config.dump());
    // Only keys should be transformed to lower case, values must stay the same
    REQUIRE(
        json::parseOptions(same1, false).config.dump() !=
        json::parseOptions(different, false).config.dump());

    // Keys forwarded to ADIOS2 should remain untouched
    std::string upper = R"END(
{
  "ADIOS2": {
    "ENGINE": {
      "TYPE": "BP3",
      "UNUSED": "PARAMETER",
      "PARAMETERS": {
        "BUFFERGROWTHFACTOR": "2.0",
        "PROFILE": "ON"
      }
    },
    "UNUSED": "AS WELL",
    "DATASET": {
      "OPERATORS": [
        {
          "TYPE": "BLOSC",
          "PARAMETERS": {
              "CLEVEL": "1",
              "DOSHUFFLE": "BLOSC_BITSHUFFLE"
          }
        }
      ]
    }
  }
}
)END";
    std::string lower = R"END(
{
  "adios2": {
    "engine": {
      "type": "BP3",
      "unused": "PARAMETER",
      "parameters": {
        "BUFFERGROWTHFACTOR": "2.0",
        "PROFILE": "ON"
      }
    },
    "unused": "AS WELL",
    "dataset": {
      "operators": [
        {
          "type": "BLOSC",
          "parameters": {
              "CLEVEL": "1",
              "DOSHUFFLE": "BLOSC_BITSHUFFLE"
          }
        }
      ]
    }
  }
}
)END";
    nlohmann::json jsonUpper = nlohmann::json::parse(upper);
    nlohmann::json jsonLower = nlohmann::json::parse(lower);
    REQUIRE(jsonUpper.dump() != jsonLower.dump());
    json::lowerCase(jsonUpper);
    REQUIRE(jsonUpper.dump() == jsonLower.dump());
}

#if !__NVCOMPILER // see https://github.com/ToruNiina/toml11/issues/205
TEST_CASE("json_merging", "auxiliary")
{
    std::string defaultVal = R"END(
{
  "mergeRecursively": {
    "changed": 43,
    "unchanged": true,
    "delete_me": "adsf"
  },
  "dontmergearrays": [
    1,
    2,
    3,
    4,
    5
  ],
  "delete_me": [345,2345,36]
}
)END";

    std::string overwrite = R"END(
{
  "mergeRecursively": {
    "changed": "new value",
    "newValue": "44",
    "delete_me": null
  },
  "dontmergearrays": [
    5,
    6,
    7
  ],
  "delete_me": null
}
)END";

    std::string expect = R"END(
{
  "mergeRecursively": {
    "changed": "new value",
    "unchanged": true,
    "newValue": "44"
  },
  "dontmergearrays": [
    5,
    6,
    7
  ]
})END";
    REQUIRE(
        json::merge(defaultVal, overwrite) ==
        json::parseOptions(expect, false).config.dump());

    {
        // The TOML library doesn't guarantee a specific order of output
        // so we need to sort lines to compare with expected results
        auto sort_lines = [](std::string const &s) -> std::vector<std::string> {
            std::vector<std::string> v;
            std::istringstream sstream(s);
            for (std::string line; std::getline(sstream, line);
                 line = std::string())
            {
                v.push_back(std::move(line));
            }
            std::sort(v.begin(), v.end());
            return v;
        };
        std::string leftJson = R"({"left": "val"})";
        std::string rightJson = R"({"right": "val"})";
        std::string leftToml = R"(left = "val")";
        std::string rightToml = R"(right = "val")";

        std::string resJson =
            nlohmann::json::parse(R"({"left": "val", "right": "val"})").dump();
        std::vector<std::string> resToml = [&sort_lines]() {
            constexpr char const *raw = R"(
left = "val"
right = "val"
        )";
            std::istringstream istream(
                raw, std::ios_base::binary | std::ios_base::in);
            toml::value tomlVal = toml::parse(istream);
            std::stringstream sstream;
            sstream << toml::format(tomlVal);
            return sort_lines(sstream.str());
        }();

        REQUIRE(json::merge(leftJson, rightJson) == resJson);
        REQUIRE(sort_lines(json::merge(leftJson, rightToml)) == resToml);
        REQUIRE(json::merge(leftToml, rightJson) == resJson);
        REQUIRE(sort_lines(json::merge(leftToml, rightToml)) == resToml);
    }
}
#endif

/*
 * This tests two things about the /data/snapshot attribute:
 *
 * 1) Reading a variable-based series without the snapshot attribute should be
 *    possible by assuming a default /data/snapshot = 0.
 * 2) The snapshot attribute might be a vector of iterations. The Read API
 *    should then return the same iteration multiple times, with different
 *    indices.
 *
 * Such files are currently not created by the openPMD-api (the API currently
 * supports creating a variable-based series with a scalar snapshot attribute).
 * But the standard will allow both options above, so reading should at least
 * be possible.
 * This test creates a variable-based JSON series and then uses the nlohmann
 * json library to modifiy the resulting series for testing purposes.
 */
TEST_CASE("variableBasedModifiedSnapshot", "[auxiliary]")
{
    constexpr auto file = "../samples/variableBasedModifiedSnapshot.json";
    {
        Series writeSeries(file, Access::CREATE);
        writeSeries.setIterationEncoding(IterationEncoding::variableBased);
        REQUIRE(
            writeSeries.iterationEncoding() ==
            IterationEncoding::variableBased);
        auto iterations = writeSeries.writeIterations();
        auto iteration = iterations[10];
        auto E_z = iteration.meshes["E"]["x"];
        E_z.resetDataset({Datatype::INT, {1}});
        E_z.makeConstant(72);

        iteration.close();
    }

    {
        nlohmann::json series;
        {
            std::fstream fstream;
            fstream.open(file, std::ios_base::in);
            fstream >> series;
        }
        series["data"]["attributes"].erase("snapshot");
        {
            std::fstream fstream;
            fstream.open(file, std::ios_base::out | std::ios_base::trunc);
            fstream << series;
        }
    }

    /*
     * Need generic capture here since the compilers are being
     * annoying otherwise.
     */
    auto testRead = [&](std::vector<size_t> const &requiredIterations) {
        Series readSeries(file, Access::READ_ONLY);
        size_t counter = 0;
        for (auto const &iteration : readSeries.readIterations())
        {
            REQUIRE(iteration.iterationIndex == requiredIterations[counter++]);
        }
        REQUIRE(counter == requiredIterations.size());
    };
    testRead(std::vector<size_t>{0});

    {
        nlohmann::json series;
        {
            std::fstream fstream;
            fstream.open(file, std::ios_base::in);
            fstream >> series;
        }
        series["data"]["attributes"].erase("snapshot");
        auto &snapshot = series["data"]["attributes"]["snapshot"];
        snapshot["datatype"] = "VEC_ULONG";
        snapshot["value"] = std::vector{1, 2, 3, 4, 5};
        {
            std::fstream fstream;
            fstream.open(file, std::ios_base::out | std::ios_base::trunc);
            fstream << series;
        }
    }

    testRead(std::vector<size_t>{1, 2, 3, 4, 5});
}

namespace auxiliary
{
template <typename Callable, typename AccumulatorTuple>
void test_matrix_impl(Callable &callable, AccumulatorTuple tuple)
{
    std::apply(callable, std::move(tuple));
}

template <
    typename Callable,
    typename AccumulatorTuple,
    typename Arg,
    typename... Args>
void test_matrix_impl(
    Callable &callable,
    AccumulatorTuple tuple,
    std::vector<Arg> const &arg,
    std::vector<Args> const &...args)
{
    for (auto &val : arg)
    {
        test_matrix_impl(
            callable, std::tuple_cat(tuple, std::tuple<Arg>{val}), args...);
    }
}

template <typename Callable, typename... Args>
void test_matrix(Callable &&callable, std::vector<Args> const &...matrix)
{
    // no std::forward, callable is called multiple times, so the impl takes
    // a simple reference
    test_matrix_impl(callable, std::tuple<>(), matrix...);
}
} // namespace auxiliary

void json_short_modes(
    std::optional<bool> short_attributes,
    std::optional<bool> template_datasets,
    std::string const &standardVersion,
    std::string const &backend,
    unsigned int *name_counter)
{
    nlohmann::json config = nlohmann::json::object();
    if (short_attributes.has_value())
    {
        config[backend]["attribute"]["mode"] =
            *short_attributes ? "short" : "long";
    }
    if (template_datasets.has_value())
    {
        config[backend]["dataset"]["mode"] =
            *template_datasets ? "template" : "dataset";
    }
    std::string name = "../samples/json_short_modes/test" +
        std::to_string((*name_counter)++) + "." + backend;

    auto config_str = [&]() {
        std::stringstream res;
        res << config;
        return res.str();
    }();
    Series output(name, Access::CREATE, config_str);
    output.setOpenPMD(standardVersion);
    auto iteration = output.writeIterations()[0];

    auto default_configured = iteration.meshes["default_configured"];
    Dataset ds1(Datatype::INT, {5});
    default_configured.resetDataset(ds1);

    auto explicitly_templated = iteration.meshes["explicitly_templated"];
    Dataset ds2 = ds1;
    ds2.options =
        R"({")" + backend + R"(": {"dataset": {"mode": "template"}}})";
    explicitly_templated.resetDataset(ds2);

    auto explicitly_not_templated =
        iteration.meshes["explicitly_not_templated"];
    Dataset ds3 = ds1;
    ds3.options = R"({")" + backend + R"(": {"dataset": {"mode": "dataset"}}})";
    explicitly_not_templated.resetDataset(ds3);

    auto undefined_dataset = iteration.meshes["undefined_dataset"];
    Dataset d4(Datatype::UNDEFINED, {Dataset::UNDEFINED_EXTENT});
    undefined_dataset.resetDataset(d4);

    output.close();

    bool expect_template_datasets = template_datasets.value_or(false);
    bool expect_short_attributes = short_attributes.value_or(
        backend == "toml" || standardVersion == "2.0.0");

    nlohmann::json resulting_dataset = [&]() {
        std::fstream handle;
        handle.open(name, std::ios_base::binary | std::ios_base::in);
        if (backend == "json")
        {
            nlohmann::json res;
            handle >> res;
            return res;
        }
        else
        {
            auto toml_val = toml::parse(handle, name);
            return json::tomlToJson(toml_val);
        }
    }();

    if (expect_short_attributes)
    {
        REQUIRE(
            resulting_dataset["attributes"]["openPMD"] ==
            nlohmann::json::string_t{standardVersion});
        REQUIRE(
            resulting_dataset["__openPMD_internal"]["attribute_mode"] ==
            nlohmann::json::string_t{"short"});
    }
    else
    {
        REQUIRE(
            resulting_dataset["attributes"]["openPMD"] ==
            nlohmann::json{{"datatype", "STRING"}, {"value", standardVersion}});
        REQUIRE(
            resulting_dataset["__openPMD_internal"]["attribute_mode"] ==
            nlohmann::json::string_t{"long"});
    }

    auto verify_full_dataset = [&](nlohmann::json const &j) {
        REQUIRE(j["datatype"] == "INT");
        if (backend == "json")
        {
            nlohmann::json null;
            REQUIRE(
                j["data"] ==
                nlohmann::json::array_t{null, null, null, null, null});
        }
        else
        {
            REQUIRE(j["data"] == nlohmann::json::array_t{0, 0, 0, 0, 0});
        }
        // `data` key, `datatype` key, and `attributes` key
        REQUIRE(j.size() == 3);
    };
    auto verify_template_dataset = [](nlohmann::json const &j) {
        REQUIRE(j["datatype"] == "INT");
        REQUIRE(j["extent"] == nlohmann::json::array_t{5});
        // `extent` key, `datatype` key, and `attributes` key
        REQUIRE(j.size() == 3);
    };

    // Undefined datasets write neither `extent` nor `data` key, so they are
    // not distinguished between template and nontemplate mode.
    REQUIRE(
        resulting_dataset["data"]["0"]["meshes"]["undefined_dataset"]
                         ["datatype"] == nlohmann::json::string_t{"UNDEFINED"});
    REQUIRE(
        resulting_dataset["data"]["0"]["meshes"]["undefined_dataset"].size() ==
        2);
    if (expect_template_datasets)
    {
        REQUIRE(
            resulting_dataset["__openPMD_internal"]["dataset_mode"] ==
            nlohmann::json::string_t{"template"});
        verify_template_dataset(
            resulting_dataset["data"]["0"]["meshes"]["default_configured"]);
    }
    else
    {
        REQUIRE(
            resulting_dataset["__openPMD_internal"]["dataset_mode"] ==
            nlohmann::json::string_t{"dataset"});
        verify_full_dataset(
            resulting_dataset["data"]["0"]["meshes"]["default_configured"]);
    }
    verify_template_dataset(
        resulting_dataset["data"]["0"]["meshes"]["explicitly_templated"]);
    verify_full_dataset(
        resulting_dataset["data"]["0"]["meshes"]["explicitly_not_templated"]);

    Series read(name, Access::READ_ONLY);
    helper::listSeries(read);
}

TEST_CASE("json_short_modes")
{
    unsigned int name_counter = 0;
    ::auxiliary::test_matrix(
        &json_short_modes,
        std::vector<std::optional<bool>>{std::nullopt, true, false},
        std::vector<std::optional<bool>>{std::nullopt, true, false},
        std::vector<std::string>{getStandardDefault(), getStandardMaximum()},
        std::vector<std::string>{
            "json"
#if !__NVCOMPILER // see https://github.com/ToruNiina/toml11/issues/205
            ,
            "toml"
#endif
        },
        std::vector<unsigned int *>{&name_counter});
}
