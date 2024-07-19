#pragma once

#include "openPMD/Dataset.hpp"
#include "openPMD/auxiliary/ShareRawInternal.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"

#include <future>
#include <optional>
#include <stdexcept>
#include <type_traits>

namespace openPMD
{
class RecordComponent;
template <typename Ptr_Type, typename ChildClass>
class ConfigureStoreChunkFromBuffer;
template <typename Ptr_Type>
class ConfigureLoadStoreFromBuffer;
template <typename T>
class DynamicMemoryView;

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
    using future_to_shared_ptr_dataset_types =
        map_variant<as_shared_pointer, dataset_types>::type;
} // namespace auxiliary::detail

enum class EnqueuePolicy
{
    Defer,
    Immediate
};

/*
 * Actual data members of `ConfigureLoadStore<>` and methods that don't depend
 * on the ChildClass template parameter. By extracting the members to this
 * struct, we can pass them around between different instances of the class
 * template. Numbers of method instantiations can be reduced.
 */
struct ConfigureLoadStoreCore
{
    ConfigureLoadStoreCore(RecordComponent &);

    RecordComponent &m_rc;
    std::optional<Offset> m_offset;
    std::optional<Extent> m_extent;

protected:
    [[nodiscard]] auto dim() const -> uint8_t;
    auto getOffset() -> Offset const &;
    auto getExtent() -> Extent const &;
    auto storeChunkConfig() -> internal::LoadStoreConfig;

public:
    /*
     * If the type is non-const, then the return type should be
     * ConfigureLoadStoreFromBuffer<>, ...
     */
    template <typename T>
    struct shared_ptr_return_type_impl
    {
        using type = ConfigureLoadStoreFromBuffer<std::shared_ptr<T>>;
    };
    /*
     * ..., but if it is a const type, Load operations make no sense, so the
     * return type should be ConfigureStoreChunkFromBuffer<>.
     */
    template <typename T>
    struct shared_ptr_return_type_impl<T const>
    {
        using type =
            ConfigureStoreChunkFromBuffer<std::shared_ptr<T const>, void>;
    };

    template <typename T>
    using shared_ptr_return_type =
        typename shared_ptr_return_type_impl<std::remove_extent_t<T>>::type;

    /*
     * As loading into unique pointer types makes no sense, the case is simpler
     * for unique pointers. Just remove the array extents here.
     */
    template <typename T>
    using unique_ptr_return_type = ConfigureStoreChunkFromBuffer<
        UniquePtrWithLambda<std::remove_extent_t<T>>,
        void>;

    // @todo rvalue references..?
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

    template <typename T>
    [[nodiscard]] auto enqueueStore() -> DynamicMemoryView<T>;
    // definition for this one is in RecordComponent.tpp since it needs the
    // definition of class RecordComponent.
    template <typename T, typename F>
    [[nodiscard]] auto enqueueStore(F &&createBuffer) -> DynamicMemoryView<T>;

    template <typename T>
    [[nodiscard]] auto enqueueLoad() -> std::future<std::shared_ptr<T>>;

    template <typename T>
    [[nodiscard]] auto load(EnqueuePolicy) -> std::shared_ptr<T>;

    [[nodiscard]] auto enqueueLoadVariant()
        -> std::future<auxiliary::detail::future_to_shared_ptr_dataset_types>;
};

/** Basic configuration for a Load/Store operation.
 *
 * @tparam ChildClass CRT pattern.
 *         The purpose is that in child classes `return *this` should return
 *         an instance of the child class, not of ConfigureLoadStore.
 *         Instantiate with void when using without subclass.
 */
template <typename ChildClass = void>
class ConfigureLoadStore : public ConfigureLoadStoreCore
{
    friend class RecordComponent;
    friend struct ConfigureLoadStoreCore;

protected:
    ConfigureLoadStore(RecordComponent &rc);
    ConfigureLoadStore(ConfigureLoadStoreCore &&);

public:
    using return_type = std::conditional_t<
        std::is_void_v<ChildClass>,
        /*then*/ ConfigureLoadStore<void>,
        /*else*/ ChildClass>;

    auto offset(Offset) -> return_type &;
    auto extent(Extent) -> return_type &;
};

/** Configuration for a Store operation with a buffer type.
 *
 * This class does intentionally not support Load operations since there are
 * pointer types (const pointers, unique pointers) where Load operations make no
 * sense. See the \ref ConfigureLoadStoreFromBuffer class template for both
 * Load/Store operations.
 *
 * @tparam Ptr_Type The type of pointer used internally.
 * @tparam ChildClass CRT pattern.
 *         The purpose is that in child classes `return *this` should return
 *         an instance of the child class, not of ConfigureStoreChunkFromBuffer.
 *         Instantiate with void when using without subclass.
 */
template <typename Ptr_Type, typename ChildClass = void>
class ConfigureStoreChunkFromBuffer
    : public ConfigureLoadStore<std::conditional_t<
          std::is_void_v<ChildClass>,
          /*then*/ ConfigureStoreChunkFromBuffer<Ptr_Type, void>,
          /*else*/ ChildClass>>
{
public:
    using return_type = std::conditional_t<
        std::is_void_v<ChildClass>,
        /*then*/ ConfigureStoreChunkFromBuffer<Ptr_Type, void>,
        /*else*/ ChildClass>;
    using parent_t = ConfigureLoadStore<return_type>;

protected:
    friend struct ConfigureLoadStoreCore;

    Ptr_Type m_buffer;
    std::optional<MemorySelection> m_mem_select;

    auto storeChunkConfig() -> internal::LoadStoreConfigWithBuffer;

protected:
    ConfigureStoreChunkFromBuffer(Ptr_Type buffer, parent_t &&);

public:
    auto memorySelection(MemorySelection) -> return_type &;

    auto as_parent() && -> parent_t &&;
    auto as_parent() & -> parent_t &;
    auto as_parent() const & -> parent_t const &;

    auto enqueueStore() -> void;

    /** This intentionally shadows the parent class's enqueueLoad method in
     * order to show a compile error when using enqueueLoad() on an object of
     * this class. The parent method can still be accessed through as_parent()
     * if needed.
     */
    template <typename X = void>
    auto enqueueLoad()
    {
        static_assert(
            auxiliary::dependent_false_v<X>,
            "Cannot load chunk data into a buffer that is const or a "
            "unique_ptr.");
    }
};

/** Configuration for a Load/Store operation with a buffer type.
 *
 * Only instantiated for pointer types where Load operations make sense (e.g. no
 * const pointers and no unique pointers).
 * \ref ConfigureStoreChunkFromBuffer is used otherwise.
 *
 * @tparam Ptr_Type The type of pointer used internally.
 */
template <typename Ptr_Type>
class ConfigureLoadStoreFromBuffer
    : public ConfigureStoreChunkFromBuffer<
          Ptr_Type,
          ConfigureLoadStoreFromBuffer<Ptr_Type>>
{
    using parent_t = ConfigureStoreChunkFromBuffer<
        Ptr_Type,
        ConfigureLoadStoreFromBuffer<Ptr_Type>>;
    friend struct ConfigureLoadStoreCore;
    ConfigureLoadStoreFromBuffer(
        Ptr_Type buffer, typename parent_t::parent_t &&);

public:
    auto enqueueLoad() -> void;

    auto load(EnqueuePolicy) -> void;
};
} // namespace openPMD

#include "openPMD/LoadStoreChunk.tpp"
