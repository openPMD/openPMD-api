#pragma once

#include "openPMD/Dataset.hpp"
#include "openPMD/Span.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"

#include <optional>
#include <type_traits>

namespace openPMD
{
class RecordComponent;
template <typename Ptr_Type>
class TypedConfigureStoreChunk;

struct MemorySelection
{
    Offset offset;
    Extent extent;
};

template <typename ChildClass = void>
class ConfigureStoreChunk
{
private:
    RecordComponent &m_rc;
    Offset m_offset;
    Extent m_extent;
    std::optional<MemorySelection> m_mem_select;

    ConfigureStoreChunk(RecordComponent &rc);

public:
    using return_type = std::conditional_t<
        std::is_void_v<ChildClass>,
        /*then*/ ConfigureStoreChunk<void>,
        /*else*/ ChildClass>;

    auto offset(Offset) && -> return_type &&;
    auto extent(Extent) && -> return_type &&;
    auto memorySelection(MemorySelection) && -> return_type &&;

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
    auto enqueue() && -> DynamicMemoryView<T>;
};

template <typename Ptr_Type>
class TypedConfigureStoreChunk
    : public ConfigureStoreChunk<TypedConfigureStoreChunk<Ptr_Type>>
{
private:
    Ptr_Type m_buffer;

public:
    using parent_t = ConfigureStoreChunk<TypedConfigureStoreChunk<Ptr_Type>>;

    auto as_parent() && -> parent_t &&;
    auto as_parent() & -> parent_t &;
    auto as_parent() const & -> parent_t const &;

    auto enqueue() && -> void;
};
} // namespace openPMD
