

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
#include <optional>
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
ConfigureLoadStore<ChildClass>::ConfigureLoadStore(RecordComponent &rc)
    : ConfigureStoreChunkData(rc)
{}

template <typename ChildClass>
ConfigureLoadStore<ChildClass>::ConfigureLoadStore(
    internal::ConfigureStoreChunkData &&data)
    : ConfigureStoreChunkData(std::move(data))
{}

template <typename ChildClass>
auto ConfigureLoadStore<ChildClass>::dim() const -> uint8_t
{
    return m_rc.getDimensionality();
}

template <typename ChildClass>
auto ConfigureLoadStore<ChildClass>::getOffset() const -> Offset
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
auto ConfigureLoadStore<ChildClass>::getExtent() const -> Extent
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
auto ConfigureLoadStore<ChildClass>::storeChunkConfig() const
    -> internal::StoreChunkConfig
{
    return internal::StoreChunkConfig{getOffset(), getExtent()};
}

template <typename ChildClass>
auto ConfigureLoadStore<ChildClass>::extent(Extent extent) -> return_type &
{
    m_extent = std::move(extent);
    return *static_cast<return_type *>(this);
}

template <typename ChildClass>
auto ConfigureLoadStore<ChildClass>::offset(Offset offset) -> return_type &
{
    m_offset = std::move(offset);
    return *static_cast<return_type *>(this);
}

template <typename ChildClass>
template <typename T>
auto ConfigureLoadStore<ChildClass>::enqueueStore() -> DynamicMemoryView<T>
{
    return m_rc.storeChunkSpan_impl<T>(storeChunkConfig());
}

template <typename Ptr_Type, typename ChildClass>
ConfigureStoreChunkFromBuffer<Ptr_Type, ChildClass>::
    ConfigureStoreChunkFromBuffer(Ptr_Type buffer, parent_t &&parent)
    : parent_t(std::move(parent)), m_buffer(std::move(buffer))
{}

template <typename Ptr_Type, typename ChildClass>
auto ConfigureStoreChunkFromBuffer<Ptr_Type, ChildClass>::as_parent()
    && -> parent_t &&
{
    return std::move(*this);
}
template <typename Ptr_Type, typename ChildClass>
auto ConfigureStoreChunkFromBuffer<Ptr_Type, ChildClass>::as_parent()
    & -> parent_t &
{
    return *this;
}
template <typename Ptr_Type, typename ChildClass>
auto ConfigureStoreChunkFromBuffer<Ptr_Type, ChildClass>::as_parent()
    const & -> parent_t const &
{
    return *this;
}

template <typename Ptr_Type, typename ChildClass>
auto ConfigureStoreChunkFromBuffer<Ptr_Type, ChildClass>::storeChunkConfig()
    const -> internal::StoreChunkConfigFromBuffer
{
    return internal::StoreChunkConfigFromBuffer{
        this->getOffset(), this->getExtent(), m_mem_select};
}

template <typename Ptr_Type, typename ChildClass>
auto ConfigureStoreChunkFromBuffer<Ptr_Type, ChildClass>::memorySelection(
    MemorySelection sel) -> return_type &
{
    this->m_mem_select = std::make_optional<MemorySelection>(std::move(sel));
    return *static_cast<return_type *>(this);
}

template <typename Ptr_Type, typename ChildClass>
auto ConfigureStoreChunkFromBuffer<Ptr_Type, ChildClass>::enqueueStore() -> void
{
    this->m_rc.storeChunk_impl(
        asWriteBuffer(std::move(m_buffer)),
        determineDatatype<auxiliary::IsPointer_t<Ptr_Type>>(),
        storeChunkConfig());
}

template <typename Ptr_Type>
ConfigureLoadStoreFromBuffer<Ptr_Type>::ConfigureLoadStoreFromBuffer(
    Ptr_Type buffer, typename parent_t::parent_t &&parent)
    : parent_t(std::move(buffer), std::move(parent))
{
    static_assert(
        std::is_same_v<
            Ptr_Type,
            std::shared_ptr<typename Ptr_Type::element_type>>,
        "ConfigureLoadStoreFromBuffer must be instantiated with a shared_ptr "
        "type.");
}

#define INSTANTIATE_METHOD_TEMPLATES(base_class, dtype)                        \
    template auto base_class::enqueueStore() -> DynamicMemoryView<dtype>;

#define INSTANTIATE_METHOD_TEMPLATES_FOR_BASE(dtype)                           \
    INSTANTIATE_METHOD_TEMPLATES(ConfigureLoadStore<void>, dtype)

template class ConfigureLoadStore<void>;
OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_METHOD_TEMPLATES_FOR_BASE)

#undef INSTANTIATE_METHOD_TEMPLATES_FOR_BASE

#define INSTANTIATE_STORE_CHUNK_FROM_BUFFER(dtype)                             \
    template class ConfigureLoadStoreFromBuffer<std::shared_ptr<dtype const>>; \
    template class ConfigureStoreChunkFromBuffer<                              \
        std::shared_ptr<dtype const>,                                          \
        ConfigureLoadStoreFromBuffer<std::shared_ptr<dtype const>>>;           \
    template class ConfigureLoadStore<                                         \
        ConfigureLoadStoreFromBuffer<std::shared_ptr<dtype const>>>;           \
    INSTANTIATE_METHOD_TEMPLATES(                                              \
        ConfigureLoadStore<                                                    \
            ConfigureLoadStoreFromBuffer<std::shared_ptr<dtype const>>>,       \
        dtype)                                                                 \
    template class ConfigureStoreChunkFromBuffer<UniquePtrWithLambda<dtype>>;  \
    template class ConfigureLoadStore<                                         \
        ConfigureStoreChunkFromBuffer<UniquePtrWithLambda<dtype>>>;            \
    INSTANTIATE_METHOD_TEMPLATES(                                              \
        ConfigureLoadStore<                                                    \
            ConfigureStoreChunkFromBuffer<UniquePtrWithLambda<dtype>>>,        \
        dtype)

OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_STORE_CHUNK_FROM_BUFFER)

#undef INSTANTIATE_STORE_CHUNK_FROM_BUFFER
#undef INSTANTIATE_METHOD_TEMPLATES

} // namespace openPMD
