#include "openPMD/auxiliary/Future.hpp"
#include "openPMD/RecordComponent.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>

// comment

#include "openPMD/DatatypeMacros.hpp"

namespace openPMD::auxiliary::detail
{
template <typename T>
auto OneTimeTask<T>::operator()() -> T
{
    if (!this->m_task_valid)
    {
        throw std::runtime_error(
            "[DeferredComputation] No valid state. Probably already "
            "computed.");
    }
    if (!this->m_task)
    {
        throw std::runtime_error(
            "[DeferredComputation] No valid task was specified.");
    }
    this->m_task_valid = false;
    if constexpr (std::is_void_v<T>)
    {
        std::move(this->m_task)();
        this->m_task = {};
    }
    else
    {
        auto res = std::move(this->m_task)();
        this->m_task = {}; // reset
        return res;
    }
}
} // namespace openPMD::auxiliary::detail

namespace openPMD::auxiliary
{

template <typename T>
DeferredComputation<T>::DeferredComputation(task_type task)
    : m_task(detail::OneTimeTask<T>{std::move(task)})
{}

template <typename T>
DeferredComputation<T>::DeferredComputation(cached_type cached_val)
    : m_task(detail::CachedValue<T>{std::move(cached_val)})
{}

template <typename T>
DeferredComputation<T>::~DeferredComputation()
{
    try
    {
        std::visit(
            auxiliary::overloaded{
                [](detail::OneTimeTask<T> &task) {
                    if (task.m_task_valid)
                    {
                        std::move(task)();
                    }
                },
                [](detail::CachedValue<T> &) {}},
            this->m_task);
    }
    catch (std::exception const &e)
    {
        std::cerr << "[DeferredComputation] Error in destructor: '" << e.what()
                  << "'." << std::endl;
    }
    catch (...)
    {
        std::cerr << "[DeferredComputation] Unknown error in destructor."
                  << std::endl;
    }
}

template <typename T>
auto DeferredComputation<T>::get() -> T
{
    return std::visit(
        auxiliary::overloaded{
            [](detail::OneTimeTask<T> &task) -> T { return std::move(task)(); },
            [](detail::CachedValue<T> &cached) -> T {
                if constexpr (std::is_void_v<T>)
                {
                    return;
                }
                else
                {
                    return cached.val;
                }
            }},
        this->m_task);
}

template <typename T>
auto DeferredComputation<T>::operator()() -> T
{
    return get();
}

template <typename T>
void DeferredComputation<T>::forget() &&
{
    std::visit(
        auxiliary::overloaded{
            [](detail::OneTimeTask<T> &task) {
                task.m_task = {};
                task.m_task_valid = false;
            },
            [](detail::CachedValue<T> const &) {}},
        this->m_task);
}

template <typename T>
auto DeferredComputation<T>::valid() const noexcept -> bool
{
    return std::visit(
        auxiliary::overloaded{
            [](detail::OneTimeTask<T> const &task) {
                return task.m_task_valid;
            },
            [](detail::CachedValue<T> const &) { return true; }},
        this->m_task);
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
