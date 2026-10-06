#include "openPMD/auxiliary/Future.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/RecordComponent.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>

// comment

#include "openPMD/DatatypeMacros.hpp"

namespace openPMD::auxiliary::detail
{
template <typename T>
OneTimeTask<T>::OneTimeTask() = default;

template <typename T>
OneTimeTask<T>::OneTimeTask(task_type task) : members{std::move(task)}
{}

template <typename T>
OneTimeTask<T>::OneTimeTask(OneTimeTask &&other) noexcept(noexcept_move)
    : members(std::move(other.members))
{
    other.members.m_task_valid = false;
}

template <typename T>
auto OneTimeTask<T>::operator=(OneTimeTask &&other) noexcept(noexcept_move)
    -> OneTimeTask &
{
    this->members = std::move(other.members);
    other.members.m_task_valid = false;
    return *this;
}

template <typename T>
auto OneTimeTask<T>::operator()() -> T
{
    if (!members.m_task_valid)
    {
        throw error::WrongAPIUsage(
            "[DeferredComputation] No valid state. Probably already "
            "computed.");
    }
    if (!members.m_task)
    {
        throw error::WrongAPIUsage(
            "[DeferredComputation] No valid task was specified.");
    }
    members.m_task_valid = false;
    if constexpr (std::is_void_v<T>)
    {
        std::move(members.m_task)();
        members.m_task = {};
    }
    else
    {
        auto res = std::move(members.m_task)();
        members.m_task = {}; // reset
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
DeferredComputation<T>::DeferredComputation() = default;

template <typename T>
DeferredComputation<T>::DeferredComputation(DeferredComputation &&) noexcept(
    noexcept_move) = default;

template <typename T>
auto DeferredComputation<T>::operator=(DeferredComputation &&other) noexcept(
    noexcept_move) -> DeferredComputation &
{
    if (this == &other)
    {
        return *this;
    }
    /*
     * The target of the assignment might still hold a pending task. As with
     * the destructor, that task represents a data operation (and possibly a
     * collective flush) that must not be silently dropped, so run it before
     * overwriting it.
     */
    this->executeIfValid();
    this->m_task = std::move(other.m_task);
    return *this;
}

template <typename T>
void DeferredComputation<T>::executeIfValid() noexcept
{
    try
    {
        std::visit(
            auxiliary::overloaded{
                [](detail::OneTimeTask<T> &task) {
                    if (task.members.m_task_valid && task.members.m_task)
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
DeferredComputation<T>::~DeferredComputation()
{
    this->executeIfValid();
}

template <typename T>
auto DeferredComputation<T>::get() -> T
{
    return std::visit(
        auxiliary::overloaded{
            [](detail::OneTimeTask<T> &task) -> T { return std::move(task)(); },
            [](detail::CachedValue<T> &cached) -> T { return cached.val; }},
        this->m_task);
}

template <>
auto DeferredComputation<void>::get() -> void
{
    std::visit(
        auxiliary::overloaded{
            [](detail::OneTimeTask<void> &task) { std::move(task)(); },
            [](detail::CachedValue<void> &) { return; }},
        this->m_task);
}

template <typename T>
auto DeferredComputation<T>::operator()() -> T
{
    return get();
}

template <typename T>
void DeferredComputation<T>::invalidate() &&
{
    std::visit(
        auxiliary::overloaded{
            [](detail::OneTimeTask<T> &task) {
                task.members.m_task = {};
                task.members.m_task_valid = false;
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
                return task.members.m_task_valid;
            },
            [](detail::CachedValue<T> const &) { return true; }},
        this->m_task);
}

template class DeferredComputation<void>;
template class DeferredComputation<RecordComponent::shared_ptr_dataset_types>;
template class DeferredComputation<std::string>; // used in tests

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
