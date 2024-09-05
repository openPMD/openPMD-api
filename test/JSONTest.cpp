#include "openPMD/auxiliary/JSON.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"
#include "openPMD/openPMD.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
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
        REQUIRE(json::merge(leftJson, rightToml) == resJson);
        REQUIRE(sort_lines(json::merge(leftToml, rightJson)) == resToml);
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
