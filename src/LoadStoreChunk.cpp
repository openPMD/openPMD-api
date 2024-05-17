

#include "openPMD/LoadStoreChunk.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Span.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"

// comment to keep clang-format from reordering
#include "openPMD/DatatypeMacros.hpp"

#include <stdexcept>

namespace openPMD
{
template <typename ChildClass>
ConfigureStoreChunk<ChildClass>::ConfigureStoreChunk(RecordComponent &rc)
    : m_rc(rc)
{}

template <typename ChildClass>
auto ConfigureStoreChunk<ChildClass>::dim() const -> uint8_t
{
    return m_rc.getDimensionality();
}

template <typename ChildClass>
auto ConfigureStoreChunk<ChildClass>::getOffset() const -> Offset
{
    if (m_offset.has_value())
    {
        return *m_offset;
    }
    else
    {
        if (m_rc.joinedDimension().has_value())
        {
            return Offset{};
        }
        else
        {
            return Offset(dim(), 0);
        }
    }
}

template <typename ChildClass>
auto ConfigureStoreChunk<ChildClass>::getExtent() const -> Extent
{
    if (m_extent.has_value())
    {
        return *m_extent;
    }
    else
    {
        return Extent(dim(), 0);
    }
}

template <typename ChildClass>
auto ConfigureStoreChunk<ChildClass>::storeChunkConfig() const
    -> internal::StoreChunkConfig
{
    return internal::StoreChunkConfig{getOffset(), getExtent()};
}

template <typename ChildClass>
auto ConfigureStoreChunk<ChildClass>::extent(Extent extent) -> return_type &
{
    m_extent = std::move(extent);
    return *this;
}

template <typename ChildClass>
auto ConfigureStoreChunk<ChildClass>::offset(Offset offset) -> return_type &
{
    m_offset = std::move(offset);
    return *this;
}

template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::enqueue() -> DynamicMemoryView<T>
{
    return m_rc.storeChunkSpan_impl<T>(storeChunkConfig());
}

template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::fromSharedPtr(
    std::shared_ptr<T>) && -> TypedConfigureStoreChunk<std::shared_ptr<T>>
{
    throw std::runtime_error("Unimplemented!");
}
template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::fromUniquePtr(UniquePtrWithLambda<T>)
    && -> TypedConfigureStoreChunk<UniquePtrWithLambda<T>>
{
    throw std::runtime_error("Unimplemented!");
}
template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::fromRawPtr(
    T *) && -> TypedConfigureStoreChunk<std::shared_ptr<T>>
{
    throw std::runtime_error("Unimplemented!");
}

template <typename Ptr_Type>
TypedConfigureStoreChunk<Ptr_Type>::TypedConfigureStoreChunk(
    Ptr_Type buffer, parent_t &&parent)
    : parent_t(std::move(parent)), m_buffer(std::move(buffer))
{}

template <typename Ptr_Type>
auto TypedConfigureStoreChunk<Ptr_Type>::as_parent() && -> parent_t &&
{
    return std::move(*this);
}
template <typename Ptr_Type>
auto TypedConfigureStoreChunk<Ptr_Type>::as_parent() & -> parent_t &
{
    return *this;
}
template <typename Ptr_Type>
auto TypedConfigureStoreChunk<Ptr_Type>::as_parent() const & -> parent_t const &
{
    return *this;
}

#define INSTANTIATE_METHOD_TEMPLATES(base_class, dtype)                        \
    template auto base_class::enqueue() -> DynamicMemoryView<dtype>;           \
    template auto base_class::fromSharedPtr(std::shared_ptr<dtype>)            \
        && -> TypedConfigureStoreChunk<std::shared_ptr<dtype>>;                \
    template auto base_class::fromUniquePtr(UniquePtrWithLambda<dtype>)        \
        && -> TypedConfigureStoreChunk<UniquePtrWithLambda<dtype>>;            \
    template auto base_class::fromRawPtr(                                      \
        dtype *) && -> TypedConfigureStoreChunk<std::shared_ptr<dtype>>;

#define INSTANTIATE_METHOD_TEMPLATES_FOR_BASE(dtype)                           \
    INSTANTIATE_METHOD_TEMPLATES(ConfigureStoreChunk<void>, dtype)

template class ConfigureStoreChunk<void>;
OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_METHOD_TEMPLATES_FOR_BASE)

#undef INSTANTIATE_METHOD_TEMPLATES_FOR_BASE

#define INSTANTIATE_TYPED_STORE_CHUNK(dtype)                                   \
    template class TypedConfigureStoreChunk<std::shared_ptr<dtype>>;           \
    INSTANTIATE_METHOD_TEMPLATES(                                              \
        ConfigureStoreChunk<TypedConfigureStoreChunk<std::shared_ptr<dtype>>>, \
        dtype)                                                                 \
    template class TypedConfigureStoreChunk<UniquePtrWithLambda<dtype>>;       \
    INSTANTIATE_METHOD_TEMPLATES(                                              \
        ConfigureStoreChunk<                                                   \
            TypedConfigureStoreChunk<UniquePtrWithLambda<dtype>>>,             \
        dtype)

OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_TYPED_STORE_CHUNK)

#undef INSTANTIATE_TYPED_STORE_CHUNK
#undef INSTANTIATE_METHOD_TEMPLATES

} // namespace openPMD
