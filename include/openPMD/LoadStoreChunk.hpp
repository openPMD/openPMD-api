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
template <typename Ptr_Type>
class ConfigureStoreChunkFromBuffer;
template <typename T>
class DynamicMemoryView;

namespace internal
{
    struct StoreChunkConfig
    {
        Offset offset;
        Extent extent;
    };
    struct StoreChunkConfigFromBuffer
    {
        Offset offset;
        Extent extent;
        std::optional<MemorySelection> memorySelection;
    };

    struct ConfigureStoreChunkData
    {
        ConfigureStoreChunkData(RecordComponent &);

        RecordComponent &m_rc;
        std::optional<Offset> m_offset;
        std::optional<Extent> m_extent;
    };
} // namespace internal

template <typename ChildClass = void>
class ConfigureLoadStore : protected internal::ConfigureStoreChunkData
{
    friend class RecordComponent;
    template <typename>
    friend class ConfigureLoadStore;

protected:
    ConfigureLoadStore(RecordComponent &rc);
    ConfigureLoadStore(ConfigureStoreChunkData &&);

    auto dim() const -> uint8_t;
    auto getOffset() const -> Offset;
    auto getExtent() const -> Extent;
    auto storeChunkConfig() const -> internal::StoreChunkConfig;

public:
    using return_type = std::conditional_t<
        std::is_void_v<ChildClass>,
        /*then*/ ConfigureLoadStore<void>,
        /*else*/ ChildClass>;
    template <typename T>
    using normalize_dataset_type = std::remove_cv_t<std::remove_extent_t<T>>;

    auto offset(Offset) -> return_type &;
    auto extent(Extent) -> return_type &;

    // @todo rvalue references..?
    template <typename T>
    auto fromSharedPtr(std::shared_ptr<T>)
        -> ConfigureStoreChunkFromBuffer<
            std::shared_ptr<normalize_dataset_type<T> const>>;
    template <typename T>
    auto fromUniquePtr(UniquePtrWithLambda<T>)
        -> ConfigureStoreChunkFromBuffer<
            UniquePtrWithLambda<normalize_dataset_type<T>>>;
    template <typename T, typename Del>
    auto fromUniquePtr(std::unique_ptr<T, Del>)
        -> ConfigureStoreChunkFromBuffer<
            UniquePtrWithLambda<normalize_dataset_type<T>>>;
    template <typename T>
    auto
    fromRawPtr(T *data) -> ConfigureStoreChunkFromBuffer<
                            std::shared_ptr<normalize_dataset_type<T> const>>;
    template <typename T_ContiguousContainer>
    auto fromContiguousContainer(T_ContiguousContainer &data) ->
        typename std::enable_if_t<
            auxiliary::IsContiguousContainer_v<T_ContiguousContainer>,
            ConfigureStoreChunkFromBuffer<
                std::shared_ptr<normalize_dataset_type<
                    typename T_ContiguousContainer::value_type> const>>>;

    template <typename T>
    [[nodiscard]] auto enqueueStore() -> DynamicMemoryView<T>;
    // definition for this one is in RecordComponent.tpp since it needs the
    // definition of class RecordComponent.
    template <typename T, typename F>
    [[nodiscard]] auto enqueueStore(F &&createBuffer) -> DynamicMemoryView<T>;
};

template <typename Ptr_Type>
class ConfigureStoreChunkFromBuffer
    : public ConfigureLoadStore<ConfigureStoreChunkFromBuffer<Ptr_Type>>
{
public:
    using parent_t =
        ConfigureLoadStore<ConfigureStoreChunkFromBuffer<Ptr_Type>>;

private:
    template <typename T>
    friend class ConfigureLoadStore;

    Ptr_Type m_buffer;
    std::optional<MemorySelection> m_mem_select;

    ConfigureStoreChunkFromBuffer(Ptr_Type buffer, parent_t &&);

    auto storeChunkConfig() const -> internal::StoreChunkConfigFromBuffer;

public:
    auto memorySelection(MemorySelection) -> ConfigureStoreChunkFromBuffer &;

    auto as_parent() && -> parent_t &&;
    auto as_parent() & -> parent_t &;
    auto as_parent() const & -> parent_t const &;

    auto enqueueStore() -> void;
};
} // namespace openPMD

#include "openPMD/LoadStoreChunk.tpp"
