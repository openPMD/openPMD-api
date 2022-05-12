/* Copyright 2017-2021 Franz Poeschel.
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "openPMD/config.hpp"
#if openPMD_HAVE_ADIOS2
#include "openPMD/Datatype.hpp"
#include "openPMD/DatatypeHelpers.hpp"
#include "openPMD/Datatype_internal.hpp"
#include "openPMD/IO/ADIOS/ADIOS2Auxiliary.hpp"

#include <iostream>

namespace openPMD::detail
{
namespace
{
    struct DoDetermineDatatype
    {
        using DT_enum = ADIOS2Datatype;

        template <typename T>
        static constexpr ADIOS2Datatype call()
        {
            return determineAdios2Datatype<T>();
        }
    };
} // namespace

template <typename T>
std::string ToDatatypeHelper<T>::type()
{
    return adios2::GetType<T>();
}

template <typename T>
std::string ToDatatypeHelper<std::vector<T>>::type()
{
    return

        adios2::GetType<T>();
}

template <typename T, size_t n>
std::string ToDatatypeHelper<std::array<T, n>>::type()
{
    return

        adios2::GetType<T>();
}

std::string ToDatatypeHelper<bool>::type()
{
    return ToDatatypeHelper<bool_representation>::type();
}

template <typename T>
std::string ToDatatype::operator()()
{
    return ToDatatypeHelper<T>::type();
}

template <int n>
std::string ToDatatype::operator()()
{
    return "";
}

ADIOS2Datatype fromADIOS2Type(std::string const &dt, bool verbose)
{
    static std::map<std::string, ADIOS2Datatype> map{
        {"string", ADIOS2Datatype::STRING},
        {"char", ADIOS2Datatype::CHAR},
        {"signed char", ADIOS2Datatype::SCHAR},
        {"unsigned char", ADIOS2Datatype::UCHAR},
        {"short", ADIOS2Datatype::SHORT},
        {"unsigned short", ADIOS2Datatype::USHORT},
        {"int", ADIOS2Datatype::INT},
        {"unsigned int", ADIOS2Datatype::UINT},
        {"long int", ADIOS2Datatype::LONG},
        {"unsigned long int", ADIOS2Datatype::ULONG},
        {"long long int", ADIOS2Datatype::LONGLONG},
        {"unsigned long long int", ADIOS2Datatype::ULONGLONG},
        {"float", ADIOS2Datatype::FLOAT},
        {"double", ADIOS2Datatype::DOUBLE},
        {"long double", ADIOS2Datatype::LONG_DOUBLE},
        {"float complex", ADIOS2Datatype::CFLOAT},
        {"double complex", ADIOS2Datatype::CDOUBLE},
        {"long double complex",
         ADIOS2Datatype::CLONG_DOUBLE}, // does not exist as of 2.7.0 but might
                                        // come later
        {"uint8_t", ADIOS2Datatype::UCHAR},
        {"int8_t", ADIOS2Datatype::SCHAR},
        {"uint16_t", determineAdios2Datatype<uint16_t>()},
        {"int16_t", determineAdios2Datatype<int16_t>()},
        {"uint32_t", determineAdios2Datatype<uint32_t>()},
        {"int32_t", determineAdios2Datatype<int32_t>()},
        {"uint64_t", determineAdios2Datatype<uint64_t>()},
        {"int64_t", determineAdios2Datatype<int64_t>()}};
    auto it = map.find(dt);
    if (it != map.end())
    {
        return it->second;
    }
    else
    {
        if (verbose)
        {
            std::cerr
                << "[ADIOS2] Warning: Encountered unknown ADIOS2 datatype,"
                   " defaulting to UNDEFINED."
                << std::endl;
        }
        return ADIOS2Datatype::UNDEFINED;
    }
}

template <typename T>
Extent AttributeInfo::call(
    adios2::IO &IO, std::string const &attributeName, VariableOrAttribute voa)
{
    switch (voa)
    {
    case VariableOrAttribute::Attribute: {
        auto attribute = IO.InquireAttribute<T>(attributeName);
        if (!attribute)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Attribute not present.");
        }
        return {attribute.Data().size()};
    }
    case VariableOrAttribute::Variable: {
        auto variable = IO.InquireVariable<T>(attributeName);
        if (!variable)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Variable not present.");
        }
        auto shape = variable.Shape();
        Extent res;
        res.reserve(shape.size());
        for (auto val : shape)
        {
            res.push_back(val);
        }
        return res;
    }
    default:
        throw std::runtime_error("[ADIOS2] Unreachable!");
    }
}

template <int n, typename... Params>
Extent AttributeInfo::call(Params &&...)
{
    return {0};
}

ADIOS2Datatype attributeInfo(
    adios2::IO &IO,
    std::string const &attributeName,
    bool verbose,
    VariableOrAttribute voa)
{
    std::string type;
    switch (voa)
    {
    case VariableOrAttribute::Attribute:
        type = IO.AttributeType(attributeName);
        break;
    case VariableOrAttribute::Variable:
        type = IO.VariableType(attributeName);
        break;
    }
    if (type.empty())
    {
        if (verbose)
        {
            std::cerr << "[ADIOS2] Warning: Attribute with name "
                      << attributeName << " has no type in backend."
                      << std::endl;
        }
        return ADIOS2Datatype::UNDEFINED;
    }
    else
    {
        ADIOS2Datatype basicType = fromADIOS2Type(type);
        Extent shape = switchAdios2AttributeType<AttributeInfo>(
            basicType, IO, attributeName, voa);

        switch (voa)
        {
        case VariableOrAttribute::Attribute: {
            auto size = shape[0];
            ADIOS2Datatype openPmdType = size == 1 ? basicType
                : size == 7 && basicType == ADIOS2Datatype::DOUBLE
                ? ADIOS2Datatype::ARR_DBL_7
                : switchAdios2Datatype<ToVectorType<DoDetermineDatatype>>(
                      basicType);
            return openPmdType;
        }
        case VariableOrAttribute::Variable: {
            if (shape.size() == 0 || (shape.size() == 1 && shape[0] == 1))
            {
                // global single value variable
                return basicType;
            }
            else if (shape.size() == 1)
            {
                auto size = shape[0];
                ADIOS2Datatype openPmdType =
                    size == 7 && basicType == ADIOS2Datatype::DOUBLE
                    ? ADIOS2Datatype::ARR_DBL_7
                    : switchAdios2Datatype<ToVectorType<DoDetermineDatatype>>(
                          basicType);
                return openPmdType;
            }
            else if (
                shape.size() == 2 &&
                (basicType == ADIOS2Datatype::CHAR ||
                 basicType == ADIOS2Datatype::SCHAR ||
                 basicType == ADIOS2Datatype::UCHAR))
            {
                return ADIOS2Datatype::VEC_STRING;
            }
            else
            {
                std::stringstream errorMsg;
                errorMsg << "[ADIOS2] Unexpected shape for " << attributeName
                         << ": [";
                for (auto const ext : shape)
                {
                    errorMsg << std::to_string(ext) << ", ";
                }
                errorMsg
                    << "] of type "; // @todo << datatypeToString(basicType);
                throw std::runtime_error(errorMsg.str());
            }
        }
        }
        throw std::runtime_error("Unreachable!");
    }
}

struct FromPublicType
{
    template <typename T>
    static constexpr ADIOS2Datatype call()
    {
        return determineAdios2Datatype<T>();
    }

    static constexpr char const *errorMsg = "FromPublicType";
};

ADIOS2Datatype fromPublicType(Datatype type)
{
    return switchType<FromPublicType>(type);
}

struct ToPublicType
{
    template <typename T>
    static constexpr Datatype call()
    {
        return determineDatatype<T>();
    }

    static constexpr char const *errorMsg = "ToPublicType";
};

Datatype toPublicType(ADIOS2Datatype type)
{
    return switchAdios2Datatype<ToPublicType>(type);
}
} // namespace openPMD::detail
#endif
