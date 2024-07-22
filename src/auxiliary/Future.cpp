#include "openPMD/auxiliary/Future.hpp"
#include "openPMD/LoadStoreChunk.hpp"

#include <memory>

// comment

#include "openPMD/DatatypeMacros.hpp"

namespace openPMD::auxiliary
{

template <typename T>
DeferredFuture<T>::DeferredFuture(task_type task)
    : m_future(task.get_future()), m_task(std::move(task))
{}

template <typename T>
auto DeferredFuture<T>::get() -> T
{
    if (m_future.valid())
    {
        m_task();
    } // else get() was already called, propagate the std::future behavior
    return m_future.get();
}

template <typename T>
auto DeferredFuture<T>::valid() const noexcept -> bool
{
    return m_future.valid();
}

template <typename T>
auto DeferredFuture<T>::wait() -> void
{
    if (!m_task.valid())
    {
        m_task();
    }
}

template class DeferredFuture<void>;
template class DeferredFuture<auxiliary::detail::shared_ptr_dataset_types>;
#define INSTANTIATE_FUTURE(dtype)                                              \
    template class DeferredFuture<std::shared_ptr<dtype>>;
OPENPMD_FOREACH_DATASET_DATATYPE(INSTANTIATE_FUTURE)
#undef INSTANTIATE_FUTURE
} // namespace openPMD::auxiliary

#include "openPMD/UndefDatatypeMacros.hpp"
