#include "openPMD/UnitDimension.hpp"
#include <algorithm>
#include <iterator>

namespace openPMD
{
namespace auxiliary
{
    void fromMapOfUnitDimension(
        double *cursor, std::map<UnitDimension, double> const &udim)
    {
        for (auto [unit, exponent] : udim)
        {
            cursor[static_cast<uint8_t>(unit)] = exponent;
        }
    }
} // namespace auxiliary

namespace unit_representations
{
    auto asArray(AsMap const &udim) -> AsArray
    {
        AsArray res;
        auxiliary::fromMapOfUnitDimension(res.data(), udim);
        return res;
    }
    auto asMap(AsArray const &array) -> AsMap
    {
        AsMap udim;
        for (size_t i = 0; i < array.size(); ++i)
        {
            if (array[i] != 0)
            {
                udim[static_cast<UnitDimension>(i)] = array[i];
            }
        }
        return udim;
    }

    auto asArrays(AsMaps const &vec) -> AsArrays
    {
        AsArrays res;
        std::transform(
            vec.begin(),
            vec.end(),
            std::back_inserter(res),
            [](auto const &map) { return asArray(map); });
        return res;
    }
    auto asMaps(AsArrays const &vec) -> AsMaps
    {
        AsMaps res;
        std::transform(
            vec.begin(),
            vec.end(),
            std::back_inserter(res),
            [](auto const &array) { return asMap(array); });
        return res;
    }
} // namespace unit_representations
} // namespace openPMD
