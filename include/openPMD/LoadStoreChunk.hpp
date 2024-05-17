#pragma once

#include "openPMD/Dataset.hpp"
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
} // namespace internal

template <typename ChildClass = void>
class ConfigureStoreChunk
{
    friend class RecordComponent;

private:
    RecordComponent &m_rc;
    std::optional<Offset> m_offset;
    std::optional<Extent> m_extent;

    ConfigureStoreChunk(RecordComponent &rc);

    auto dim() const -> uint8_t;
    auto getOffset() const -> Offset;
    auto getExtent() const -> Extent;
    auto storeChunkConfig() const -> internal::StoreChunkConfig;

public:
    using return_type = std::conditional_t<
        std::is_void_v<ChildClass>,
        /*then*/ ConfigureStoreChunk<void>,
        /*else*/ ChildClass>;

    auto offset(Offset) -> return_type &;
    auto extent(Extent) -> return_type &;

    template <typename T>
    auto fromSharedPtr(
        std::shared_ptr<T>) && -> TypedConfigureStoreChunk<std::shared_ptr<T>>;
    template <typename T>
    auto fromUniquePtr(UniquePtrWithLambda<T>)
        && -> TypedConfigureStoreChunk<UniquePtrWithLambda<T>>;
    template <typename T, typename Del>
    auto fromUniquePtr(std::unique_ptr<T, Del>)
        && -> TypedConfigureStoreChunk<UniquePtrWithLambda<T>>;
    template <typename T>
    auto fromRawPtr(T *data) && -> TypedConfigureStoreChunk<std::shared_ptr<T>>;
    template <typename T_ContiguousContainer>
    auto fromContiguousContainer(T_ContiguousContainer &data) && ->
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
    friend class ConfigureStoreChunk<TypedConfigureStoreChunk<Ptr_Type>>;

    Ptr_Type m_buffer;
    std::optional<MemorySelection> m_mem_select;

    TypedConfigureStoreChunk(Ptr_Type buffer, parent_t &&);

    auto storeChunkConfig() const -> internal::StoreChunkConfigFromBuffer;

public:
    auto memorySelection(MemorySelection) & -> TypedConfigureStoreChunk &;

    auto as_parent() && -> parent_t &&;
    auto as_parent() & -> parent_t &;
    auto as_parent() const & -> parent_t const &;

    auto enqueue() & -> void;
};

template <typename ChildClass>
template <typename T, typename Del>
auto ConfigureStoreChunk<ChildClass>::fromUniquePtr(
    std::unique_ptr<T, Del> data)
    && -> TypedConfigureStoreChunk<UniquePtrWithLambda<T>>
{
    return fromUniquePtr(UniquePtrWithLambda<T>(std::move(data)));
}
template <typename ChildClass>
template <typename T_ContiguousContainer>
auto ConfigureStoreChunk<ChildClass>::fromContiguousContainer(
    T_ContiguousContainer &data) && ->
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
