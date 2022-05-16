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
#if defined(__INTEL_COMPILER)
/*
 * ICPC has trouble with if constexpr, thinking that return statements are
 * missing afterwards. Deactivate the warning.
 * Note that putting a statement here will not help to fix this since it will
 * then complain about unreachable code.
 * https://community.intel.com/t5/Intel-C-Compiler/quot-if-constexpr-quot-and-quot-missing-return-statement-quot-in/td-p/1154551
 */
#pragma warning(disable : 1011)
        }
#pragma warning(default : 1011)
#else
        }
#endif

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
#if defined(__INTEL_COMPILER)
/*
 * ICPC has trouble with if constexpr, thinking that return statements are
 * missing afterwards. Deactivate the warning.
 * Note that putting a statement here will not help to fix this since it will
 * then complain about unreachable code.
 * https://community.intel.com/t5/Intel-C-Compiler/quot-if-constexpr-quot-and-quot-missing-return-statement-quot-in/td-p/1154551
 */
#pragma warning(disable : 1011)
        }
#pragma warning(default : 1011)
#else
        }
#endif

        constexpr static char const *errorMsg =
            "toVectorType: received unknown datatype.";
    };
} // namespace detail
} // namespace openPMD
