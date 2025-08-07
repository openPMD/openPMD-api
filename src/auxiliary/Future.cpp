#include "openPMD/auxiliary/Future.hpp"
#include "openPMD/RecordComponent.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>

// comment

#include "openPMD/DatatypeMacros.hpp"

namespace openPMD::auxiliary
{

template <typename T>
DeferredComputation<T>::DeferredComputation(task_type task)
    : m_task([wrapped_task = std::move(task), this]() {
        if (!this->m_valid)
        {
            throw std::runtime_error(
                "[DeferredComputation] No valid state. Probably already "
                "computed.");
        }
        this->m_valid = false;
        return std::move(wrapped_task)();
    })
    , m_valid(true)
{}

template <typename T>
auto DeferredComputation<T>::get() -> T
{
    return m_task();
}

template <typename T>
auto DeferredComputation<T>::valid() const noexcept -> bool
{
    return m_valid;
}

template class DeferredComputation<void>;
template class DeferredComputation<RecordComponent::shared_ptr_dataset_types>;
// clang-format off
#define INSTANTIATE_FUTURE(dtype)                                              \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */                          \
    template class DeferredComputation<std::shared_ptr<dtype>>;
#define INSTANTIATE_FUTURE_WITH_AND_WITHOUT_EXTENT(type) INSTANTIATE_FUTURE(type) INSTANTIATE_FUTURE(type[])
OPENPMD_FOREACH_NONVECTOR_DATATYPE(INSTANTIATE_FUTURE_WITH_AND_WITHOUT_EXTENT)
#undef INSTANTIATE_FUTURE
#undef INSTANTIATE_FUTURE_WITH_AND_WITHOUT_EXTENT
// clang-format on
} // namespace openPMD::auxiliary

#include "openPMD/UndefDatatypeMacros.hpp"
