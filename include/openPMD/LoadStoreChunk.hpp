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
class TypedConfigureStoreChunk;
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
    using normalize_dataset_type =
        std::remove_cv_t<std::remove_extent_t<T>> const;

    auto offset(Offset) -> return_type &;
    auto extent(Extent) -> return_type &;

    // @todo rvalue references..?
    template <typename T>
    auto fromSharedPtr(std::shared_ptr<T>)
        -> TypedConfigureStoreChunk<std::shared_ptr<normalize_dataset_type<T>>>;
    template <typename T>
    auto fromUniquePtr(UniquePtrWithLambda<T>)
        -> TypedConfigureStoreChunk<
            UniquePtrWithLambda<normalize_dataset_type<T>>>;
    template <typename T, typename Del>
    auto fromUniquePtr(std::unique_ptr<T, Del>)
        -> TypedConfigureStoreChunk<
            UniquePtrWithLambda<normalize_dataset_type<T>>>;
    template <typename T>
    auto fromRawPtr(T *data) -> TypedConfigureStoreChunk<std::shared_ptr<T>>;
    template <typename T_ContiguousContainer>
    auto fromContiguousContainer(T_ContiguousContainer &data) ->
        typename std::enable_if_t<
            auxiliary::IsContiguousContainer_v<T_ContiguousContainer>,
            TypedConfigureStoreChunk<
                std::shared_ptr<typename T_ContiguousContainer::value_type>>>;

    template <typename T>
    auto enqueue() -> DynamicMemoryView<T>;
    // definition for this one is in RecordComponent.tpp since it needs the
    // definition of class RecordComponent.
    template <typename T, typename F>
    auto enqueue(F &&createBuffer) -> DynamicMemoryView<T>;
};

// @todo rename as ConfigureStoreChunkFromBuffer
template <typename Ptr_Type>
class TypedConfigureStoreChunk
    : public ConfigureStoreChunk<TypedConfigureStoreChunk<Ptr_Type>>
{
public:
    using parent_t = ConfigureStoreChunk<TypedConfigureStoreChunk<Ptr_Type>>;

private:
    template <typename T>
    friend class ConfigureStoreChunk;

    Ptr_Type m_buffer;
    std::optional<MemorySelection> m_mem_select;

    TypedConfigureStoreChunk(Ptr_Type buffer, parent_t &&);

    auto storeChunkConfig() const -> internal::StoreChunkConfigFromBuffer;

public:
    auto memorySelection(MemorySelection) & -> TypedConfigureStoreChunk &;

    auto as_parent() && -> parent_t &&;
    auto as_parent() & -> parent_t &;
    auto as_parent() const & -> parent_t const &;

    auto enqueue() -> void;
};

template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::fromSharedPtr(std::shared_ptr<T> data)
    -> TypedConfigureStoreChunk<std::shared_ptr<normalize_dataset_type<T>>>
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return TypedConfigureStoreChunk<std::shared_ptr<normalize_dataset_type<T>>>(
        std::static_pointer_cast<normalize_dataset_type<T>>(std::move(data)),
        {std::move(*this)});
}
template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::fromUniquePtr(UniquePtrWithLambda<T> data)
    -> TypedConfigureStoreChunk<UniquePtrWithLambda<normalize_dataset_type<T>>>
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return TypedConfigureStoreChunk<
        UniquePtrWithLambda<normalize_dataset_type<T>>>(
        std::move(data).template static_cast_<normalize_dataset_type<T>>(),
        {std::move(*this)});
}
template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::fromRawPtr(T *data)
    -> TypedConfigureStoreChunk<std::shared_ptr<T>>
{
    if (!data)
    {
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    }
    return TypedConfigureStoreChunk<std::shared_ptr<T>>(
        auxiliary::shareRaw(data), {std::move(*this)});
}

template <typename ChildClass>
template <typename T, typename Del>
auto ConfigureStoreChunk<ChildClass>::fromUniquePtr(
    std::unique_ptr<T, Del> data)
    -> TypedConfigureStoreChunk<UniquePtrWithLambda<normalize_dataset_type<T>>>
{
    return fromUniquePtr(UniquePtrWithLambda<T>(std::move(data)));
}
template <typename ChildClass>
template <typename T_ContiguousContainer>
auto ConfigureStoreChunk<ChildClass>::fromContiguousContainer(
    T_ContiguousContainer &data) ->
    typename std::enable_if_t<
        auxiliary::IsContiguousContainer_v<T_ContiguousContainer>,
        TypedConfigureStoreChunk<
            std::shared_ptr<typename T_ContiguousContainer::value_type>>>
{
    if (!m_extent.has_value() && dim() == 1)
    {
        m_extent = Extent{data.size()};
    }
    return fromRawPtr(data.data());
}
} // namespace openPMD
