#pragma once

#include "openPMD/backend/Attribute.hpp"

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace openPMD::auxiliary
{
template <typename F, typename SFINAE = void>
struct CallResult
{
    // If F is not callable, then use its type directly without calling.
    using type = F;
};

template <typename F>
struct CallResult<F, std::void_t<decltype(std::declval<F>()())>>
{
    // If F is callable (used to avoid complex computations until necessary),
    // then use its return type.
    using type = decltype(std::declval<F>()());
};

template <typename F>
using CallResult_t = typename CallResult<F>::type;

template <typename T>
auto write_val_to_stderr(T const &val) -> std::ostream &;
auto write_to_stderr(Attribute const &a) -> std::ostream &;

auto createDefaultAxisLabels(uint64_t dimensionality)
    -> std::vector<std::string>;
auto createDefaultVector(uint64_t dimensionality, double defaultValue)
    -> std::vector<double>;
} // namespace openPMD::auxiliary

namespace openPMD::internal
{
enum class WriteOrRead : std::uint8_t
{
    Write,
    Read
};
}
