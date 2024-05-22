#pragma once

#include "openPMD/LoadStoreChunk.hpp"

namespace openPMD
{
template <typename ChildClass>
template <typename T>
auto ConfigureLoadStore<ChildClass>::fromSharedPtr(std::shared_ptr<T> data)
    -> shared_ptr_return_type<T>
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return shared_ptr_return_type<T>(
        std::static_pointer_cast<normalize_dataset_type<T> const>(
            std::move(data)),
        {std::move(*this)});
}
template <typename ChildClass>
template <typename T>
auto ConfigureLoadStore<ChildClass>::fromUniquePtr(UniquePtrWithLambda<T> data)
    -> unique_ptr_return_type<T>

{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return unique_ptr_return_type<T>(
        std::move(data).template static_cast_<normalize_dataset_type<T>>(),
        {std::move(*this)});
}
template <typename ChildClass>
template <typename T>
auto ConfigureLoadStore<ChildClass>::fromRawPtr(T *data)
    -> shared_ptr_return_type<T>
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return shared_ptr_return_type<T>(
        auxiliary::shareRaw(data), {std::move(*this)});
}

template <typename ChildClass>
template <typename T, typename Del>
auto ConfigureLoadStore<ChildClass>::fromUniquePtr(std::unique_ptr<T, Del> data)
    -> unique_ptr_return_type<T>
{
    return fromUniquePtr(UniquePtrWithLambda<T>(std::move(data)));
}
template <typename ChildClass>
template <typename T_ContiguousContainer>
auto ConfigureLoadStore<ChildClass>::fromContiguousContainer(
    T_ContiguousContainer &data)
    -> std::enable_if_t<
        auxiliary::IsContiguousContainer_v<T_ContiguousContainer>,
        shared_ptr_return_type<typename T_ContiguousContainer::value_type>>
{
    if (!m_extent.has_value() && dim() == 1)
    {
        m_extent = Extent{data.size()};
    }
    return fromRawPtr(data.data());
}
} // namespace openPMD
