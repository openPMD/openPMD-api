

#include "openPMD/LoadStoreChunk.hpp"
#include "openPMD/DatatypeMacros.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"
#include <stdexcept>

namespace openPMD
{
template <typename ChildClass>
ConfigureStoreChunk<ChildClass>::ConfigureStoreChunk(RecordComponent &rc)
    : m_rc(rc), m_offset(rc.getDimensionality(), 0), m_extent{rc.getExtent()}
{}

template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::enqueue() && -> DynamicMemoryView<T>
{
    throw std::runtime_error("Unimplemented!");
}

template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::fromSharedPtr(
    std::shared_ptr<T>) && -> TypedConfigureStoreChunk<std::shared_ptr<T>>
{
    throw std::runtime_error("Unimplemented!");
}
template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::fromUniquePtr(UniquePtrWithLambda<T>)
    && -> TypedConfigureStoreChunk<UniquePtrWithLambda<T>>
{
    throw std::runtime_error("Unimplemented!");
}
template <typename ChildClass>
template <typename T>
auto ConfigureStoreChunk<ChildClass>::fromRawPtr(
    T *) && -> TypedConfigureStoreChunk<std::shared_ptr<T>>
{
    throw std::runtime_error("Unimplemented!");
}

#define INSTANTIATE_METHOD_TEMPLATES(base_class, dtype)                        \
    template auto base_class::enqueue() && -> DynamicMemoryView<dtype>;        \
    template auto base_class::fromSharedPtr(std::shared_ptr<dtype>)            \
        && -> TypedConfigureStoreChunk<std::shared_ptr<dtype>>;                \
    template auto base_class::fromUniquePtr(UniquePtrWithLambda<dtype>)        \
        && -> TypedConfigureStoreChunk<UniquePtrWithLambda<dtype>>;            \
    template auto base_class::fromRawPtr(                                      \
        dtype *) && -> TypedConfigureStoreChunk<std::shared_ptr<dtype>>;

template class ConfigureStoreChunk<void>;

#define INSTANTIATE_METHOD_TEMPLATES_FOR_BASE(dtype)                           \
    INSTANTIATE_METHOD_TEMPLATES(ConfigureStoreChunk<void>, dtype)

OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_METHOD_TEMPLATES_FOR_BASE)

#undef INSTANTIATE_METHOD_TEMPLATES_FOR_BASE

#define INSTANTIATE_TYPED_STORE_CHUNK(dtype)                                   \
    template class TypedConfigureStoreChunk<std::shared_ptr<dtype>>;           \
    INSTANTIATE_METHOD_TEMPLATES(                                              \
        ConfigureStoreChunk<TypedConfigureStoreChunk<std::shared_ptr<dtype>>>, \
        dtype)                                                                 \
    template class TypedConfigureStoreChunk<UniquePtrWithLambda<dtype>>;       \
    INSTANTIATE_METHOD_TEMPLATES(                                              \
        ConfigureStoreChunk<                                                   \
            TypedConfigureStoreChunk<UniquePtrWithLambda<dtype>>>,             \
        dtype)

OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_TYPED_STORE_CHUNK)

#undef INSTANTIATE_TYPED_STORE_CHUNK
#undef INSTANTIATE_METHOD_TEMPLATES

} // namespace openPMD
