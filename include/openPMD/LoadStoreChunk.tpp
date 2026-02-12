#pragma once

#include "openPMD/LoadStoreChunk.hpp"

namespace openPMD
{
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
} // namespace openPMD
