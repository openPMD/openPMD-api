#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace openPMD::auxiliary
{
template <typename F>
struct defer_type
{
    F functor;
    bool do_run_this = true;
    ~defer_type()
    {
        if (!do_run_this)
        {
            return;
        }
        do_run_this = false;
        std::move(functor)();
    }

    auto to_opaque() && -> defer_type<std::function<void()>>
    {
        do_run_this = false;
        if (!do_run_this)
        {
            return defer_type<std::function<void()>>{{}, false};
        }
        else
        {
            return defer_type<std::function<void()>>{
                std::function<void()>{std::move(functor)}};
        }
    }
};

using opaque_defer_type = defer_type<std::function<void()>>;

template <typename F>
auto defer(F &&functor) -> defer_type<std::remove_reference_t<F>>
{
    return defer_type<std::remove_reference_t<F>>{std::forward<F>(functor)};
}
} // namespace openPMD::auxiliary
