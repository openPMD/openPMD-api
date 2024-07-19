

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

#include <future>
#include <memory>
#include <optional>
#include <stdexcept>

namespace openPMD
{
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

    /*
     * There is no backend support currently for const unique pointers.
     * We support these mostly for providing a clean API to users that have such
     * pointers and want to store from them, but there will be no
     * backend-specific optimizations for such buffers as there are for
     * non-const unique pointers.
     */
    template <typename T>
    auto
    asWriteBuffer(UniquePtrWithLambda<T const> &&ptr) -> auxiliary::WriteBuffer
    {
        auto raw_ptr = ptr.release();
        return asWriteBuffer(std::shared_ptr<T const>{
            raw_ptr,
            [deleter = std::move(ptr.get_deleter())](auto const *delete_me) {
                deleter(delete_me);
            }});
    }
} // namespace

ConfigureLoadStoreCore::ConfigureLoadStoreCore(RecordComponent &rc) : m_rc(rc)
{}

template <typename ChildClass>
ConfigureLoadStore<ChildClass>::ConfigureLoadStore(RecordComponent &rc)
    : ConfigureLoadStoreCore(rc)
{}

template <typename ChildClass>
ConfigureLoadStore<ChildClass>::ConfigureLoadStore(
    ConfigureLoadStoreCore &&data)
    : ConfigureLoadStoreCore(std::move(data))
{}

auto ConfigureLoadStoreCore::dim() const -> uint8_t
{
    return m_rc.getDimensionality();
}

auto ConfigureLoadStoreCore::getOffset() -> Offset const &
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

auto ConfigureLoadStoreCore::getExtent() -> Extent const &
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

auto ConfigureLoadStoreCore::storeChunkConfig() -> internal::LoadStoreConfig
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

template <typename T>
auto ConfigureLoadStoreCore::enqueueStore() -> DynamicMemoryView<T>
{
    return m_rc.storeChunkSpan_impl<T>(storeChunkConfig());
}

template <typename T>
auto ConfigureLoadStoreCore::enqueueLoad() -> std::future<std::shared_ptr<T>>
{
    auto res = m_rc.loadChunkAllocate_impl<T>(storeChunkConfig());
    return std::async(
        std::launch::deferred,
        [res_lambda = std::move(res), rc = m_rc]() mutable {
            rc.seriesFlush();
            return res_lambda;
        });
}

template <typename T>
auto ConfigureLoadStoreCore::load(EnqueuePolicy ep) -> std::shared_ptr<T>
{
    auto res = m_rc.loadChunkAllocate_impl<T>(storeChunkConfig());
    switch (ep)
    {
    case EnqueuePolicy::Defer:
        break;
    case EnqueuePolicy::Immediate:
        m_rc.seriesFlush();
        break;
    }
    return res;
}

struct VisitorEnqueueLoadVariant
{
    template <typename T>
    static auto call(RecordComponent &rc, internal::LoadStoreConfig cfg)
        -> std::future<auxiliary::detail::future_to_shared_ptr_dataset_types>
    {
        auto res = rc.loadChunkAllocate_impl<T>(std::move(cfg));
        return std::async(
            std::launch::deferred,
            [res_lambda = std::move(res), rc_lambda = rc]() mutable
            -> auxiliary::detail::future_to_shared_ptr_dataset_types {
                rc_lambda.seriesFlush();
                return res_lambda;
            });
    }

    static auto non_templated_implementation(
        RecordComponent &rc, internal::LoadStoreConfig cfg)
        -> std::future<auxiliary::detail::future_to_shared_ptr_dataset_types>
    {
        return rc.visit<VisitorEnqueueLoadVariant>(std::move(cfg));
    }
};

auto ConfigureLoadStoreCore::enqueueLoadVariant()
    -> std::future<auxiliary::detail::future_to_shared_ptr_dataset_types>
{
    return VisitorEnqueueLoadVariant::non_templated_implementation(
        m_rc, this->storeChunkConfig());
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

template <typename Ptr_Type>
auto ConfigureLoadStoreFromBuffer<Ptr_Type>::load(EnqueuePolicy ep) -> void
{
    this->m_rc.loadChunk_impl(
        std::move(this->m_buffer), this->storeChunkConfig());
    switch (ep)
    {

    case EnqueuePolicy::Defer:
        break;
    case EnqueuePolicy::Immediate:
        this->m_rc.seriesFlush();
        break;
    }
}

/* clang-format would destroy the NOLINT comments */
// clang-format off
#define INSTANTIATE_METHOD_TEMPLATES(dtype)                                    \
    template auto ConfigureLoadStoreCore::enqueueStore()                       \
        ->DynamicMemoryView<dtype>;                                            \
    template auto ConfigureLoadStoreCore::enqueueLoad()                        \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
        ->std::future<std::shared_ptr<dtype>>;                                 \
    template auto ConfigureLoadStoreCore::load(EnqueuePolicy)                  \
        ->std::shared_ptr<dtype>;
// clang-format on

template class ConfigureLoadStore<void>;
OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_METHOD_TEMPLATES)

#undef INSTANTIATE_METHOD_TEMPLATES

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
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
    template class ConfigureStoreChunkFromBuffer<UniquePtrWithLambda<dtype>>;  \
    template class ConfigureLoadStore<                                         \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
        ConfigureStoreChunkFromBuffer<UniquePtrWithLambda<dtype>>>;            \
    template class ConfigureStoreChunkFromBuffer<                              \
        std::shared_ptr<dtype const>>;                                         \
    template class ConfigureLoadStore<                                         \
        ConfigureStoreChunkFromBuffer<std::shared_ptr<dtype const>>>;          \
    template class ConfigureStoreChunkFromBuffer<                              \
        UniquePtrWithLambda<dtype const>>;                                     \
    template class ConfigureLoadStore<                                         \
        ConfigureStoreChunkFromBuffer<UniquePtrWithLambda<dtype const>>>;
// clang-format on

OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_STORE_CHUNK_FROM_BUFFER)

#undef INSTANTIATE_STORE_CHUNK_FROM_BUFFER
#undef INSTANTIATE_METHOD_TEMPLATES

} // namespace openPMD
