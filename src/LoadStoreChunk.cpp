

#include "openPMD/LoadStoreChunk.hpp"
#include "openPMD/Datatype.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Span.hpp"
#include "openPMD/auxiliary/Future.hpp"
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/auxiliary/Memory_internal.hpp"
#include "openPMD/auxiliary/ShareRawInternal.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"

// comment to keep clang-format from reordering
#include "openPMD/DatatypeMacros.hpp"
#include "openPMD/backend/Attributable.hpp"

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
    auto asWriteBuffer(UniquePtrWithLambda<T const> &&ptr)
        -> auxiliary::WriteBuffer
    {
        auto raw_ptr = ptr.release();
        return asWriteBuffer(
            std::shared_ptr<T const>{
                raw_ptr,
                [deleter = std::move(ptr.get_deleter())](
                    auto const *delete_me) { deleter(delete_me); }});
    }
} // namespace

ConfigureLoadStore::ConfigureLoadStore(RecordComponent &rc) : m_rc(rc)
{}

auto ConfigureLoadStore::dim() const -> uint8_t
{
    return m_rc.getDimensionality();
}

auto ConfigureLoadStore::storeChunkConfig() -> internal::LoadStoreConfig
{
    return internal::LoadStoreConfig{getOffset(), getExtent()};
}

auto ConfigureLoadStore::deferFlush(Attributable &attr)
{
    if (m_unsafeNoAutomaticFlush)
    {
        throw error::Internal(
            "Configuring an automatic flush operating after configuring that "
            "those should be switched off.");
    }
    auto index = attr.IOHandler()->m_flushCounter;
    return [attr,
            old_index = *index,
            current_index = std::weak_ptr(index)]() mutable {
        auto lock_current_index = current_index.lock();
        if (!lock_current_index || *lock_current_index >= old_index)
        {
            return;
        }
        attr.seriesFlush();
    };
}

auto ConfigureLoadStore::getOffset() -> Offset const &
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

auto ConfigureLoadStore::getExtent() -> Extent const &
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

auto ConfigureLoadStore::withSharedPtr_impl_mut(
    std::shared_ptr<void> data, Datatype datatype)
    -> openPMD::ConfigureLoadStoreFromBuffer
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return openPMD::ConfigureLoadStoreFromBuffer(
        auxiliary::WriteBuffer(std::move(data)), datatype, {std::move(*this)});
}
auto ConfigureLoadStore::withSharedPtr_impl_const(
    std::shared_ptr<void const> data, Datatype datatype)
    -> openPMD::ConfigureStoreChunkFromBuffer
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return openPMD::ConfigureStoreChunkFromBuffer(
        auxiliary::WriteBuffer(std::move(data)), datatype, {std::move(*this)});
}

auto ConfigureLoadStore::withUniquePtr_impl_mut(
    UniquePtrWithLambda<void> data, Datatype dtype)
    -> openPMD::ConfigureStoreChunkFromBuffer

{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }

    return openPMD::ConfigureStoreChunkFromBuffer(
        auxiliary::WriteBuffer(std::move(data)), dtype, {std::move(*this)});
}
auto ConfigureLoadStore::withUniquePtr_impl_const(
    UniquePtrWithLambda<void const> data, Datatype dtype)
    -> openPMD::ConfigureStoreChunkFromBuffer

{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }

    void const *raw_ptr = data.release();
    auto &deleter = data.get_deleter();
    return openPMD::ConfigureStoreChunkFromBuffer(
        auxiliary::WriteBuffer(
            std::shared_ptr<void const>(
                raw_ptr,
                [deleter_lambda = std::move(deleter)](auto const *p) {
                    deleter_lambda(p);
                })),
        dtype,
        {std::move(*this)});
}

auto ConfigureLoadStore::withRawPtr_impl_mut(void *data, Datatype dtype)
    -> openPMD::ConfigureLoadStoreFromBuffer
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return openPMD::ConfigureLoadStoreFromBuffer(
        auxiliary::WriteBuffer(auxiliary::shareRaw(data)),
        dtype,
        {std::move(*this)});
}

auto ConfigureLoadStore::withRawPtr_impl_const(void const *data, Datatype dtype)
    -> openPMD::ConfigureStoreChunkFromBuffer
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return openPMD::ConfigureStoreChunkFromBuffer(
        auxiliary::WriteBuffer(auxiliary::shareRaw(data)),
        dtype,
        {std::move(*this)});
}

template <typename T>
auto ConfigureLoadStore::storeSpan() -> DynamicMemoryView<T>
{
    return m_rc.storeChunkSpan_impl<T>(storeChunkConfig());
}

template <typename T>
auto ConfigureLoadStore::load()
    -> auxiliary::DeferredComputation<std::shared_ptr<T>>
{
    auto res = m_rc.loadChunkAllocate_impl<T>(storeChunkConfig());
    if (m_unsafeNoAutomaticFlush)
    {
        return auxiliary::DeferredComputation<std::shared_ptr<T>>(
            std::move(res));
    }
    return auxiliary::DeferredComputation<std::shared_ptr<T>>(
        [res_lambda = std::move(res), dflush = deferFlush(m_rc)]() mutable {
            dflush();
            return res_lambda;
        });
}

struct VisitorEnqueueLoadVariantWithFlush
{
    template <typename T, typename F>
    static auto
    call(RecordComponent &rc, internal::LoadStoreConfig cfg, F &&dflush)
        -> auxiliary::DeferredComputation<
            auxiliary::detail::shared_ptr_dataset_types>
    {
        auto res = rc.loadChunkAllocate_impl<T>(std::move(cfg));
        return auxiliary::DeferredComputation<
            auxiliary::detail::shared_ptr_dataset_types>(
            [res_lambda = std::move(res),
             dflush_lambda = std::forward<F>(dflush)]() mutable
                -> auxiliary::detail::shared_ptr_dataset_types {
                dflush_lambda();
                return res_lambda;
            });
    }
};
struct VisitorEnqueueLoadVariantWithoutFlush
{
    template <typename T>
    static auto call(RecordComponent &rc, internal::LoadStoreConfig cfg)
        -> auxiliary::DeferredComputation<
            auxiliary::detail::shared_ptr_dataset_types>
    {
        auto res = rc.loadChunkAllocate_impl<T>(std::move(cfg));
        return auxiliary::DeferredComputation<
            auxiliary::detail::shared_ptr_dataset_types>(std::move(res));
    }
};

auto ConfigureLoadStore::loadVariant() -> auxiliary::DeferredComputation<
    auxiliary::detail::shared_ptr_dataset_types>
{
    if (m_unsafeNoAutomaticFlush)
    {
        return m_rc.visit<VisitorEnqueueLoadVariantWithoutFlush>(
            this->storeChunkConfig());
    }
    else
    {
        return m_rc.visit<VisitorEnqueueLoadVariantWithFlush>(
            this->storeChunkConfig(), deferFlush(m_rc));
    }
}

struct VisitorLoadVariant
{
    template <typename T>
    static auto call(RecordComponent &rc, internal::LoadStoreConfig cfg)
        -> auxiliary::detail::shared_ptr_dataset_types
    {
        return rc.loadChunkAllocate_impl<T>(std::move(cfg));
    }
};

ConfigureStoreChunkFromBuffer::ConfigureStoreChunkFromBuffer(
    auxiliary::WriteBuffer buffer, Datatype dt, ConfigureLoadStore &&core)
    : ConfigureLoadStore(std::move(core))
    , m_buffer(std::move(buffer))
    , m_datatype(dt)
{}

auto ConfigureStoreChunkFromBuffer::storeChunkConfig()
    -> internal::LoadStoreConfigWithBuffer
{
    return internal::LoadStoreConfigWithBuffer{
        this->getOffset(), this->getExtent(), m_mem_select};
}

auto ConfigureStoreChunkFromBuffer::store()
    -> auxiliary::DeferredComputation<void>
{
    this->m_rc.storeChunk_impl(
        std::move(m_buffer), m_datatype, storeChunkConfig());
    if (m_unsafeNoAutomaticFlush)
    {
        return auxiliary::DeferredComputation<void>(
            auxiliary::detail::CachedValue<void>());
    }
    return auxiliary::DeferredComputation<void>(
        [dflush = deferFlush(m_rc)]() mutable -> void { dflush(); });
}

auto ConfigureLoadStoreFromBuffer::load()
    -> auxiliary::DeferredComputation<void>
{
    auto *shared_ptr = std::get_if<auxiliary::WriteBuffer::ReadSharedPtr>(
        &this->m_buffer.as_variant<auxiliary::WriteBufferTypes>());
    if (!shared_ptr)
    {
        throw std::runtime_error(
            "ConfigureLoadStoreFromBuffer must be instantiated with a "
            "non-const shared_ptr type.");
    }
    this->m_rc.loadChunk_impl(
        *shared_ptr, m_datatype, this->storeChunkConfig());
    if (m_unsafeNoAutomaticFlush)
    {
        return auxiliary::DeferredComputation<void>(
            auxiliary::detail::CachedValue<void>());
    }
    return auxiliary::DeferredComputation<void>(
        [dflush = this->deferFlush(this->m_rc)]() mutable -> void {
            dflush();
        });
}

void ConfigureLoadStore::extent_impl(Extent extent)
{
    m_extent = std::make_optional<Extent>(std::move(extent));
}

void ConfigureLoadStore::offset_impl(Offset offset)
{
    m_offset = std::make_optional<Offset>(std::move(offset));
}

void ConfigureLoadStore::unsafeNoAutomaticFlush_impl()
{
    m_unsafeNoAutomaticFlush = true;
}

void ConfigureStoreChunkFromBuffer::memorySelection_impl(MemorySelection sel)
{
    m_mem_select = std::make_optional<MemorySelection>(std::move(sel));
}
// namespace core

// need this for clang-tidy
#define OPENPMD_ARRAY(type) type[]
#define OPENPMD_POINTER(type) type *
#define OPENPMD_APPLY_TEMPLATE(template_, type) template_<type>

#define INSTANTIATE_METHOD_TEMPLATES(dtype)                                    \
    template auto ConfigureLoadStore::load()                                   \
        -> auxiliary::DeferredComputation<OPENPMD_APPLY_TEMPLATE(              \
            std::shared_ptr, dtype)>;
#define INSTANTIATE_METHOD_TEMPLATES_WITH_AND_WITHOUT_EXTENT(type)             \
    INSTANTIATE_METHOD_TEMPLATES(type)                                         \
    INSTANTIATE_METHOD_TEMPLATES(OPENPMD_ARRAY(type))                          \
    template auto ConfigureLoadStore::storeSpan() -> DynamicMemoryView<type>;

OPENPMD_FOREACH_DATASET_DATATYPE(
    INSTANTIATE_METHOD_TEMPLATES_WITH_AND_WITHOUT_EXTENT)

#undef INSTANTIATE_METHOD_TEMPLATES
#undef INSTANTIATE_METHOD_TEMPLATES_WITH_AND_WITHOUT_EXTENT

#undef INSTANTIATE_METHOD_TEMPLATES
#undef OPENPMD_ARRAY
#undef OPENPMD_POINTER
#undef OPENPMD_APPLY_TEMPLATE
} // namespace openPMD
