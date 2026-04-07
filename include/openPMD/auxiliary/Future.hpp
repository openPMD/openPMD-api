#pragma once

#include <functional>
#include <variant>

namespace openPMD::auxiliary::detail
{
template <typename T>
struct OneTimeTask
{
    using task_type = std::function<T()>;
    task_type m_task;
    bool m_task_valid = true;

    auto operator()() -> T;
};

template <typename T>
struct CachedValue
{
    T val;
};
template <>
struct CachedValue<void>
{
    // this is silly
};
} // namespace openPMD::auxiliary::detail

namespace openPMD::auxiliary
{
template <typename T>
class DeferredComputation
{
    using task_type = std::function<T()>;
    using cached_type = std::conditional_t<
        std::is_void_v<T>,
        // just something that is not void
        detail::CachedValue<void>,
        T>;
    std::variant<detail::OneTimeTask<T>, detail::CachedValue<T>> m_task;

public:
    DeferredComputation(task_type);
    DeferredComputation(cached_type);

    ~DeferredComputation();

    auto get() -> T;
    auto operator()() -> T;

    void forget() &&;

    [[nodiscard]] auto valid() const noexcept -> bool;
};
} // namespace openPMD::auxiliary
