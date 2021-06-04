#ifndef REGIONS_HELPERS_HPP
#define REGIONS_HELPERS_HPP

#include <array>
#include <cstddef>
#include <type_traits>

namespace openPMD {
namespace Regions {

namespace detail {

// Combine hashes
template <typename T> std::size_t hash_combine(std::size_t seed, const T &x) {
  std::hash<T> h;
  // Taken from Boost
  return seed ^
         h(x) + size_t(0x00e6052366ac4440eULL) + (seed << 6) + (seed >> 2);
}

// Convert a tuple-like type to a std::array
template <typename T, class Tuple, std::size_t... Is, typename U>
constexpr auto array_push(const Tuple &t, std::index_sequence<Is...>,
                          const U &x) {
  return std::array<T, sizeof...(Is) + 1>{std::get<Is>(t)..., T(x)};
}

// Append an element to a std::array
template <class T, std::size_t N, typename U>
constexpr auto array_push(const std::array<T, N> &a, const U &e) {
  return array_push<T>(a, std::make_index_sequence<N>(), e);
}

// Construct a std::array from a function
template <typename T, std::size_t N, typename F>
constexpr std::array<T, N> construct_array(const F &f) {
  if constexpr (N == 0)
    return std::array<T, N>();
  if constexpr (N > 0)
    return array_push<T>(construct_array<T, N - 1>(f), f(N - 1));
}
} // namespace detail

} // namespace Regions
} // namespace openPMD

#endif // #ifndef REGIONS_HELPERS_HPP
