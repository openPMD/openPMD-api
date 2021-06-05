#ifndef REGIONS_HELPERS_HPP
#define REGIONS_HELPERS_HPP

#include <array>
#include <cstddef>
#include <type_traits>

namespace openPMD {
namespace Regions {

namespace helpers {

////////////////////////////////////////////////////////////////////////////////

// Combine hash values
template <typename T> std::size_t hash_combine(std::size_t seed, const T &x) {
  std::hash<T> h;
  // Taken from Boost
  return seed ^
         h(x) + size_t(0x00e6052366ac4440eULL) + (seed << 6) + (seed >> 2);
}

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

// Compare tuples

namespace impl {

template <std::size_t N> struct tuple_eq {
  template <typename Tuple1, typename Tuple2>
  constexpr bool operator()(const Tuple1 &x, const Tuple2 &y) const {
    typedef std::tuple_element_t<N - 1, Tuple1> T1;
    typedef std::tuple_element_t<N - 1, Tuple2> T2;
    typedef std::common_type_t<T1, T2> T;
    const std::equal_to<T> eq;
    return tuple_eq<N - 1>()(x, y) &&
           eq(std::get<N - 1>(x), std::get<N - 1>(y));
  }
};
template <> struct tuple_eq<0> {
  template <typename Tuple1, typename Tuple2>
  constexpr bool operator()(const Tuple1 &x, const Tuple2 &y) const {
    return true;
  }
};

template <std::size_t N> struct tuple_cmp {
  template <typename Tuple1, typename Tuple2>
  constexpr int operator()(const Tuple1 &x, const Tuple2 &y) const {
    typedef std::tuple_element_t<N - 1, Tuple1> T1;
    typedef std::tuple_element_t<N - 1, Tuple2> T2;
    typedef std::common_type_t<T1, T2> T;
    const int cmp = tuple_cmp<N - 1>()(x, y);
    if (cmp != 0)
      return cmp;
    const std::less<T> lt;
    if (lt(std::get<N - 1>(x), std::get<N - 1>(y)))
      return -1;
    if (lt(std::get<N - 1>(y), std::get<N - 1>(x)))
      return +1;
    return 0;
  }
};
template <> struct tuple_cmp<0> {
  template <typename Tuple1, typename Tuple2>
  constexpr int operator()(const Tuple1 &x, const Tuple2 &y) const {
    return 0;
  }
};

} // namespace impl

template <typename Tuple1, typename Tuple2>
constexpr bool tuple_eq(const Tuple1 &x, const Tuple2 &y) {
  constexpr std::size_t sz = std::tuple_size_v<Tuple1>;
  static_assert(std::tuple_size_v<Tuple2> == sz);
  return impl::tuple_eq<sz>()(x, y);
}

template <typename Tuple1, typename Tuple2>
constexpr int tuple_cmp(const Tuple1 &x, const Tuple2 &y) {
  constexpr std::size_t sz = std::tuple_size_v<Tuple1>;
  static_assert(std::tuple_size_v<Tuple2> == sz);
  return impl::tuple_cmp<sz>()(x, y);
}

template <typename Tuple1, typename Tuple2>
constexpr bool tuple_lt(const Tuple1 &x, const Tuple2 &y) {
  return tuple_cmp(x, y) < 0;
}

} // namespace helpers

} // namespace Regions
} // namespace openPMD

#endif // #ifndef REGIONS_HELPERS_HPP
