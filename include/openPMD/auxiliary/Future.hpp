#pragma once

#include "openPMD/auxiliary/TypeTraits.hpp"

#include <functional>

namespace openPMD::auxiliary
{
template <typename T>
class DeferredComputation
{
    using task_type = std::function<T()>;
    task_type m_task;
    bool m_valid = false;

public:
    DeferredComputation(task_type);

    auto get() -> T;

    [[nodiscard]] auto valid() const noexcept -> bool;
};
} // namespace openPMD::auxiliary
