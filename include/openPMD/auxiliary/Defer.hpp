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

    explicit defer_type() = default;

    struct forwarding_tag
    {};

    template <typename F_>
    defer_type(forwarding_tag, F_ &&functor_in)
        : functor{std::forward<F_>(functor_in)}
    {}

    template <typename F_>
    defer_type(defer_type<F_> &&other) : functor{std::move(other.functor)}
    {
        other.do_run_this = false;
    }

    template <typename F_>
    auto operator=(defer_type<F_> &&other)
    {
        functor = std::move(other.functor);
        other.do_run_this = false;
    }

    defer_type(defer_type const &) = delete;
    auto operator=(defer_type const &) -> defer_type & = delete;
};

using opaque_defer_type = defer_type<std::function<void()>>;

template <typename F>
auto defer(F &&functor) -> defer_type<std::remove_reference_t<F>>
{
    using res_t = defer_type<std::remove_reference_t<F>>;
    using tag_t = typename res_t::forwarding_tag;
    return res_t{tag_t{}, std::forward<F>(functor)};
}
} // namespace openPMD::auxiliary
