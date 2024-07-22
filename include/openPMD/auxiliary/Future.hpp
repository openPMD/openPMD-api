#pragma once

#include "openPMD/auxiliary/TypeTraits.hpp"

#include <future>

namespace openPMD::auxiliary
{
template <typename T>
class DeferredFuture
{
    using task_type = std::packaged_task<T()>;
    using future_type = std::future<T>;
    future_type m_future;
    task_type m_task;

public:
    DeferredFuture(task_type);

    auto get() -> T;

    [[nodiscard]] auto valid() const noexcept -> bool;

    auto wait() -> void;
};
} // namespace openPMD::auxiliary
