#pragma once

#include "openPMD/LoadStoreChunk.hpp"

namespace openPMD
{
template <typename T>
auto ConfigureLoadStore::withSharedPtr(std::shared_ptr<T> data)
    -> shared_ptr_return_type<T>
{
    using T_decayed = std::remove_cv_t<std::remove_extent_t<T>>;
    constexpr auto dtype = determineDatatype<T_decayed>();
    if constexpr (std::is_const_v<T>)
    {
        return withSharedPtr_impl_const(data, dtype);
    }
    else
    {
        return withSharedPtr_impl_mut(data, dtype);
    }
}

template <typename T>
auto ConfigureLoadStore::withUniquePtr(UniquePtrWithLambda<T> data)
    -> unique_ptr_return_type<T>

{
    using T_decayed = std::remove_cv_t<std::remove_extent_t<T>>;
    constexpr auto dtype = determineDatatype<T_decayed>();
    if constexpr (std::is_const_v<T>)
    {
        return withUniquePtr_impl_const(
            std::move(data).template static_cast_<void const>(), dtype);
    }
    else
    {
        return withUniquePtr_impl_mut(
            std::move(data).template static_cast_<void>(), dtype);
    }
}

template <typename T, typename Del>
auto ConfigureLoadStore::withUniquePtr(std::unique_ptr<T, Del> data)
    -> unique_ptr_return_type<T>
{
    return withUniquePtr(UniquePtrWithLambda<T>(std::move(data)));
}

template <typename T>
auto ConfigureLoadStore::withRawPtr(T *data) -> shared_ptr_return_type<T>
{
    using T_decayed = std::remove_cv_t<std::remove_extent_t<T>>;
    constexpr auto dtype = determineDatatype<T_decayed>();
    if constexpr (std::is_const_v<T>)
    {
        return withRawPtr_impl_const(data, dtype);
    }
    else
    {
        return withRawPtr_impl_mut(data, dtype);
    }
}

template <typename T_ContiguousContainer>
auto ConfigureLoadStore::withContiguousContainer(T_ContiguousContainer &data)
    -> std::enable_if_t<
        auxiliary::IsContiguousContainer_v<T_ContiguousContainer>,
        shared_ptr_return_type<typename T_ContiguousContainer::value_type>>
{
    if (!m_extent.has_value() && dim() == 1)
    {
        m_extent = Extent{data.size()};
    }
    return withRawPtr(data.data());
}
} // namespace openPMD
