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

struct MemorySelection
{
    Offset offset;
    Extent extent;
};

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
class ConfigureStoreChunk : protected internal::ConfigureStoreChunkData
{
    friend class RecordComponent;
    template <typename>
    friend class ConfigureStoreChunk;

protected:
    ConfigureStoreChunk(RecordComponent &rc);
    ConfigureStoreChunk(ConfigureStoreChunkData &&);

    auto dim() const -> uint8_t;
    auto getOffset() const -> Offset;
    auto getExtent() const -> Extent;
    auto storeChunkConfig() const -> internal::StoreChunkConfig;

public:
    using return_type = std::conditional_t<
        std::is_void_v<ChildClass>,
        /*then*/ ConfigureStoreChunk<void>,
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
    auto enqueue() -> DynamicMemoryView<T>;
    // definition for this one is in RecordComponent.tpp since it needs the
    // definition of class RecordComponent.
    template <typename T, typename F>
    auto enqueue(F &&createBuffer) -> DynamicMemoryView<T>;
};

template <typename Ptr_Type>
class ConfigureStoreChunkFromBuffer
    : public ConfigureStoreChunk<ConfigureStoreChunkFromBuffer<Ptr_Type>>
{
public:
    using parent_t =
        ConfigureStoreChunk<ConfigureStoreChunkFromBuffer<Ptr_Type>>;

private:
    template <typename T>
    friend class ConfigureStoreChunk;

    Ptr_Type m_buffer;
    std::optional<MemorySelection> m_mem_select;

    ConfigureStoreChunkFromBuffer(Ptr_Type buffer, parent_t &&);

    auto storeChunkConfig() const -> internal::StoreChunkConfigFromBuffer;

public:
    auto memorySelection(MemorySelection) & -> ConfigureStoreChunkFromBuffer &;

    auto as_parent() && -> parent_t &&;
    auto as_parent() & -> parent_t &;
    auto as_parent() const & -> parent_t const &;

    auto enqueue() -> void;
};

template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::fromSharedPtr(std::shared_ptr<T> data)
    -> ConfigureStoreChunkFromBuffer<
        std::shared_ptr<normalize_dataset_type<T> const>>
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return ConfigureStoreChunkFromBuffer<
        std::shared_ptr<normalize_dataset_type<T> const>>(
        std::static_pointer_cast<normalize_dataset_type<T> const>(
            std::move(data)),
        {std::move(*this)});
}
template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::fromUniquePtr(UniquePtrWithLambda<T> data)
    -> ConfigureStoreChunkFromBuffer<
        UniquePtrWithLambda<normalize_dataset_type<T>>>
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return ConfigureStoreChunkFromBuffer<
        UniquePtrWithLambda<normalize_dataset_type<T>>>(
        std::move(data).template static_cast_<normalize_dataset_type<T>>(),
        {std::move(*this)});
}
template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::fromRawPtr(T *data)
    -> ConfigureStoreChunkFromBuffer<
        std::shared_ptr<normalize_dataset_type<T> const>>
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return ConfigureStoreChunkFromBuffer<
        std::shared_ptr<normalize_dataset_type<T> const>>(
        auxiliary::shareRaw(data), {std::move(*this)});
}

template <typename ChildClass>
template <typename T, typename Del>
auto ConfigureStoreChunk<ChildClass>::fromUniquePtr(
    std::unique_ptr<T, Del> data)
    -> ConfigureStoreChunkFromBuffer<
        UniquePtrWithLambda<normalize_dataset_type<T>>>
{
    return fromUniquePtr(UniquePtrWithLambda<T>(std::move(data)));
}
template <typename ChildClass>
template <typename T_ContiguousContainer>
auto ConfigureStoreChunk<ChildClass>::fromContiguousContainer(
    T_ContiguousContainer &data) ->
    typename std::enable_if_t<
        auxiliary::IsContiguousContainer_v<T_ContiguousContainer>,
        ConfigureStoreChunkFromBuffer<std::shared_ptr<normalize_dataset_type<
            typename T_ContiguousContainer::value_type> const>>>
{
    if (!m_extent.has_value() && dim() == 1)
    {
        m_extent = Extent{data.size()};
    }
    return fromRawPtr(data.data());
}
} // namespace openPMD
