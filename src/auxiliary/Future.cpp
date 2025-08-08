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

// need this for clang-tidy
#define OPENPMD_ARRAY(type) type[]
#define OPENPMD_APPLY_TEMPLATE(template_, type) template_<type>

#define INSTANTIATE_FUTURE(dtype)                                              \
    template class DeferredComputation<OPENPMD_APPLY_TEMPLATE(                 \
        std::shared_ptr, dtype)>;
#define INSTANTIATE_FUTURE_WITH_AND_WITHOUT_EXTENT(type)                       \
    INSTANTIATE_FUTURE(type) INSTANTIATE_FUTURE(OPENPMD_ARRAY(type))
OPENPMD_FOREACH_NONVECTOR_DATATYPE(INSTANTIATE_FUTURE_WITH_AND_WITHOUT_EXTENT)
#undef INSTANTIATE_FUTURE
#undef INSTANTIATE_FUTURE_WITH_AND_WITHOUT_EXTENT
#undef OPENPMD_ARRAY
#undef OPENPMD_APPLY_TEMPLATE
} // namespace openPMD::auxiliary

#include "openPMD/UndefDatatypeMacros.hpp"
