#ifndef REGIONS_HELPERS_HPP
#define REGIONS_HELPERS_HPP

#include <array>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <vector>

namespace openPMD {
namespace Regions {

#define REGIONS_DEBUG 1 // 0 or 1

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
  return std::array<T, sizeof...(Is) + 1>{std::get<Is>(t)..., {T(x)}};
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
  constexpr bool operator()(const Tuple1 &, const Tuple2 &) const {
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
  constexpr int operator()(const Tuple1 &, const Tuple2 &) const {
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

////////////////////////////////////////////////////////////////////////////////

// Reduce over a std::vector, using bisection to reduce cost

/** Reduce a non-empty range
 */
template <typename Op, typename T,
          typename R = typename std::result_of<Op(T, T)>::type>
T reduce1(const Op &op, std::vector<T> &xs) {
  assert(!xs.empty());
  for (std::size_t dist = 1; dist < xs.size(); dist *= 2)
    for (std::size_t i = 0; i + dist < xs.size(); i += 2 * dist)
      xs[i] = op(std::move(xs[i]), std::move(xs[i + dist]));
  return std::move(xs[0]);
}

/** Mapreduce a range with a given neutral element
 */
template <typename F, typename Op, typename R, typename B, typename E>
R mapreduce(const F &f, const Op &op, R z, const B &b, const E &e) {
  std::vector<R> rs;
  for (auto i = b; i != e; ++i)
    rs.push_back(f(*i));
  if (rs.empty())
    return z;
  return reduce1(op, rs);
}

/** Mapreduce a range
 */
template <typename F, typename Op, typename I,
          typename T = typename std::iterator_traits<I>::value_type,
          typename R = std::decay_t<std::result_of_t<F(T)>>>
R mapreduce(const F &f, const Op &op, const I &b, const I &e) {
  return mapreduce(f, op, R(), b, e);
}

/** Mapreduce a container
 */
template <typename F, typename Op, typename C,
          typename T = typename C::value_type,
          typename R = std::decay_t<std::result_of_t<F(T)>>>
R mapreduce(const F &f, const Op &op, const C &c) {
  return mapreduce(f, op, std::begin(c), std::end(c));
}

} // namespace helpers

} // namespace Regions
} // namespace openPMD

#endif // #ifndef REGIONS_HELPERS_HPP
