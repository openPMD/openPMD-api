#pragma once

#include "openPMD/auxiliary/TypeTraits.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace openPMD
{
namespace detail
{
    template <typename DoDetermineDatatype>
    struct BasicDatatype
    {
        using DT = typename DoDetermineDatatype::DT_enum;

        template <typename T>
        constexpr static DT call()
        {
            if constexpr (auxiliary::IsVector_v<T>)
            {
                return DoDetermineDatatype::template call<
                    typename T::value_type>();
            }
            else if constexpr (auxiliary::IsArray_v<T>)
            {
                return DoDetermineDatatype::template call<
                    typename T::value_type>();
            }
            else
            {
                return DoDetermineDatatype::template call<T>();
            }
        }

        constexpr static char const *errorMsg =
            "basicDatatype: received unknown datatype.";
    };

    template <typename DoDetermineDatatype>
    struct ToVectorType
    {
        using DT = typename DoDetermineDatatype::DT_enum;

        template <typename T>
        constexpr static DT call()
        {
            if constexpr (auxiliary::IsVector_v<T>)
            {
                return DoDetermineDatatype::template call<T>();
            }
            else if constexpr (auxiliary::IsArray_v<T>)
            {
                return DoDetermineDatatype::template call<
                    std::vector<typename T::value_type>>();
            }
            else
            {
                return DoDetermineDatatype::template call<std::vector<T>>();
            }
        }

        constexpr static char const *errorMsg =
            "toVectorType: received unknown datatype.";
    };
} // namespace detail
} // namespace openPMD
