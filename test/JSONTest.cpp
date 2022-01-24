#include "openPMD/auxiliary/JSON.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"

#include <catch2/catch.hpp>

#include <variant>


using namespace openPMD;

TEST_CASE( "json_parsing", "[auxiliary]" )
{
    std::string wrongValue = R"END(
{
  "ADIOS2": {
    "duplicate key": 1243,
    "DUPLICATE KEY": 234
  }
})END";
    REQUIRE_THROWS_WITH(
        json::parseOptions( wrongValue, false ),
        error::BackendConfigSchema(
            { "adios2", "duplicate key" }, "JSON config: duplicate keys." )
            .what() );
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
        json::parseOptions( same1, false ).config.dump() ==
        json::parseOptions( same2, false ).config.dump() );
    // Only keys should be transformed to lower case, values must stay the same
    REQUIRE(
        json::parseOptions( same1, false ).config.dump() !=
        json::parseOptions( different, false ).config.dump() );

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
    nlohmann::json jsonUpper = nlohmann::json::parse( upper );
    nlohmann::json jsonLower = nlohmann::json::parse( lower );
    REQUIRE( jsonUpper.dump() != jsonLower.dump() );
    json::lowerCase( jsonUpper );
    REQUIRE( jsonUpper.dump() == jsonLower.dump() );
}

TEST_CASE( "json_merging", "auxiliary" )
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
        json::merge( defaultVal, overwrite ) ==
        json::parseOptions( expect, false ).config.dump() );
}
