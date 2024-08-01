

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

namespace core
{
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

    template <typename T>
    auto ConfigureLoadStore::enqueueStore() -> DynamicMemoryView<T>
    {
        return m_rc.storeChunkSpan_impl<T>(storeChunkConfig());
    }

    template <typename T>
    auto ConfigureLoadStore::enqueueLoad()
        -> auxiliary::DeferredComputation<std::shared_ptr<T>>
    {
        auto res = m_rc.loadChunkAllocate_impl<T>(storeChunkConfig());
        return auxiliary::DeferredComputation<std::shared_ptr<T>>(
            [res_lambda = std::move(res), rc = m_rc]() mutable {
                rc.seriesFlush();
                return res_lambda;
            });
    }

    template <typename T>
    auto ConfigureLoadStore::load(EnqueuePolicy ep) -> std::shared_ptr<T>
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
            -> auxiliary::DeferredComputation<
                auxiliary::detail::shared_ptr_dataset_types>
        {
            auto res = rc.loadChunkAllocate_impl<T>(std::move(cfg));
            return auxiliary::DeferredComputation<
                auxiliary::detail::shared_ptr_dataset_types>(

                [res_lambda = std::move(res), rc_lambda = rc]() mutable
                -> auxiliary::detail::shared_ptr_dataset_types {
                    std::cout << "Flushing Series from Future" << std::endl;
                    rc_lambda.seriesFlush();
                    std::cout << "Flushed Series from Future" << std::endl;
                    return res_lambda;
                });
        }
    };

    auto ConfigureLoadStore::enqueueLoadVariant()
        -> auxiliary::DeferredComputation<
            auxiliary::detail::shared_ptr_dataset_types>
    {
        return m_rc.visit<VisitorEnqueueLoadVariant>(this->storeChunkConfig());
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

    auto ConfigureLoadStore::loadVariant(EnqueuePolicy ep)
        -> auxiliary::detail::shared_ptr_dataset_types
    {
        auto res = m_rc.visit<VisitorLoadVariant>(this->storeChunkConfig());
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

    template <typename Ptr_Type>
    ConfigureStoreChunkFromBuffer<Ptr_Type>::ConfigureStoreChunkFromBuffer(
        Ptr_Type buffer, ConfigureLoadStore &&core)
        : ConfigureLoadStore(std::move(core)), m_buffer(std::move(buffer))
    {}

    template <typename Ptr_Type>
    auto ConfigureStoreChunkFromBuffer<Ptr_Type>::storeChunkConfig()
        -> internal::LoadStoreConfigWithBuffer
    {
        return internal::LoadStoreConfigWithBuffer{
            this->getOffset(), this->getExtent(), m_mem_select};
    }

    template <typename Ptr_Type>
    auto ConfigureStoreChunkFromBuffer<Ptr_Type>::enqueueStore()
        -> auxiliary::DeferredComputation<void>
    {
        this->m_rc.storeChunk_impl(
            asWriteBuffer(std::move(m_buffer)),
            determineDatatype<auxiliary::IsPointer_t<Ptr_Type>>(),
            storeChunkConfig());
        return auxiliary::DeferredComputation<void>(
            [rc_lambda = m_rc]() mutable -> void { rc_lambda.seriesFlush(); });
    }

    template <typename Ptr_Type>
    auto
    ConfigureStoreChunkFromBuffer<Ptr_Type>::store(EnqueuePolicy ep) -> void
    {
        this->m_rc.storeChunk_impl(
            asWriteBuffer(std::move(m_buffer)),
            determineDatatype<auxiliary::IsPointer_t<Ptr_Type>>(),
            storeChunkConfig());
        switch (ep)
        {
        case EnqueuePolicy::Defer:
            break;
        case EnqueuePolicy::Immediate:
            m_rc.seriesFlush();
            break;
        }
    }

    template <typename Ptr_Type>
    auto ConfigureLoadStoreFromBuffer<Ptr_Type>::enqueueLoad() -> void
    {
        static_assert(
            std::is_same_v<
                Ptr_Type,
                std::shared_ptr<
                    std::remove_cv_t<typename Ptr_Type::element_type>>>,
            "ConfigureLoadStoreFromBuffer must be instantiated with a "
            "non-const "
            "shared_ptr type.");
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
} // namespace core

namespace compose
{
    template <typename ChildClass>
    auto ConfigureLoadStore<ChildClass>::extent(Extent extent) -> ChildClass &
    {
        static_cast<ChildClass *>(this)->m_extent =
            std::make_optional<Extent>(std::move(extent));
        return *static_cast<ChildClass *>(this);
    }

    template <typename ChildClass>
    auto ConfigureLoadStore<ChildClass>::offset(Offset offset) -> ChildClass &
    {
        static_cast<ChildClass *>(this)->m_offset =
            std::make_optional<Offset>(std::move(offset));
        return *static_cast<ChildClass *>(this);
    }

    template <typename ChildClass>
    auto ConfigureStoreChunkFromBuffer<ChildClass>::memorySelection(
        MemorySelection sel) -> ChildClass &
    {
        static_cast<ChildClass *>(this)->m_mem_select =
            std::make_optional<MemorySelection>(std::move(sel));
        return *static_cast<ChildClass *>(this);
    }
} // namespace compose

template class compose::ConfigureLoadStore<ConfigureLoadStore>;

/* clang-format would destroy the NOLINT comments */
// clang-format off
#define INSTANTIATE_METHOD_TEMPLATES(dtype)                                    \
    template auto core::ConfigureLoadStore::enqueueStore()                     \
        -> DynamicMemoryView<dtype>;                                           \
    template auto core::ConfigureLoadStore::enqueueLoad()                      \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
        -> auxiliary::DeferredComputation<std::shared_ptr<dtype>>;                  \
    template auto core::ConfigureLoadStore::load(EnqueuePolicy)                \
        ->std::shared_ptr<dtype>;
// clang-format on

OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_METHOD_TEMPLATES)

#undef INSTANTIATE_METHOD_TEMPLATES

/* clang-format would destroy the NOLINT comments */
// clang-format off
#define INSTANTIATE_HALF(pointer_type)                                         \
    template class ConfigureStoreChunkFromBuffer<pointer_type>;                \
    template class core::ConfigureStoreChunkFromBuffer<pointer_type>;          \
    template class compose::ConfigureLoadStore<                                \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
        ConfigureStoreChunkFromBuffer<pointer_type>>;                          \
    template class compose::ConfigureStoreChunkFromBuffer<                     \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
        ConfigureStoreChunkFromBuffer<pointer_type>>;
// clang-format on

/* clang-format would destroy the NOLINT comments */
// clang-format off
#define INSTANTIATE_FULL(pointer_type)                                         \
    INSTANTIATE_HALF(pointer_type)                                             \
    template class ConfigureLoadStoreFromBuffer<pointer_type>;                 \
    template class core::ConfigureLoadStoreFromBuffer<pointer_type>;           \
    template class compose::ConfigureLoadStore<                                \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
  ConfigureLoadStoreFromBuffer<pointer_type>>;                                 \
    template class compose::ConfigureStoreChunkFromBuffer<                     \
   /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                           \
       ConfigureLoadStoreFromBuffer<pointer_type>>;
// clang-format on

#define INSTANTIATE_STORE_CHUNK_FROM_BUFFER(dtype)                             \
    INSTANTIATE_FULL(std::shared_ptr<dtype>)                                   \
    INSTANTIATE_HALF(std::shared_ptr<dtype const>)                             \
    INSTANTIATE_HALF(UniquePtrWithLambda<dtype>)                               \
    INSTANTIATE_HALF(UniquePtrWithLambda<dtype const>)
//  /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */

OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_STORE_CHUNK_FROM_BUFFER)

#undef INSTANTIATE_STORE_CHUNK_FROM_BUFFER
#undef INSTANTIATE_METHOD_TEMPLATES
#undef INSTANTIATE_FULL
#undef INSTANTIATE_HALF

ConfigureLoadStore::ConfigureLoadStore(RecordComponent &rc)
    : core::ConfigureLoadStore{rc}
{}
ConfigureLoadStore::ConfigureLoadStore(core::ConfigureLoadStore &&core)
    : core::ConfigureLoadStore{std::move(core)}
{}
} // namespace openPMD
