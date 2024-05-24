#pragma once

#include "openPMD/Dataset.hpp"
#include "openPMD/auxiliary/ShareRawInternal.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"

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

    struct ConfigureLoadStoreData
    {
        ConfigureLoadStoreData(RecordComponent &);

        RecordComponent &m_rc;
        std::optional<Offset> m_offset;
        std::optional<Extent> m_extent;
    };
} // namespace internal

template <typename ChildClass = void>
class ConfigureLoadStore : protected internal::ConfigureLoadStoreData
{
    friend class RecordComponent;
    template <typename>
    friend class ConfigureLoadStore;

protected:
    ConfigureLoadStore(RecordComponent &rc);
    ConfigureLoadStore(ConfigureLoadStoreData &&);

    auto dim() const -> uint8_t;
    auto getOffset() -> Offset const &;
    auto getExtent() -> Extent const &;
    auto storeChunkConfig() -> internal::LoadStoreConfig;

public:
    using return_type = std::conditional_t<
        std::is_void_v<ChildClass>,
        /*then*/ ConfigureLoadStore<void>,
        /*else*/ ChildClass>;

    auto offset(Offset) -> return_type &;
    auto extent(Extent) -> return_type &;

    template <typename T>
    struct shared_ptr_return_type_impl
    {
        using type = ConfigureLoadStoreFromBuffer<
            std::shared_ptr<std::remove_extent_t<T>>>;
    };
    template <typename T>
    struct shared_ptr_return_type_impl<T const>
    {
        using type =
            ConfigureStoreChunkFromBuffer<std::shared_ptr<T const>, void>;
    };
    template <typename T>
    struct shared_ptr_return_type_impl<T const[]>
    {
        using type =
            ConfigureStoreChunkFromBuffer<std::shared_ptr<T const>, void>;
    };

    template <typename T>
    using shared_ptr_return_type =
        typename shared_ptr_return_type_impl<T>::type;

    template <typename T>
    using normalize_dataset_type = std::remove_cv_t<std::remove_extent_t<T>>;
    template <typename T>
    using unique_ptr_return_type = ConfigureStoreChunkFromBuffer<
        UniquePtrWithLambda<normalize_dataset_type<T>>,
        void>;

    // @todo rvalue references..?
    template <typename T>
    auto fromSharedPtr(std::shared_ptr<T>) -> shared_ptr_return_type<T>;
    template <typename T>
    auto fromUniquePtr(UniquePtrWithLambda<T>) -> unique_ptr_return_type<T>;
    template <typename T, typename Del>
    auto fromUniquePtr(std::unique_ptr<T, Del>) -> unique_ptr_return_type<T>;
    template <typename T>
    auto fromRawPtr(T *data) -> shared_ptr_return_type<T>;
    template <typename T_ContiguousContainer>
    auto fromContiguousContainer(T_ContiguousContainer &data)
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
    [[nodiscard]] auto enqueueLoad() -> std::shared_ptr<T>;

    using shared_ptr_dataset_types = auxiliary::detail::
        map_variant<auxiliary::detail::as_shared_pointer, dataset_types>::type;
    [[nodiscard]] auto enqueueLoadVariant() -> shared_ptr_dataset_types;
};

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
    template <typename T>
    friend class ConfigureLoadStore;

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
    template <typename X = void>
    auto enqueueLoad()
    {
        static_assert(
            auxiliary::dependent_false_v<X>,
            "Cannot load chunk data into a buffer that is const or a "
            "unique_ptr.");
    }
};

template <typename Ptr_Type>
class ConfigureLoadStoreFromBuffer
    : public ConfigureStoreChunkFromBuffer<
          Ptr_Type,
          ConfigureLoadStoreFromBuffer<Ptr_Type>>
{
    using parent_t = ConfigureStoreChunkFromBuffer<
        Ptr_Type,
        ConfigureLoadStoreFromBuffer<Ptr_Type>>;
    template <typename>
    friend class ConfigureLoadStore;
    ConfigureLoadStoreFromBuffer(
        Ptr_Type buffer, typename parent_t::parent_t &&);

public:
    auto enqueueLoad() -> void;
};
} // namespace openPMD

#include "openPMD/LoadStoreChunk.tpp"
