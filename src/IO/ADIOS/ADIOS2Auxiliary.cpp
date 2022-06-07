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
#include "openPMD/IO/ADIOS/ADIOS2Auxiliary.hpp"

#include <iostream>

namespace openPMD
{
namespace detail
{
    template <typename T>
    std::string ToDatatypeHelper<T>::type()
    {
        return adios2::GetType<T>();
    }

    template <typename T>
    std::string ToDatatypeHelper<std::vector<T> >::type()
    {
        return

            adios2::GetType<T>();
    }

    template <typename T, size_t n>
    std::string ToDatatypeHelper<std::array<T, n> >::type()
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

    Datatype fromADIOS2Type(std::string const &dt, bool verbose)
    {
        static std::map<std::string, Datatype> map{
            {"string", Datatype::STRING},
            {"char", Datatype::CHAR},
            {"signed char", Datatype::CHAR},
            {"unsigned char", Datatype::UCHAR},
            {"short", Datatype::SHORT},
            {"unsigned short", Datatype::USHORT},
            {"int", Datatype::INT},
            {"unsigned int", Datatype::UINT},
            {"long int", Datatype::LONG},
            {"unsigned long int", Datatype::ULONG},
            {"long long int", Datatype::LONGLONG},
            {"unsigned long long int", Datatype::ULONGLONG},
            {"float", Datatype::FLOAT},
            {"double", Datatype::DOUBLE},
            {"long double", Datatype::LONG_DOUBLE},
            {"float complex", Datatype::CFLOAT},
            {"double complex", Datatype::CDOUBLE},
            {"long double complex",
             Datatype::CLONG_DOUBLE}, // does not exist as of 2.7.0 but might
                                      // come later
            {"uint8_t", Datatype::UCHAR},
            {"int8_t", Datatype::CHAR},
            {"uint16_t", determineDatatype<uint16_t>()},
            {"int16_t", determineDatatype<int16_t>()},
            {"uint32_t", determineDatatype<uint32_t>()},
            {"int32_t", determineDatatype<int32_t>()},
            {"uint64_t", determineDatatype<uint64_t>()},
            {"int64_t", determineDatatype<int64_t>()}};
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
            return Datatype::UNDEFINED;
        }
    }

    template <typename T>
    Extent AttributeInfo::operator()(
        adios2::IO &IO,
        std::string const &attributeName,
        VariableOrAttribute voa)
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
    Extent AttributeInfo::operator()(Params &&...)
    {
        return {0};
    }

    Datatype attributeInfo(
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
            return Datatype::UNDEFINED;
        }
        else
        {
            static AttributeInfo ai;
            Datatype basicType = fromADIOS2Type(type);
            Extent shape = switchAdios2AttributeType(
                basicType, ai, IO, attributeName, voa);

            switch (voa)
            {
            case VariableOrAttribute::Attribute: {
                auto size = shape[0];
                Datatype openPmdType = size == 1 ? basicType
                    : size == 7 && basicType == Datatype::DOUBLE
                    ? Datatype::ARR_DBL_7
                    : toVectorType(basicType);
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
                    Datatype openPmdType =
                        size == 7 && basicType == Datatype::DOUBLE
                        ? Datatype::ARR_DBL_7
                        : toVectorType(basicType);
                    return openPmdType;
                }
                else if (
                    shape.size() == 2 &&
                    (basicType == Datatype::CHAR ||
                     basicType == Datatype::UCHAR))
                {
                    return Datatype::VEC_STRING;
                }
                else
                {
                    std::stringstream errorMsg;
                    errorMsg << "[ADIOS2] Unexpected shape for "
                             << attributeName << ": [";
                    for (auto const ext : shape)
                    {
                        errorMsg << std::to_string(ext) << ", ";
                    }
                    errorMsg << "] of type " << datatypeToString(basicType);
                    throw std::runtime_error(errorMsg.str());
                }
            }
            }
            throw std::runtime_error("Unreachable!");
        }
    }
} // namespace detail
} // namespace openPMD
#endif
