#pragma once

#include "openPMD/auxiliary/TypeTraits.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace openPMD
{
namespace detail
{
    template <typename DT, typename T>
    constexpr DT determineDatatypeGeneric()
    {
        if constexpr (std::is_same_v<DT, Datatype>)
        {
            return determineDatatype<T>();
        }
        else
        {
            return DT{}; // @todo compile time error
        }
    }

    template <typename DT, typename T>
    struct BasicDatatypeHelper
    {
        constexpr static DT m_dt = determineDatatypeGeneric<DT, T>();
    };

    template <typename DT, typename T>
    struct BasicDatatypeHelper<DT, std::vector<T>>
    {
        constexpr static DT m_dt = BasicDatatypeHelper<DT, T>{}.m_dt;
    };

    template <typename DT, typename T, std::size_t n>
    struct BasicDatatypeHelper<DT, std::array<T, n>>
    {
        constexpr static DT m_dt = BasicDatatypeHelper<DT, T>{}.m_dt;
    };

    template <typename DT>
    struct BasicDatatype
    {
        template <typename T>
        constexpr static DT call()
        {
            constexpr auto res = BasicDatatypeHelper<DT, T>::m_dt;
            return res;
        }

        constexpr static char const *errorMsg =
            "basicDatatype: received unknown datatype.";
    };

    template <typename DT>
    struct ToVectorType
    {
        template <typename T>
        constexpr static DT call()
        {
            if constexpr (auxiliary::IsVector_v<T>)
            {
                return determineDatatypeGeneric<DT, T>();
            }
            else if constexpr (auxiliary::IsArray_v<T>)
            {
                return determineDatatypeGeneric<
                    DT,
                    std::vector<typename T::value_type>>();
            }
            else
            {
                return determineDatatypeGeneric<DT, std::vector<T>>();
            }
        }

        constexpr static char const *errorMsg =
            "toVectorType: received unknown datatype.";
    };
} // namespace detail
} // namespace openPMD
