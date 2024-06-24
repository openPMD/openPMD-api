

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
    ConfigureLoadStoreData::ConfigureLoadStoreData(RecordComponent &rc)
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
    : ConfigureLoadStoreData(rc)
{}

template <typename ChildClass>
ConfigureLoadStore<ChildClass>::ConfigureLoadStore(
    internal::ConfigureLoadStoreData &&data)
    : ConfigureLoadStoreData(std::move(data))
{}

template <typename ChildClass>
auto ConfigureLoadStore<ChildClass>::dim() const -> uint8_t
{
    return m_rc.getDimensionality();
}

template <typename ChildClass>
auto ConfigureLoadStore<ChildClass>::getOffset() -> Offset const &
{
    if (!m_offset.has_value())
    {
        if (m_rc.joinedDimension().has_value())
        {
            m_offset = std::make_optional<Offset>();
        }
        else
        {
            m_offset = std::make_optional<Offset>(dim(), 0);
        }
    }
    return *m_offset;
}

template <typename ChildClass>
auto ConfigureLoadStore<ChildClass>::getExtent() -> Extent const &
{
    if (!m_extent.has_value())
    {
        m_extent = std::make_optional<Extent>(m_rc.getExtent());
        if (m_offset.has_value())
        {
            auto it_o = m_offset->begin();
            auto end_o = m_offset->end();
            auto it_e = m_extent->begin();
            auto end_e = m_extent->end();
            for (; it_o != end_o && it_e != end_e; ++it_e, ++it_o)
            {
                *it_e -= *it_o;
            }
        }
    }
    return *m_extent;
}

template <typename ChildClass>
auto ConfigureLoadStore<ChildClass>::storeChunkConfig()
    -> internal::LoadStoreConfig
{
    return internal::LoadStoreConfig{getOffset(), getExtent()};
}

template <typename ChildClass>
auto ConfigureLoadStore<ChildClass>::extent(Extent extent) -> return_type &
{
    m_extent = std::make_optional<Extent>(std::move(extent));
    return *static_cast<return_type *>(this);
}

template <typename ChildClass>
auto ConfigureLoadStore<ChildClass>::offset(Offset offset) -> return_type &
{
    m_offset = std::make_optional<Offset>(std::move(offset));
    return *static_cast<return_type *>(this);
}

template <typename ChildClass>
template <typename T>
auto ConfigureLoadStore<ChildClass>::enqueueStore() -> DynamicMemoryView<T>
{
    return m_rc.storeChunkSpan_impl<T>(storeChunkConfig());
}

template <typename ChildClass>
template <typename T>
auto ConfigureLoadStore<ChildClass>::enqueueLoad() -> std::shared_ptr<T>
{
    return m_rc.loadChunkAllocate_impl<T>(storeChunkConfig());
}

namespace
{
    template <typename ConfigureLoadStore_t>
    struct VisitorEnqueueLoadVariant
    {
        template <typename T>
        static auto call(RecordComponent const &, ConfigureLoadStore_t &cfg) ->
            typename ConfigureLoadStore_t::shared_ptr_dataset_types
        {
            return cfg.template enqueueLoad<T>();
        }
    };
} // namespace

template <typename ChildClass>
auto ConfigureLoadStore<ChildClass>::enqueueLoadVariant()
    -> shared_ptr_dataset_types
{
    return m_rc
        .visit<VisitorEnqueueLoadVariant<ConfigureLoadStore<ChildClass>>>(
            *this);
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
    -> internal::LoadStoreConfigWithBuffer
{
    return internal::LoadStoreConfigWithBuffer{
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

template <typename Ptr_Type>
auto ConfigureLoadStoreFromBuffer<Ptr_Type>::enqueueLoad() -> void
{
    this->m_rc.loadChunk_impl(
        std::move(this->m_buffer), this->storeChunkConfig());
}

#define INSTANTIATE_METHOD_TEMPLATES(base_class, dtype)                        \
    template auto base_class::enqueueStore() -> DynamicMemoryView<dtype>;      \
    template auto base_class::enqueueLoad() -> std::shared_ptr<dtype>;

#define INSTANTIATE_METHOD_TEMPLATES_FOR_BASE(dtype)                           \
    INSTANTIATE_METHOD_TEMPLATES(ConfigureLoadStore<void>, dtype)

template class ConfigureLoadStore<void>;
OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_METHOD_TEMPLATES_FOR_BASE)

#undef INSTANTIATE_METHOD_TEMPLATES_FOR_BASE

/* clang-format would destroy the NOLINT comments */
// clang-format off
#define INSTANTIATE_STORE_CHUNK_FROM_BUFFER(dtype)                             \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
    template class ConfigureLoadStoreFromBuffer<std::shared_ptr<dtype>>;       \
    template class ConfigureStoreChunkFromBuffer<                              \
        std::shared_ptr<dtype>,                                                \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
    ConfigureLoadStoreFromBuffer<std::shared_ptr<dtype>>>;                     \
    template class ConfigureLoadStore<                                         \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
        ConfigureLoadStoreFromBuffer<std::shared_ptr<dtype>>>;                 \
    INSTANTIATE_METHOD_TEMPLATES(                                              \
        ConfigureLoadStore<                                                    \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
            ConfigureLoadStoreFromBuffer<std::shared_ptr<dtype>>>,             \
        dtype)                                                                 \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
    template class ConfigureStoreChunkFromBuffer<UniquePtrWithLambda<dtype>>;  \
    template class ConfigureLoadStore<                                         \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
        ConfigureStoreChunkFromBuffer<UniquePtrWithLambda<dtype>>>;            \
    INSTANTIATE_METHOD_TEMPLATES(                                              \
        ConfigureLoadStore<                                                    \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
            ConfigureStoreChunkFromBuffer<UniquePtrWithLambda<dtype>>>,        \
        dtype)                                                                 \
    template class ConfigureStoreChunkFromBuffer<                              \
        std::shared_ptr<dtype const>>;                                         \
    template class ConfigureLoadStore<                                         \
        ConfigureStoreChunkFromBuffer<std::shared_ptr<dtype const>>>;          \
    INSTANTIATE_METHOD_TEMPLATES(                                              \
        ConfigureLoadStore<                                                    \
            ConfigureStoreChunkFromBuffer<std::shared_ptr<dtype const>>>,      \
        dtype)
// clang-format on

OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_STORE_CHUNK_FROM_BUFFER)

#undef INSTANTIATE_STORE_CHUNK_FROM_BUFFER
#undef INSTANTIATE_METHOD_TEMPLATES

} // namespace openPMD
