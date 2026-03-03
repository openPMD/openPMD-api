
#include "openPMD/backend/scientific_defaults/ScientificDefaults_auxiliary.hpp"

#include "openPMD/UnitDimension.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Variant_internal.hpp"

#include <iostream>
#include <type_traits>
#include <utility>

namespace openPMD::auxiliary
{

template <typename T>
auto write_val_to_stderr(T const &val) -> std::ostream &
{
    if constexpr (auxiliary::IsVector_v<T> || auxiliary::IsArray_v<T>)
    {
        auxiliary::write_vec_to_stream(std::cerr, val);
    }
    else if constexpr (std::is_same_v<T, unit_representations::AsMap>)
    {
        std::cerr << "Unit_Map";
    }
    else
    {
        std::cerr << val;
    }
    return std::cerr;
}
auto write_to_stderr(Attribute const &a) -> std::ostream &
{
    std::visit(
        [](auto const &val) { write_val_to_stderr(val); },
        a.getVariant<attribute_types>());
    return std::cerr;
}

auto createDefaultAxisLabels(uint64_t dimensionality)
    -> std::vector<std::string>
{
    switch (dimensionality)
    {
    case 0:
    case 1:
        return {"x"};
    case 2:
        return {"x", "y"};
    case 3:
        return {"x", "y", "z"};
    default:
        if (dimensionality < 100)
        {
            // x1, x2, x3, x4, ...
            std::vector<std::string> res;
            res.reserve(dimensionality);
            for (uint64_t i = 0; i < dimensionality; ++i)
            {
                res.emplace_back("x" + std::to_string(i));
            }
            return res;
        }
        else
        {
            std::string msg = "Please verify dimensionality. Was inferred as '";
            msg += std::to_string(dimensionality);
            msg += "'. Seems a bit much.";
            return {std::move(msg)};
        }
    }
    return std::vector<std::string>{"x", "y", "z"};
}

// Helper function to create default vector based on dimensionality
auto createDefaultVector(uint64_t dimensionality, double defaultValue)
    -> std::vector<double>
{
    if (dimensionality < 100)
    {
        return std::vector<double>(dimensionality, defaultValue);
    }
    else
    {
        return std::vector<double>{defaultValue};
    }
}

} // namespace openPMD::auxiliary
