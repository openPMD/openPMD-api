#pragma once

#include "openPMD/Dataset.hpp"
#include "openPMD/LoadStoreAPI.hpp"
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
    /** Internal configuration for load/store operations without buffer. Default
     * values for optionally specified parameters (offset, extent) must be
     * computed to create this configuration struct. */
    struct LoadStoreConfig
    {
        Offset offset;
        Extent extent;
        API api = API::chaining;
    };
    /** Internal configuration for load/store operations with buffer. Default
     * values for optionally specified parameters (offset, extent) must be
     * computed to create this configuration struct. MemorySelection remains
     * optional even then. */
    struct LoadStoreConfigWithBuffer
    {
        Offset offset;
        Extent extent;
        std::optional<MemorySelection> memorySelection;
        API api = API::chaining;
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

/** Base class for configuring load/store chunk operations.
 *
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
    std::shared_ptr<RecordComponent> m_rc;

    std::optional<Offset> m_offset;
    std::optional<Extent> m_extent;
    internal::API api = internal::API::chaining;

    bool m_unsafeNoAutomaticFlush = false;

public:
    ConfigureLoadStore(ConfigureLoadStore const &other);
    ConfigureLoadStore &operator=(ConfigureLoadStore const &other);
    ConfigureLoadStore(ConfigureLoadStore &&);
    ConfigureLoadStore &operator=(ConfigureLoadStore &&);

protected:
    [[nodiscard]] auto dim() const -> uint8_t;
    auto storeChunkConfig() -> internal::LoadStoreConfig;

    auto deferFlush(RecordComponent &);

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

    virtual auto getBufferSize() -> std::optional<size_t>;

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

    /** Retrieve the configured offset. If no offset has been specified, compute
     *  and store it now (default: full dataset selection). May be overwritten
     *  at a later point using offset().
     */
    auto computeOffset() -> Offset const &;
    /** Retrieve the configured extent. If no extent has been specified, compute
     *  and store it now (default: full dataset selection). May be overwritten
     *  at a later point using extent().
     */
    auto computeExtent() -> Extent const &;

    // Configuration methods (always available)

    /** Set the offset within the dataset
     *
     * Optional. The operation will apply without offset by default (i.e. offset
     * = (0, 0, ...)).
     *
     * @param offset Offset within the dataset
     * @return Reference to this object for chaining
     */
    auto offset(Offset offset) -> this_t &
    {
        offset_impl(std::move(offset));
        return *this;
    }
    /** Set the extent within the dataset
     *
     * Optional. The operation will apply to the entire dataset by default (i.e.
     * operation extent = global dataset extent - operation offset).
     *
     * @param extent Extent within the dataset, counted from the offset
     * @return Reference to this object for chaining
     */
    auto extent(Extent extent) -> this_t &
    {
        extent_impl(std::move(extent));
        return *this;
    }
    /** Disable automatic flush after store operation
     *
     * The returned objects of type DeferredComputation will still return a
     * buffer upon get() / operator()(), but these buffers are not guaranteed to
     * be filled until explicitly flushing.
     * By default, invoking get() / operator()() would also flush the underlying
     * Series, an MPI-collective operation. Disabling that automatic flush
     * removes the need for all ranks to invoke their handles in a consistent
     * order; the user is then responsible for calling Series::flush()
     * collectively at a suitable point.
     *
     * @return Reference to this object for chaining
     */
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
    [[nodiscard]] auto withSharedPtr(std::shared_ptr<T>)
        -> shared_ptr_return_type<T>;
    template <typename T>
    [[nodiscard]] auto withUniquePtr(UniquePtrWithLambda<T>)
        -> unique_ptr_return_type<T>;
    template <typename T, typename Del>
    [[nodiscard]] auto withUniquePtr(std::unique_ptr<T, Del>)
        -> unique_ptr_return_type<T>;
    template <typename T>
    [[nodiscard]] auto withRawPtr(T *data) -> shared_ptr_return_type<T>;
    /** Specify a contiguous container (std::vector, std::array, ...) as the
     * buffer for the operation.
     *
     * The buffer size is inferred from the container and set automatically, so
     * that the operation's extent is adjusted to the container when no explicit
     * extent is given.
     *
     * @param data Contiguous container large enough for the selected data
     * @return A buffer-specific configuration for the operation
     */
    template <typename T_ContiguousContainer>
    [[nodiscard]] auto withContiguousContainer(T_ContiguousContainer &data)
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

    /** Load the chunk data, allocating a buffer
     *
     * The returned handle performs the load, and the automatic flush of the
     * underlying Series, when invoked via get() / operator()(). That flush is
     * an MPI-collective operation, so in parallel codes every rank must invoke
     * (or explicitly destroy) its handles in a consistent order. Invoking the
     * handle is an explicit, user-controlled action and the return value is
     * marked [[nodiscard]], so handles cannot be dropped unnoticed. Use
     * unsafeNoAutomaticFlush() to defer flushing to a later explicit
     * Series::flush() instead.
     *
     * @return Deferred computation that performs the load when invoked
     */
    template <typename T>
    [[nodiscard]] auto load()
        -> auxiliary::DeferredComputation<std::shared_ptr<T>>;

    /** Type-erased version of load(). See load() for the collective semantics
     * of the automatic flush. */
    [[nodiscard]] auto loadVariant() -> auxiliary::DeferredComputation<
        auxiliary::detail::shared_ptr_dataset_types>;

    [[nodiscard]] auto getComponentHandle() const -> RecordComponent;
};

/** Configuration for storing chunks from a buffer.
 *
 * This class is used to configure a store chunk operation, where data is
 * stored from a provided buffer into a dataset.
 * This class is distinct from ConfigureLoadStoreFromBuffer, since reading
 * data does not make sense on const / unique pointer types. This way, the type
 * system will only allow read operations where they can actually run.
 */
class ConfigureStoreChunkFromBuffer : public ConfigureLoadStore
{
    friend class ConfigureLoadStore;

protected:
    // shared_ptr to make this config object copyable
    std::shared_ptr<auxiliary::WriteBuffer> m_buffer;
    Datatype m_datatype;
    std::optional<MemorySelection> m_mem_select;
    std::optional<size_t> m_buffer_size;

    ConfigureStoreChunkFromBuffer(
        auxiliary::WriteBuffer buffer, Datatype, ConfigureLoadStore &&);

    // The below methods return void.
    // For chaining calls, they should return *this, but this class right
    // here is going to be somewhere in the inheritance chain, and the final
    // class should be returned. Could be solved more elegantly with CRT,
    // but that blows up compile-time, so we make internal void functions
    // and then repeat them in the final classes.

    /** Set memory selection for non-contiguous memory regions */
    void memorySelection_impl(MemorySelection);

    auto storeChunkConfig() -> internal::LoadStoreConfigWithBuffer;

    void bufferSize_impl(size_t);

    auto getBufferSize() -> std::optional<size_t> override;

public:
    using this_t = ConfigureStoreChunkFromBuffer;

    // Configuration methods (always available)

    /** Set the offset within the dataset
     *
     * Optional. The operation will apply without offset by default (i.e. offset
     * = (0, 0, ...)).
     *
     * @param offset Offset within the dataset
     * @return Reference to this object for chaining
     */
    auto offset(Offset offset) -> this_t &
    {
        offset_impl(std::move(offset));
        return *this;
    }

    /** Set the extent within the dataset
     *
     * Optional. The operation will apply to the entire dataset by default (i.e.
     * operation extent = global dataset extent - operation offset).
     *
     * @param extent Extent within the dataset, counted from the offset
     * @return Reference to this object for chaining
     */
    auto extent(Extent extent) -> this_t &
    {
        extent_impl(std::move(extent));
        return *this;
    }

    /** Disable automatic flush after store operation
     *
     * The returned objects of type DeferredComputation will still return a
     * buffer upon get() / operator()(), but these buffers are not guaranteed to
     * be filled until explicitly flushing.
     * By default, invoking get() / operator()() would also flush the underlying
     * Series, an MPI-collective operation. Disabling that automatic flush
     * removes the need for all ranks to invoke their handles in a consistent
     * order; the user is then responsible for calling Series::flush()
     * collectively at a suitable point.
     *
     * @return Reference to this object for chaining
     */
    auto unsafeNoAutomaticFlush() -> this_t &
    {
        unsafeNoAutomaticFlush_impl();
        return *this;
    }

    /** Set memory selection for non-contiguous memory regions
     *
     * Only supported with ADIOS2 >= 2.10.1 (the capability to reset a memory
     * selection was added upstream in 2.11.0 and backported to 2.10.1). Older
     * versions cannot reset a memory selection once it has been set, which
     * would silently leak it into subsequent store operations of the same
     * variable. Those versions reject memory selections with an error at store
     * time.
     *
     * @param memorySelection Selection of memory region
     * @return Reference to this object for chaining
     */
    auto memorySelection(MemorySelection memorySelection) -> this_t &
    {
        memorySelection_impl(std::move(memorySelection));
        return *this;
    }

    /** Set the number of elements the buffer can hold
     *
     * Optional. This tells the openPMD API the size of the buffer in elements.
     * It is used to bound the operation's extent to the buffer when the extent
     * is not set explicitly: for one-dimensional datasets, an otherwise full
     * selection is shortened to the buffer size so that the buffer is not read
     * from or written to beyond its bounds.
     * It is set automatically when passing a contiguous container (see
     * withContiguousContainer()) and can be set explicitly for raw pointers
     * (see withRawPtr()), where the buffer size cannot be inferred.
     *
     * @param size Number of elements in the buffer
     * @return Reference to this object for chaining
     */
    auto bufferSize(size_t size) -> this_t &
    {
        bufferSize_impl(size);
        return *this;
    }

    // Enqueue method (deferred execution)

    /** Store the chunk data
     *
     * The returned handle performs the store, and the automatic flush of the
     * underlying Series, when invoked via get() / operator()(). That flush is
     * an MPI-collective operation, so in parallel codes every rank must invoke
     * (or explicitly destroy) its handles in a consistent order. Invoking the
     * handle is an explicit, user-controlled action and the return value is
     * marked [[nodiscard]], so handles cannot be dropped unnoticed. Use
     * unsafeNoAutomaticFlush() to defer flushing to a later explicit
     * Series::flush() instead.
     *
     * @return Deferred computation that performs the store when invoked
     */
    [[nodiscard]] auto store() -> auxiliary::DeferredComputation<void>;

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

/** Configuration for loading/storing chunks from/to a buffer.
 *
 * This class supports both loading and storing operations, allowing
 * reading data into or writing data from a provided buffer.
 */
class ConfigureLoadStoreFromBuffer : public ConfigureStoreChunkFromBuffer
{
    friend class ConfigureLoadStore;
    friend class RecordComponent;

    using ConfigureStoreChunkFromBuffer::ConfigureStoreChunkFromBuffer;

public:
    using this_t = ConfigureLoadStoreFromBuffer;

    // Configuration methods (always available)

    /** Set the offset within the dataset
     *
     * Optional. The operation will apply without offset by default (i.e. offset
     * = (0, 0, ...)).
     *
     * @param offset Offset within the dataset
     * @return Reference to this object for chaining
     */
    auto offset(Offset offset) -> this_t &
    {
        offset_impl(std::move(offset));
        return *this;
    }

    /** Set the extent within the dataset
     *
     * Optional. The operation will apply to the entire dataset by default (i.e.
     * operation extent = global dataset extent - operation offset).
     *
     * @param extent Extent within the dataset, counted from the offset
     * @return Reference to this object for chaining
     */
    auto extent(Extent extent) -> this_t &
    {
        extent_impl(std::move(extent));
        return *this;
    }

    /** Disable automatic flush after operation
     *
     * The returned objects of type DeferredComputation will still return a
     * buffer upon get() / operator()(), but these buffers are not guaranteed to
     * be filled until explicitly flushing.
     * By default, invoking get() / operator()() would also flush the underlying
     * Series, an MPI-collective operation. Disabling that automatic flush
     * removes the need for all ranks to invoke their handles in a consistent
     * order; the user is then responsible for calling Series::flush()
     * collectively at a suitable point.
     *
     * @return Reference to this object for chaining
     */
    auto unsafeNoAutomaticFlush() -> this_t &
    {
        unsafeNoAutomaticFlush_impl();
        return *this;
    }

    /** Set memory selection for non-contiguous memory regions
     *
     * Only supported with ADIOS2 >= 2.10.1 (the capability to reset a memory
     * selection was added upstream in 2.11.0 and backported to 2.10.1). Older
     * versions cannot reset a memory selection once it has been set, which
     * would silently leak it into subsequent store operations of the same
     * variable. Those versions reject memory selections with an error at store
     * time.
     *
     * @param memorySelection Selection of memory region
     * @return Reference to this object for chaining
     */
    auto memorySelection(MemorySelection memorySelection) -> this_t &
    {
        memorySelection_impl(std::move(memorySelection));
        return *this;
    }

    /** Set the number of elements the buffer can hold
     *
     * Optional. This tells the openPMD API the size of the buffer in elements.
     * It is used to bound the operation's extent to the buffer when the extent
     * is not set explicitly: for one-dimensional datasets, an otherwise full
     * selection is shortened to the buffer size so that data is not loaded
     * beyond the buffer's bounds.
     * It is set automatically when passing a contiguous container (see
     * withContiguousContainer()) and can be set explicitly for raw pointers
     * (see withRawPtr()), where the buffer size cannot be inferred.
     *
     * @param size Number of elements in the buffer
     * @return Reference to this object for chaining
     */
    auto bufferSize(size_t size) -> this_t &
    {
        bufferSize_impl(size);
        return *this;
    }

    // Enqueue method (deferred execution)

    /** Load the chunk data into the buffer
     *
     * The returned handle performs the load, and the automatic flush of the
     * underlying Series, when invoked via get() / operator()(). That flush is
     * an MPI-collective operation, so in parallel codes every rank must invoke
     * (or explicitly destroy) its handles in a consistent order. Invoking the
     * handle is an explicit, user-controlled action and the return value is
     * marked [[nodiscard]], so handles cannot be dropped unnoticed. Use
     * unsafeNoAutomaticFlush() to defer flushing to a later explicit
     * Series::flush() instead.
     *
     * @return Deferred computation that performs the load when invoked
     */
    [[nodiscard]] auto load() -> auxiliary::DeferredComputation<void>;
};

} // namespace openPMD

#include "openPMD/UndefDatatypeMacros.hpp"
// comment to prevent these includes from being moved by clang-format
#include "openPMD/LoadStoreChunk.tpp"
