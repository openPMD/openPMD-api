

#include "openPMD/LoadStoreChunk.hpp"
#include "openPMD/Datatype.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Span.hpp"
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/auxiliary/ShareRawInternal.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"

// comment to keep clang-format from reordering
#include "openPMD/DatatypeMacros.hpp"

#include <memory>
#include <stdexcept>

namespace openPMD
{

namespace internal
{
    ConfigureStoreChunkData::ConfigureStoreChunkData(RecordComponent &rc)
        : m_rc(rc)
    {}
} // namespace internal

namespace
{
    template <typename T>
    auto asWriteBuffer(std::shared_ptr<T> &&ptr) -> auxiliary::WriteBuffer
    {
        /* std::static_pointer_cast correctly reference-counts the pointer */
        return auxiliary::WriteBuffer(
            std::static_pointer_cast<void const>(std::move(ptr)));
    }
    template <typename T>
    auto asWriteBuffer(UniquePtrWithLambda<T> &&ptr) -> auxiliary::WriteBuffer
    {
        return auxiliary::WriteBuffer(
            std::move(ptr).template static_cast_<void>());
    }
} // namespace

template <typename ChildClass>
ConfigureStoreChunk<ChildClass>::ConfigureStoreChunk(RecordComponent &rc)
    : ConfigureStoreChunkData(rc)
{}

template <typename ChildClass>
ConfigureStoreChunk<ChildClass>::ConfigureStoreChunk(
    internal::ConfigureStoreChunkData &&data)
    : ConfigureStoreChunkData(std::move(data))
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
    return *static_cast<return_type *>(this);
}

template <typename ChildClass>
auto ConfigureStoreChunk<ChildClass>::offset(Offset offset) -> return_type &
{
    m_offset = std::move(offset);
    return *static_cast<return_type *>(this);
}

template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::enqueue() -> DynamicMemoryView<T>
{
    return m_rc.storeChunkSpan_impl<T>(storeChunkConfig());
}

template <typename Ptr_Type>
ConfigureStoreChunkFromBuffer<Ptr_Type>::ConfigureStoreChunkFromBuffer(
    Ptr_Type buffer, parent_t &&parent)
    : parent_t(std::move(parent)), m_buffer(std::move(buffer))
{}

template <typename Ptr_Type>
auto ConfigureStoreChunkFromBuffer<Ptr_Type>::as_parent() && -> parent_t &&
{
    return std::move(*this);
}
template <typename Ptr_Type>
auto ConfigureStoreChunkFromBuffer<Ptr_Type>::as_parent() & -> parent_t &
{
    return *this;
}
template <typename Ptr_Type>
auto ConfigureStoreChunkFromBuffer<Ptr_Type>::as_parent()
    const & -> parent_t const &
{
    return *this;
}

template <typename Ptr_Type>
auto ConfigureStoreChunkFromBuffer<Ptr_Type>::storeChunkConfig() const
    -> internal::StoreChunkConfigFromBuffer
{
    return internal::StoreChunkConfigFromBuffer{
        this->getOffset(), this->getExtent(), m_mem_select};
}

template <typename Ptr_Type>
auto ConfigureStoreChunkFromBuffer<Ptr_Type>::enqueue() -> void
{
    this->m_rc.storeChunk_impl(
        asWriteBuffer(std::move(m_buffer)),
        determineDatatype<auxiliary::IsPointer_t<Ptr_Type>>(),
        storeChunkConfig());
}

#define INSTANTIATE_METHOD_TEMPLATES(base_class, dtype)                        \
    template auto base_class::enqueue() -> DynamicMemoryView<dtype>;

#define INSTANTIATE_METHOD_TEMPLATES_FOR_BASE(dtype)                           \
    INSTANTIATE_METHOD_TEMPLATES(ConfigureStoreChunk<void>, dtype)

template class ConfigureStoreChunk<void>;
OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_METHOD_TEMPLATES_FOR_BASE)

#undef INSTANTIATE_METHOD_TEMPLATES_FOR_BASE

#define INSTANTIATE_STORE_CHUNK_FROM_BUFFER(dtype)                             \
    template class ConfigureStoreChunkFromBuffer<                              \
        std::shared_ptr<dtype const>>;                                         \
    template class ConfigureStoreChunk<                                        \
        ConfigureStoreChunkFromBuffer<std::shared_ptr<dtype const>>>;          \
    INSTANTIATE_METHOD_TEMPLATES(                                              \
        ConfigureStoreChunk<                                                   \
            ConfigureStoreChunkFromBuffer<std::shared_ptr<dtype const>>>,      \
        dtype)                                                                 \
    template class ConfigureStoreChunkFromBuffer<UniquePtrWithLambda<dtype>>;  \
    template class ConfigureStoreChunk<                                        \
        ConfigureStoreChunkFromBuffer<UniquePtrWithLambda<dtype>>>;            \
    INSTANTIATE_METHOD_TEMPLATES(                                              \
        ConfigureStoreChunk<                                                   \
            ConfigureStoreChunkFromBuffer<UniquePtrWithLambda<dtype>>>,        \
        dtype)

OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_STORE_CHUNK_FROM_BUFFER)

#undef INSTANTIATE_STORE_CHUNK_FROM_BUFFER
#undef INSTANTIATE_METHOD_TEMPLATES

} // namespace openPMD
