#pragma once

#include "openPMD/Dataset.hpp"
#include "openPMD/auxiliary/Future.hpp"
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"

// comment to prevent this include from being moved by clang-format
#include "openPMD/DatatypeMacros.hpp"

#include <optional>
#include <type_traits>

namespace openPMD
{
class RecordComponent;
class ConfigureStoreChunkFromBuffer;
class ConfigureLoadStoreFromBuffer;
template <typename T>
class DynamicMemoryView;
class Attributable;

namespace internal
{
    struct LoadStoreConfig
    {
        Offset offset;
        Extent extent;
    };
    struct LoadStoreConfigWithBuffer
    {
        Offset offset;
        Extent extent;
        std::optional<MemorySelection> memorySelection;
    };

} // namespace internal

namespace auxiliary::detail
{
#define OPENPMD_ENUMERATE_TYPES(type) , std::shared_ptr<type>
    using shared_ptr_dataset_types = auxiliary::detail::variant_tail_t<
        auxiliary::detail::bottom OPENPMD_FOREACH_DATASET_DATATYPE(
            OPENPMD_ENUMERATE_TYPES)>;
#undef OPENPMD_ENUMERATE_TYPES
} // namespace auxiliary::detail

/*
 * Actual data members of `ConfigureLoadStore<>` and methods that don't
 * depend on the ChildClass template parameter. By extracting the members to
 * this struct, we can pass them around between different instances of the
 * class template. Numbers of method instantiations can be reduced.
 */
class ConfigureLoadStore
{
    friend class openPMD::RecordComponent;

protected:
    ConfigureLoadStore(RecordComponent &);
    RecordComponent &m_rc;

    std::optional<Offset> m_offset;
    std::optional<Extent> m_extent;

    bool m_unsafeNoAutomaticFlush = false;

    [[nodiscard]] auto dim() const -> uint8_t;
    auto storeChunkConfig() -> internal::LoadStoreConfig;

    auto deferFlush(Attributable &);

    auto getOffset() -> Offset const &;
    auto getExtent() -> Extent const &;

    // The below methods return void.
    // For chaining calls, they should return *this, but this class right
    // here is going to be somewhere in the inheritance chain, and the final
    // class should be returned. Could be solved more elegantly with CRT,
    // but that blows up compile-time, so we make internal void functions
    // and then repeat them in the final classes.
    // (e.g. ConfigureLoadStoreFromBuffer::offset())

    void offset_impl(Offset);
    void extent_impl(Extent);
    void unsafeNoAutomaticFlush_impl();

private:
    auto withSharedPtr_impl_mut(std::shared_ptr<void> data, Datatype)
        -> openPMD::ConfigureLoadStoreFromBuffer;
    auto withSharedPtr_impl_const(std::shared_ptr<void const> data, Datatype)
        -> openPMD::ConfigureStoreChunkFromBuffer;
    auto withUniquePtr_impl_mut(UniquePtrWithLambda<void>, Datatype)
        -> openPMD::ConfigureStoreChunkFromBuffer;
    auto withUniquePtr_impl_const(UniquePtrWithLambda<void const>, Datatype)
        -> openPMD::ConfigureStoreChunkFromBuffer;
    auto withRawPtr_impl_mut(void *data, Datatype)
        -> openPMD::ConfigureLoadStoreFromBuffer;
    auto withRawPtr_impl_const(void const *data, Datatype)
        -> openPMD::ConfigureStoreChunkFromBuffer;

public:
    using this_t = ConfigureLoadStore;

    // Configuration methods (always available)
    auto offset(Offset offset) -> this_t &
    {
        offset_impl(std::move(offset));
        return *this;
    }
    auto extent(Extent extent) -> this_t &
    {
        extent_impl(std::move(extent));
        return *this;
    }
    auto unsafeNoAutomaticFlush() -> this_t &
    {
        unsafeNoAutomaticFlush_impl();
        return *this;
    }

    /*
     * If the type is non-const, then the return type should be
     * ConfigureLoadStoreFromBuffer, but if it is a const type, Load operations
     * make no sense, so the return type should be
     * ConfigureStoreChunkFromBuffer<>.
     */
    template <typename T>
    using shared_ptr_return_type = std::conditional_t<
        std::is_const_v<T>,
        ConfigureStoreChunkFromBuffer,
        ConfigureLoadStoreFromBuffer>;

    /*
     * As loading into unique pointer types makes no sense, the case is
     * simpler for unique pointers. Just remove the array extents here.
     * (Our interface wrappers still support const-type unique pointers,
     * but the internal logic does not handle them separately.)
     */
    template <typename T>
    using unique_ptr_return_type = openPMD::ConfigureStoreChunkFromBuffer;

    // Buffer specification methods (return specialized configurations)
    template <typename T>
    auto withSharedPtr(std::shared_ptr<T>) -> shared_ptr_return_type<T>;
    template <typename T>
    auto withUniquePtr(UniquePtrWithLambda<T>) -> unique_ptr_return_type<T>;
    template <typename T, typename Del>
    auto withUniquePtr(std::unique_ptr<T, Del>) -> unique_ptr_return_type<T>;
    template <typename T>
    auto withRawPtr(T *data) -> shared_ptr_return_type<T>;
    template <typename T_ContiguousContainer>
    auto withContiguousContainer(T_ContiguousContainer &data)
        -> std::enable_if_t<
            auxiliary::IsContiguousContainer_v<T_ContiguousContainer>,
            shared_ptr_return_type<typename T_ContiguousContainer::value_type>>;

    // Enqueue methods (deferred execution)
    template <typename T>
    [[nodiscard]] auto storeSpan() -> DynamicMemoryView<T>;
    // definition for this one is in RecordComponent.tpp since it needs the
    // definition of class RecordComponent.
    template <typename T, typename F>
    [[nodiscard]] auto storeSpan(F &&createBuffer) -> DynamicMemoryView<T>;

    template <typename T>
    [[nodiscard]] auto load()
        -> auxiliary::DeferredComputation<std::shared_ptr<T>>;

    [[nodiscard]] auto loadVariant() -> auxiliary::DeferredComputation<
        auxiliary::detail::shared_ptr_dataset_types>;
};

class ConfigureStoreChunkFromBuffer : public ConfigureLoadStore
{
    friend class ConfigureLoadStore;

protected:
    auxiliary::WriteBuffer m_buffer;
    Datatype m_datatype;
    std::optional<MemorySelection> m_mem_select;

    ConfigureStoreChunkFromBuffer(
        auxiliary::WriteBuffer buffer, Datatype, ConfigureLoadStore &&);

    // The below methods return void.
    // For chaining calls, they should return *this, but this class right
    // here is going to be somewhere in the inheritance chain, and the final
    // class should be returned. Could be solved more elegantly with CRT,
    // but that blows up compile-time, so we make internal void functions
    // and then repeat them in the final classes.
    void memorySelection_impl(MemorySelection);

    auto storeChunkConfig() -> internal::LoadStoreConfigWithBuffer;

public:
    using this_t = ConfigureStoreChunkFromBuffer;

    // Configuration methods (always available)
    auto offset(Offset offset) -> this_t &
    {
        offset_impl(std::move(offset));
        return *this;
    }
    auto extent(Extent extent) -> this_t &
    {
        extent_impl(std::move(extent));
        return *this;
    }
    auto unsafeNoAutomaticFlush() -> this_t &
    {
        unsafeNoAutomaticFlush_impl();
        return *this;
    }
    auto memorySelection(MemorySelection memorySelection) -> this_t &
    {
        memorySelection_impl(std::move(memorySelection));
        return *this;
    }

    // Enqueue method (deferred execution)
    auto store() -> auxiliary::DeferredComputation<void>;

    /** This intentionally shadows the parent class's enqueueLoad methods in
     * order to show a compile error when using load() on an object
     * of this class. The parent method can still be accessed through
     * typecasting if needed.
     */
    template <typename X = void>
    auto load()
    {
        static_assert(
            auxiliary::dependent_false_v<X>,
            "Cannot load chunk data into a buffer that is const or a "
            "unique_ptr.");
    }
};

class ConfigureLoadStoreFromBuffer : public ConfigureStoreChunkFromBuffer
{
    friend class ConfigureLoadStore;
    friend class RecordComponent;

    using ConfigureStoreChunkFromBuffer::ConfigureStoreChunkFromBuffer;

public:
    using this_t = ConfigureLoadStoreFromBuffer;

    // Configuration methods (always available)
    auto offset(Offset offset) -> this_t &
    {
        offset_impl(std::move(offset));
        return *this;
    }
    auto extent(Extent extent) -> this_t &
    {
        extent_impl(std::move(extent));
        return *this;
    }
    auto unsafeNoAutomaticFlush() -> this_t &
    {
        unsafeNoAutomaticFlush_impl();
        return *this;
    }
    auto memorySelection(MemorySelection memorySelection) -> this_t &
    {
        memorySelection_impl(std::move(memorySelection));
        return *this;
    }

    // Enqueue method (deferred execution)
    auto load() -> auxiliary::DeferredComputation<void>;
};

} // namespace openPMD

#include "openPMD/UndefDatatypeMacros.hpp"
// comment to prevent these includes from being moved by clang-format
#include "openPMD/LoadStoreChunk.tpp"
