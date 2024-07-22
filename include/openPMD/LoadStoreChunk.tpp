#pragma once

#include "openPMD/LoadStoreChunk.hpp"

namespace openPMD::core
{
template <typename T>
auto ConfigureLoadStore::withSharedPtr(std::shared_ptr<T> data)
    -> shared_ptr_return_type<T>
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return shared_ptr_return_type<T>(
        std::static_pointer_cast<std::remove_extent_t<T>>(std::move(data)),
        {std::move(*this)});
}
template <typename T>
auto ConfigureLoadStore::withUniquePtr(UniquePtrWithLambda<T> data)
    -> unique_ptr_return_type<T>

{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return unique_ptr_return_type<T>(
        std::move(data).template static_cast_<std::remove_extent_t<T>>(),
        {std::move(*this)});
}
template <typename T>
auto ConfigureLoadStore::withRawPtr(T *data) -> shared_ptr_return_type<T>
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return shared_ptr_return_type<T>(
        auxiliary::shareRaw(data), {std::move(*this)});
}

template <typename T, typename Del>
auto ConfigureLoadStore::withUniquePtr(std::unique_ptr<T, Del> data)
    -> unique_ptr_return_type<T>
{
    return withUniquePtr(UniquePtrWithLambda<T>(std::move(data)));
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
} // namespace openPMD::core
