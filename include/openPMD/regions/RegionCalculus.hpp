#ifndef REGIONCALCULUS2_HPP
#define REGIONCALCULUS2_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace openPMD {
namespace RegionCalculus {

namespace detail {
// Combine hashes
template <typename T> std::size_t hash_combine(std::size_t seed, const T &x) {
  std::hash<T> h;
  // Taken from Boost
  return seed ^
         h(x) + size_t(0x00e6052366ac4440eULL) + (seed << 6) + (seed >> 2);
}
} // namespace detail

namespace detail {
template <typename T, class Tuple, std::size_t... Is, typename U>
constexpr auto array_push(const Tuple &t, std::index_sequence<Is...>,
                          const U &x) {
  return std::array<T, sizeof...(Is) + 1>{std::get<Is>(t)..., T(x)};
}

template <class T, std::size_t N, typename U>
constexpr auto array_push(const std::array<T, N> &a, const U &e) {
  return array_push<T>(a, std::make_index_sequence<N>(), e);
}

template <typename T, std::size_t N, typename F>
constexpr std::array<T, N> construct_array(const F &f) {
  if constexpr (N == 0)
    return std::array<T, N>();
  if constexpr (N > 0)
    return array_push<T>(construct_array<T, N - 1>(f), f(N - 1));
}
} // namespace detail

/** A D-dimensional point
 *
 * The dimension D needs to be known at compile time. @see ndpoint
 *
 * Points can represent either points or distances. Points are
 * fixed-size vectors that support arithmetic operations.
 */
template <typename T, std::size_t D> class point {
  std::array<T, D> elts;

public:
  /** Component type
   */
  typedef T value_type;
  /** Return type of point::size()
   */
  typedef std::size_t size_type;

  /** Create a value-initialized point
   *
   * For most types, this initializes all components to zero.
   */
  constexpr point() : elts{} {} // value initialization

  point(const point &) = default;
  point(point &&) = default;
  point &operator=(const point &) = default;
  point &operator=(point &&) = default;

  /** Loop over the natural number sequence [0, ..., D-1], and evaluate f for
   * each number
   */
  template <typename F> static constexpr void loop(const F &f) {
    for (size_type d = 0; d < D; ++d)
      f(d);
  }
  /** Create a new point by applying a function to the natural number sequence
   * [0, ..., D-1]
   */
  template <typename F> static constexpr point make(const F &f) {
    return detail::construct_array<T, D>(f);
  }

  /** Create a point with each component set to the same value a
   */
  static constexpr point pure(const T &a) {
    return make([&](size_type) { return a; });
  }

  /** Create a unit point, where component dir is one, and all other components
   * are zero
   */
  static constexpr point unit(size_type dir) {
    return make([&](size_type d) { return d == dir; });
  }

  /** Create a point with components set to the natural number
   * sequence [0, ..., D-1]
   */
  static constexpr point iota() {
    return point::make([](size_type d) { return d; });
  }

  /** Map a function over all components of one or several points
   *
   * Example:
   *   point<int,3> pi, pj;
   *   point<int,3> pk = fmap([](auto i, auto j) { return i+j; }, pi, pj);
   * This calculates the component-wise sum of pi and pj, i.e. pi + pj .
   */
  template <typename F, typename... Args,
            typename R = std::remove_cv_t<
                std::remove_reference_t<std::result_of_t<F(T, Args...)>>>>
  friend constexpr point<R, D> fmap(const F &f, const point &x,
                                    const point<Args, D> &...args) {
    return point<R, D>::make(
        [&](size_type d) { return f(x.elts[d], args.elts[d]...); });
  }
  /** Map a function over all components of one or several points, returning
   * void
   *
   * Example:
   *   point<int,3> pi;
   *   fmap_([](auto& i) { return i*=2; }, pi);
   * This doubles each component of pi, the same as pi *= 2 .
   */
  template <typename F, typename... Args>
  friend constexpr void fmap_(const F &f, const point &x,
                              const point<Args, D> &...args) {
    loop([&](size_type d) { f(x.elts[d], args.elts[d]...); });
  }

  /** Reduce over all components of one or several points
   *
   * Example:
   *   point<int,3> pi;
   *   int s = fold([](auto r, auto i) { return r+i; }, 0, pi);
   * This calculates the sum of all components ("horizonal sum") of pi,
   * same as sum(pi).
   */
  template <typename Op, typename R, typename... Args,
            typename = std::remove_cv_t<
                std::remove_reference_t<std::result_of_t<Op(R, T, Args...)>>>>
  friend constexpr R fold(const Op &op, R r, const point &x,
                          const point<Args, D> &...args) {
    loop([&](int d) { r = op(r, x[d], args[d]...); });
    return r;
  }

  /** Create a point from a point with different component type
   */
  template <typename U>
  constexpr point(const point<U, D> &x)
      : elts(fmap([](const U &a) { return T(a); }, x)) {}

  /** Create a point from pointers to first and one past the last element
   */
  constexpr point(const T *begin, const T *end)
      : elts((assert(begin + D == end),
              make([&](size_type d) { return begin[d]; }))) {}
  /** Create a point from an initializer list
   *
   * Example: point<int,3>{1,2,3}
   */
  constexpr point(std::initializer_list<T> lst)
      : point(lst.begin(), lst.end()) {}
  /** Create a point from a C-style array
   */
  constexpr point(const T (&arr)[D]) : elts(&arr[0], &arr[D]) {}
  /** Create a point from a std::array
   */
  constexpr point(const std::array<T, D> &arr) : elts(arr) {}
  /** Create a point from a std::vector
   */
  constexpr point(const std::vector<T> &vec) : point(vec.begin(), vec.end()) {}

  /** Convert a point to a std::array
   */
  operator std::array<T, D>() const { return elts; }
  /** Convert a point to a std::vector
   */
  explicit operator std::vector<T>() const {
    return std::vector<T>(elts.begin(), elts.end());
  }

  /** Number of components (same as number of dimensions)
   */
  constexpr size_type size() const { return D; }

  /** Get a component of a point
   */
  constexpr const T &operator[](const size_type d) const { return elts[d]; }
  /** Get a component of a point
   */
  constexpr T &operator[](const size_type d) { return elts[d]; }

  /** Number of dimensions (same as number of components)
   */
  constexpr size_type ndims() const { return D; }

  friend constexpr point operator+(const point &x) {
    return fmap([](const T &a) { return +a; }, x);
  }
  friend constexpr point operator-(const point &x) {
    return fmap([](const T &a) { return -a; }, x);
  }
  friend constexpr point operator~(const point &x) {
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return fmap([](const T &a) { return ~a; }, x);
    else
      return point();
  }
  friend constexpr point<bool, D> operator!(const point &x) {
    return fmap([](const T &a) { return !a; }, x);
  }

  friend constexpr point operator+(const point &x, const point &y) {
    return fmap([](const T &a, const T &b) { return a + b; }, x, y);
  }
  friend constexpr point operator-(const point &x, const point &y) {
    return fmap([](const T &a, const T &b) { return a - b; }, x, y);
  }
  friend constexpr point operator*(const point &x, const point &y) {
    return fmap([](const T &a, const T &b) { return a * b; }, x, y);
  }
  friend constexpr point operator/(const point &x, const point &y) {
    return fmap([](const T &a, const T &b) { return a / b; }, x, y);
  }
  friend constexpr point operator%(const point &x, const point &y) {
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return fmap([](const T &a, const T &b) { return a % b; }, x, y);
    else
      return point();
  }
  friend constexpr point operator&(const point &x, const point &y) {
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return fmap([](const T &a, const T &b) { return a & b; }, x, y);
    else
      return point();
  }
  friend constexpr point operator|(const point &x, const point &y) {
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return fmap([](const T &a, const T &b) { return a | b; }, x, y);
    else
      return point();
  }
  friend constexpr point operator^(const point &x, const point &y) {
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return fmap([](const T &a, const T &b) { return a ^ b; }, x, y);
    else
      return point();
  }
  friend constexpr point<bool, D> operator&&(const point &x, const point &y) {
    return fmap([](const T &a, const T &b) { return a && b; }, x, y);
  }
  friend constexpr point<bool, D> operator||(const point &x, const point &y) {
    return fmap([](const T &a, const T &b) { return a || b; }, x, y);
  }

  friend constexpr point operator+(const T &a, const point &y) {
    return fmap([&](const T &b) { return a + b; }, y);
  }
  friend constexpr point operator-(const T &a, const point &y) {
    return fmap([&](const T &b) { return a - b; }, y);
  }
  friend constexpr point operator*(const T &a, const point &y) {
    return fmap([&](const T &b) { return a * b; }, y);
  }
  friend constexpr point operator/(const T &a, const point &y) {
    return fmap([&](const T &b) { return a / b; }, y);
  }
  friend constexpr point operator%(const T &a, const point &y) {
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return fmap([&](const T &b) { return a % b; }, y);
    else
      return point();
  }
  friend constexpr point operator&(const T &a, const point &y) {
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return fmap([&](const T &b) { return a & b; }, y);
    else
      return point();
  }
  friend constexpr point operator|(const T &a, const point &y) {
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return fmap([&](const T &b) { return a | b; }, y);
    else
      return point();
  }
  friend constexpr point operator^(const T &a, const point &y) {
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return fmap([&](const T &b) { return a ^ b; }, y);
    else
      return point();
  }
  friend constexpr point<bool, D> operator&&(const T &a, const point &y) {
    return fmap([&](const T &b) { return a && b; }, y);
  }
  friend constexpr point<bool, D> operator||(const T &a, const point &y) {
    return fmap([&](const T &b) { return a || b; }, y);
  }

  friend constexpr point operator+(const point &x, const T &b) {
    return fmap([&](const T &a) { return a + b; }, x);
  }
  friend constexpr point operator-(const point &x, const T &b) {
    return fmap([&](const T &a) { return a - b; }, x);
  }
  friend constexpr point operator*(const point &x, const T &b) {
    return fmap([&](const T &a) { return a * b; }, x);
  }
  friend constexpr point operator/(const point &x, const T &b) {
    return fmap([&](const T &a) { return a / b; }, x);
  }
  friend constexpr point operator%(const point &x, const T &b) {
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return fmap([&](const T &a) { return a % b; }, x);
    else return point();
  }
  friend constexpr point operator&(const point &x, const T &b) {
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return fmap([&](const T &a) { return a & b; }, x);
    else
      return point();
  }
  friend constexpr point operator|(const point &x, const T &b) {
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return fmap([&](const T &a) { return a | b; }, x);
    else
      return point();
  }
  friend constexpr point operator^(const point &x, const T &b) {
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
      return fmap([&](const T &a) { return a ^ b; }, x);
    else
      return point();
  }
  friend constexpr point<bool, D> operator&&(const point &x, const T &b) {
    return fmap([&](const T &a) { return a && b; }, x);
  }
  friend constexpr point<bool, D> operator||(const point &x, const T &b) {
    return fmap([&](const T &a) { return a || b; }, x);
  }

  constexpr point &operator+=(const point &x) { return *this = *this + x; }
  constexpr point &operator-=(const point &x) { return *this = *this - x; }
  constexpr point &operator*=(const point &x) { return *this = *this * x; }
  constexpr point &operator/=(const point &x) { return *this = *this / x; }
  constexpr point &operator%=(const point &x) { return *this = *this % x; }
  constexpr point &operator&=(const point &x) { return *this = *this & x; }
  constexpr point &operator|=(const point &x) { return *this = *this | x; }
  constexpr point &operator^=(const point &x) { return *this = *this ^ x; }

  constexpr point &operator+=(const T &a) { return *this = *this + a; }
  constexpr point &operator-=(const T &a) { return *this = *this - a; }
  constexpr point &operator*=(const T &a) { return *this = *this * a; }
  constexpr point &operator/=(const T &a) { return *this = *this / a; }
  constexpr point &operator%=(const T &a) { return *this = *this % a; }
  constexpr point &operator&=(const T &a) { return *this = *this & a; }
  constexpr point &operator|=(const T &a) { return *this = *this | a; }
  constexpr point &operator^=(const T &a) { return *this = *this ^ a; }

  friend constexpr point abs(const point &x) {
    using std::abs;
    return fmap([&](const T &a) { return abs(a); }, x);
  }
  friend constexpr point fabs(const point &x) {
    using std::fabs;
    return fmap([&](const T &a) { return fabs(a); }, x);
  }

  friend constexpr point fmax(const point &x, const point &y) {
    using std::fmax;
    return fmap([](const T &a, const T &b) { return fmax(a, b); }, x, y);
  }
  friend constexpr point fmin(const point &x, const point &y) {
    using std::fmin;
    return fmap([](const T &a, const T &b) { return fmin(a, b); }, x, y);
  }
  friend constexpr point max(const point &x, const point &y) {
    using std::max;
    return fmap([](const T &a, const T &b) { return max(a, b); }, x, y);
  }
  friend constexpr point min(const point &x, const point &y) {
    using std::min;
    return fmap([](const T &a, const T &b) { return min(a, b); }, x, y);
  }

  friend constexpr point fmax(const T &a, const point &y) {
    using std::fmax;
    return fmap([&](const T &b) { return fmax(a, b); }, y);
  }
  friend constexpr point fmin(const T &a, const point &y) {
    using std::fmin;
    return fmap([&](const T &b) { return fmin(a, b); }, y);
  }
  friend constexpr point max(const T &a, const point &y) {
    using std::max;
    return fmap([&](const T &b) { return max(a, b); }, y);
  }
  friend constexpr point min(const T &a, const point &y) {
    using std::min;
    return fmap([&](const T &b) { return min(a, b); }, y);
  }

  friend constexpr point fmax(const point &x, const T &b) {
    using std::fmax;
    return fmap([&](const T &a) { return fmax(a, b); }, x);
  }
  friend constexpr point fmin(const point &x, const T &b) {
    using std::fmin;
    return fmap([&](const T &a) { return fmin(a, b); }, x);
  }
  friend constexpr point max(const point &x, const T &b) {
    using std::max;
    return fmap([&](const T &a) { return max(a, b); }, x);
  }
  friend constexpr point min(const point &x, const T &b) {
    using std::min;
    return fmap([&](const T &a) { return min(a, b); }, x);
  }

  friend constexpr bool all(const point &x) {
    return fold([](bool r, const T &a) { return r && a; }, true, x);
  }
  friend constexpr bool any(const point &x) {
    return fold([](bool r, const T &a) { return r || a; }, false, x);
  }
  friend constexpr T max_element(const point &x) {
    using std::max;
    return fold([](const T &r, const T &a) { return max(r, a); },
                std::numeric_limits<T>::lowest(), x);
  }
  friend constexpr T min_element(const point &x) {
    using std::min;
    return fold([](const T &r, const T &a) { return min(r, a); },
                std::numeric_limits<T>::max(), x);
  }
  friend constexpr T product(const point &x) {
    return fold([](const T &r, const T &a) { return r * a; }, T(1), x);
  }
  friend constexpr T sum(const point &x) {
    return fold([](const T &r, const T &a) { return r + a; }, T(0), x);
  }

  friend constexpr bool operator==(const point &x, const point &y) {
    return all(fmap([](const T &a, const T &b) { return a == b; }));
  }
  friend constexpr bool operator!=(const point &x, const point &y) {
    return any(fmap([](const T &a, const T &b) { return a != b; }));
  }

  /** Output a point
   */
  friend std::ostream &operator<<(std::ostream &os, const point &x) {
    os << "[";
    for (size_type d = 0; d < D; ++d) {
      if (d != 0)
        os << ",";
      os << x[d];
    }
    os << "]";
    return os;
  }
};

} // namespace RegionCalculus
} // namespace openPMD

namespace std {

template <typename T, std::size_t D>
struct equal_to<openPMD::RegionCalculus::point<T, D>> {
  constexpr bool
  operator()(const openPMD::RegionCalculus::point<T, D> &p,
             const openPMD::RegionCalculus::point<T, D> &q) const {
    return openPMD::RegionCalculus::point<T, D>::all(
        openPMD::RegionCalculus::point<T, D>::fmap(
            [](const T &a, const T &b) { return equal_to<T>()(a, b); }, p, q));
  }
};

template <typename T, std::size_t D>
struct hash<openPMD::RegionCalculus::point<T, D>> {
  constexpr size_t
  operator()(const openPMD::RegionCalculus::point<T, D> &p) const {
    return openPMD::RegionCalculus::point<T, D>::fold(
        [](size_t r, const T &b) {
          return openPMD::RegionCalculus::detail::hash_combine(r, b);
        },
        size_t(0xb22da17173243869ULL), p);
  }
};

template <typename T, std::size_t D>
struct less<openPMD::RegionCalculus::point<T, D>> {
  constexpr bool
  operator()(const openPMD::RegionCalculus::point<T, D> &p,
             const openPMD::RegionCalculus::point<T, D> &q) const {
    return openPMD::RegionCalculus::point<T, D>::fold(
        [](bool r, const T &a, const T &b) { return r && less<T>()(a, b); },
        true, p, q);
  }
};

} // namespace std

namespace openPMD {
namespace RegionCalculus {

namespace detail {

// Abstract base helper class

template <typename T> class vpoint {
public:
  typedef T value_type;
  typedef std::size_t size_type;

  virtual std::unique_ptr<vpoint> copy() const = 0;

  virtual ~vpoint() {}

  virtual size_type size() const = 0;

  virtual const T &operator[](const size_type d) const = 0;
  virtual T &operator[](const size_type d) = 0;

  virtual size_type ndims() const = 0;

  virtual std::unique_ptr<vpoint> operator+() const = 0;
  virtual std::unique_ptr<vpoint> operator-() const = 0;
  virtual std::unique_ptr<vpoint> operator~() const = 0;
  virtual std::unique_ptr<vpoint<bool>> operator!() const = 0;

  virtual std::unique_ptr<vpoint> operator+(const vpoint &x) const = 0;
  virtual std::unique_ptr<vpoint> operator-(const vpoint &x) const = 0;
  virtual std::unique_ptr<vpoint> operator*(const vpoint &x) const = 0;
  virtual std::unique_ptr<vpoint> operator/(const vpoint &x) const = 0;
  virtual std::unique_ptr<vpoint> operator%(const vpoint &x) const = 0;
  virtual std::unique_ptr<vpoint> operator&(const vpoint &x) const = 0;
  virtual std::unique_ptr<vpoint> operator|(const vpoint &x) const = 0;
  virtual std::unique_ptr<vpoint> operator^(const vpoint &x) const = 0;
  virtual std::unique_ptr<vpoint<bool>> operator&&(const vpoint &x) const = 0;
  virtual std::unique_ptr<vpoint<bool>> operator||(const vpoint &x) const = 0;

  virtual std::unique_ptr<vpoint> left_plus(const T &a) const = 0;
  virtual std::unique_ptr<vpoint> left_minus(const T &a) const = 0;
  virtual std::unique_ptr<vpoint> left_multiplies(const T &a) const = 0;
  virtual std::unique_ptr<vpoint> left_divides(const T &a) const = 0;
  virtual std::unique_ptr<vpoint> left_modulus(const T &a) const = 0;
  virtual std::unique_ptr<vpoint> left_bit_and(const T &a) const = 0;
  virtual std::unique_ptr<vpoint> left_bit_or(const T &a) const = 0;
  virtual std::unique_ptr<vpoint> left_bit_xor(const T &a) const = 0;
  virtual std::unique_ptr<vpoint<bool>> left_logical_and(const T &a) const = 0;
  virtual std::unique_ptr<vpoint<bool>> left_logical_or(const T &a) const = 0;

  virtual std::unique_ptr<vpoint> operator+(const T &b) const = 0;
  virtual std::unique_ptr<vpoint> operator-(const T &b) const = 0;
  virtual std::unique_ptr<vpoint> operator*(const T &b) const = 0;
  virtual std::unique_ptr<vpoint> operator/(const T &b) const = 0;
  virtual std::unique_ptr<vpoint> operator%(const T &b) const = 0;
  virtual std::unique_ptr<vpoint> operator&(const T &b) const = 0;
  virtual std::unique_ptr<vpoint> operator|(const T &b) const = 0;
  virtual std::unique_ptr<vpoint> operator^(const T &b) const = 0;
  virtual std::unique_ptr<vpoint<bool>> operator&&(const T &b) const = 0;
  virtual std::unique_ptr<vpoint<bool>> operator||(const T &b) const = 0;

  virtual vpoint &operator+=(const vpoint &x) = 0;
  virtual vpoint &operator-=(const vpoint &x) = 0;
  virtual vpoint &operator*=(const vpoint &x) = 0;
  virtual vpoint &operator/=(const vpoint &x) = 0;
  virtual vpoint &operator%=(const vpoint &x) = 0;
  virtual vpoint &operator&=(const vpoint &x) = 0;
  virtual vpoint &operator|=(const vpoint &x) = 0;
  virtual vpoint &operator^=(const vpoint &x) = 0;

  virtual vpoint &operator+=(const T &b) = 0;
  virtual vpoint &operator-=(const T &b) = 0;
  virtual vpoint &operator*=(const T &b) = 0;
  virtual vpoint &operator/=(const T &b) = 0;
  virtual vpoint &operator%=(const T &b) = 0;
  virtual vpoint &operator&=(const T &b) = 0;
  virtual vpoint &operator|=(const T &b) = 0;
  virtual vpoint &operator^=(const T &b) = 0;

  virtual std::ostream &output(std::ostream &os) const = 0;
};

// Helper class wrapping point<T,D>

template <typename T, std::size_t D> class wpoint final : public vpoint<T> {
  point<T, D> p;

public:
  using typename vpoint<T>::value_type;
  using typename vpoint<T>::size_type;

  wpoint() : p{} {}

  wpoint(const wpoint &x) = default;
  wpoint(wpoint &&) = default;
  wpoint &operator=(const wpoint &) = default;
  wpoint &operator=(wpoint &&) = default;

  wpoint(const point<T, D> &p) : p(p) {}
  wpoint(point<T, D> &&p) : p(std::move(p)) {}

  std::unique_ptr<vpoint<T>> copy() const override {
    return std::make_unique<wpoint>(*this);
  }

  ~wpoint() override {}

  constexpr size_type size() const override { return p.size(); }

  const T &operator[](const size_type d) const override { return p[d]; }
  T &operator[](const size_type d) override { return p[d]; }

  constexpr size_type ndims() const override { return p.ndims(); }

  std::unique_ptr<vpoint<T>> operator+() const override {
    return std::make_unique<wpoint>(+p);
  }
  std::unique_ptr<vpoint<T>> operator-() const override {
    return std::make_unique<wpoint>(+p);
  }
  std::unique_ptr<vpoint<T>> operator~() const override {
    return std::make_unique<wpoint>(~p);
  }
  std::unique_ptr<vpoint<bool>> operator!() const override {
    return std::make_unique<wpoint<bool, D>>(!p);
  }

  std::unique_ptr<vpoint<T>> operator+(const vpoint<T> &x) const override {
    return std::make_unique<wpoint>(p + dynamic_cast<const wpoint &>(x).p);
  }
  std::unique_ptr<vpoint<T>> operator-(const vpoint<T> &x) const override {
    return std::make_unique<wpoint>(p - dynamic_cast<const wpoint &>(x).p);
  }
  std::unique_ptr<vpoint<T>> operator*(const vpoint<T> &x) const override {
    return std::make_unique<wpoint>(p * dynamic_cast<const wpoint &>(x).p);
  }
  std::unique_ptr<vpoint<T>> operator/(const vpoint<T> &x) const override {
    return std::make_unique<wpoint>(p / dynamic_cast<const wpoint &>(x).p);
  }
  std::unique_ptr<vpoint<T>> operator%(const vpoint<T> &x) const override {
    return std::make_unique<wpoint>(p % dynamic_cast<const wpoint &>(x).p);
  }
  std::unique_ptr<vpoint<T>> operator&(const vpoint<T> &x) const override {
    return std::make_unique<wpoint>(p & dynamic_cast<const wpoint &>(x).p);
  }
  std::unique_ptr<vpoint<T>> operator|(const vpoint<T> &x) const override {
    return std::make_unique<wpoint>(p | dynamic_cast<const wpoint &>(x).p);
  }
  std::unique_ptr<vpoint<T>> operator^(const vpoint<T> &x) const override {
    return std::make_unique<wpoint>(p ^ dynamic_cast<const wpoint &>(x).p);
  }
  std::unique_ptr<vpoint<bool>> operator&&(const vpoint<T> &x) const override {
    return std::make_unique<wpoint<bool, D>>(p &&
                                             dynamic_cast<const wpoint &>(x).p);
  }
  std::unique_ptr<vpoint<bool>> operator||(const vpoint<T> &x) const override {
    return std::make_unique<wpoint<bool, D>>(p ||
                                             dynamic_cast<const wpoint &>(x).p);
  }

  std::unique_ptr<vpoint<T>> left_plus(const T &a) const override {
    return std::make_unique<wpoint>(a + p);
  }
  std::unique_ptr<vpoint<T>> left_minus(const T &a) const override {
    return std::make_unique<wpoint>(a - p);
  }
  std::unique_ptr<vpoint<T>> left_multiplies(const T &a) const override {
    return std::make_unique<wpoint>(a * p);
  }
  std::unique_ptr<vpoint<T>> left_divides(const T &a) const override {
    return std::make_unique<wpoint>(a / p);
  }
  std::unique_ptr<vpoint<T>> left_modulus(const T &a) const override {
    return std::make_unique<wpoint>(a % p);
  }
  std::unique_ptr<vpoint<T>> left_bit_and(const T &a) const override {
    return std::make_unique<wpoint>(a & p);
  }
  std::unique_ptr<vpoint<T>> left_bit_or(const T &a) const override {
    return std::make_unique<wpoint>(a | p);
  }
  std::unique_ptr<vpoint<T>> left_bit_xor(const T &a) const override {
    return std::make_unique<wpoint>(a ^ p);
  }
  std::unique_ptr<vpoint<bool>> left_logical_and(const T &a) const override {
    return std::make_unique<wpoint<bool, D>>(a && p);
  }
  std::unique_ptr<vpoint<bool>> left_logical_or(const T &a) const override {
    return std::make_unique<wpoint<bool, D>>(a || p);
  }

  std::unique_ptr<vpoint<T>> operator+(const T &b) const override {
    return std::make_unique<wpoint>(p + b);
  }
  std::unique_ptr<vpoint<T>> operator-(const T &b) const override {
    return std::make_unique<wpoint>(p - b);
  }
  std::unique_ptr<vpoint<T>> operator*(const T &b) const override {
    return std::make_unique<wpoint>(p * b);
  }
  std::unique_ptr<vpoint<T>> operator/(const T &b) const override {
    return std::make_unique<wpoint>(p / b);
  }
  std::unique_ptr<vpoint<T>> operator%(const T &b) const override {
    return std::make_unique<wpoint>(p % b);
  }
  std::unique_ptr<vpoint<T>> operator&(const T &b) const override {
    return std::make_unique<wpoint>(p & b);
  }
  std::unique_ptr<vpoint<T>> operator|(const T &b) const override {
    return std::make_unique<wpoint>(p | b);
  }
  std::unique_ptr<vpoint<T>> operator^(const T &b) const override {
    return std::make_unique<wpoint>(p ^ b);
  }
  std::unique_ptr<vpoint<bool>> operator&&(const T &b) const override {
    return std::make_unique<wpoint<bool, D>>(p && b);
  }
  std::unique_ptr<vpoint<bool>> operator||(const T &b) const override {
    return std::make_unique<wpoint<bool, D>>(p || b);
  }

  vpoint<T> &operator+=(const vpoint<T> &x) override {
    p += dynamic_cast<const wpoint &>(x).p;
    return *this;
  }
  vpoint<T> &operator-=(const vpoint<T> &x) override {
    p -= dynamic_cast<const wpoint &>(x).p;
    return *this;
  }
  vpoint<T> &operator*=(const vpoint<T> &x) override {
    p *= dynamic_cast<const wpoint &>(x).p;
    return *this;
  }
  vpoint<T> &operator/=(const vpoint<T> &x) override {
    p /= dynamic_cast<const wpoint &>(x).p;
    return *this;
  }
  vpoint<T> &operator%=(const vpoint<T> &x) override {
    p %= dynamic_cast<const wpoint &>(x).p;
    return *this;
  }
  vpoint<T> &operator&=(const vpoint<T> &x) override {
    p &= dynamic_cast<const wpoint &>(x).p;
    return *this;
  }
  vpoint<T> &operator|=(const vpoint<T> &x) override {
    p |= dynamic_cast<const wpoint &>(x).p;
    return *this;
  }
  vpoint<T> &operator^=(const vpoint<T> &x) override {
    p ^= dynamic_cast<const wpoint &>(x).p;
    return *this;
  }

  vpoint<T> &operator+=(const T &b) override {
    p += b;
    return *this;
  }
  vpoint<T> &operator-=(const T &b) override {
    p -= b;
    return *this;
  }
  vpoint<T> &operator*=(const T &b) override {
    p *= b;
    return *this;
  }
  vpoint<T> &operator/=(const T &b) override {
    p /= b;
    return *this;
  }
  vpoint<T> &operator%=(const T &b) override {
    p %= b;
    return *this;
  }
  vpoint<T> &operator&=(const T &b) override {
    p &= b;
    return *this;
  }
  vpoint<T> &operator|=(const T &b) override {
    p |= b;
    return *this;
  }
  vpoint<T> &operator^=(const T &b) override {
    p ^= b;
    return *this;
  }

  std::ostream &output(std::ostream &os) const override { return os << p; }
};

template <typename T>
std::unique_ptr<vpoint<T>> make_vpoint(const std::size_t D) {
  switch (D) {
  case 0:
    return std::make_unique<wpoint<T, 0>>();
  case 1:
    return std::make_unique<wpoint<T, 1>>();
  case 2:
    return std::make_unique<wpoint<T, 2>>();
  case 3:
    return std::make_unique<wpoint<T, 3>>();
  case 4:
    return std::make_unique<wpoint<T, 4>>();
  case 5:
    return std::make_unique<wpoint<T, 5>>();
  default:
    abort();
  }
}

} // namespace detail

/** A point
 *
 * The dimension (number of component) of the point is only known at
 * run-time. @see point
 *
 * Points can represent either points or distances. Points are
 * fixed-size vectors that support arithmetic operations.
 */
template <typename T> class ndpoint {

  template <typename U> using vpoint = detail::vpoint<U>;

  template <typename U> friend class ndpoint;

  std::unique_ptr<vpoint<T>> p;

  ndpoint(std::unique_ptr<vpoint<T>> p) : p(std::move(p)) {}

public:
  /** Component type
   */
  typedef typename vpoint<T>::value_type value_type;
  /** Return type of point::size()
   */
  typedef typename vpoint<T>::size_type size_type;

  /** Create an invalid point
   */
  ndpoint() : p() {}
  /** Create a value-initialized point with D components
   */
  ndpoint(const size_type D) : p(detail::make_vpoint<T>(D)) {}

  ndpoint(const ndpoint &x) : p(x.p ? x.p->copy() : nullptr) {}
  ndpoint(ndpoint &&) = default;
  ndpoint &operator=(const ndpoint &) = default;
  ndpoint &operator=(ndpoint &&) = default;

  /** Check whether a point is valid
   *
   * A valid point knows its number of dimensions, and its components
   * are initialized. An invalid point does not know its number of
   * dimensions and holds no data, similar to a null pointer.
   *
   * Most other member functions must not be called for invalid
   * points.
   */
  operator bool() const { return bool(p); }

  /** Number of comopnents (same as number of dimensions)
   */
  size_type size() const { return p->size(); }

  /** Get a component of a point
   */
  const T &operator[](const size_type d) const { return (*p)[d]; }
  /** Get a component of a point
   */
  T &operator[](const size_type d) { return (*p)[d]; }

  /** Number of dimensions (same as number of comopnents)
   */
  size_type ndims() const { return p->ndims(); }

  friend ndpoint operator+(const ndpoint &x) { return ndpoint(+*x.p); }
  friend ndpoint operator-(const ndpoint &x) { return ndpoint(-*x.p); }
  friend ndpoint operator~(const ndpoint &x) { return ndpoint(~*x.p); }
  friend ndpoint<bool> operator!(const ndpoint &x) {
    return ndpoint<bool>(!*x.p);
  }

  friend ndpoint operator+(const ndpoint &x, const ndpoint &y) {
    return ndpoint(*x.p + *y.p);
  }
  friend ndpoint operator-(const ndpoint &x, const ndpoint &y) {
    return ndpoint(*x.p - *y.p);
  }
  friend ndpoint operator*(const ndpoint &x, const ndpoint &y) {
    return ndpoint(*x.p * *y.p);
  }
  friend ndpoint operator/(const ndpoint &x, const ndpoint &y) {
    return ndpoint(*x.p / *y.p);
  }
  friend ndpoint operator%(const ndpoint &x, const ndpoint &y) {
    return ndpoint(*x.p % *y.p);
  }
  friend ndpoint operator&(const ndpoint &x, const ndpoint &y) {
    return ndpoint(*x.p & *y.p);
  }
  friend ndpoint operator|(const ndpoint &x, const ndpoint &y) {
    return ndpoint(*x.p | *y.p);
  }
  friend ndpoint operator^(const ndpoint &x, const ndpoint &y) {
    return ndpoint(*x.p ^ *y.p);
  }
  friend ndpoint<bool> operator&&(const ndpoint &x, const ndpoint &y) {
    return ndpoint(*x.p && *y.p);
  }
  friend ndpoint<bool> operator||(const ndpoint &x, const ndpoint &y) {
    return ndpoint(*x.p || *y.p);
  }

  friend ndpoint operator+(const T &a, const ndpoint &y) {
    return ndpoint(y.p->left_plus(a));
  }
  friend ndpoint operator-(const T &a, const ndpoint &y) {
    return ndpoint(y.p->left_minus(a));
  }
  friend ndpoint operator*(const T &a, const ndpoint &y) {
    return ndpoint(y.p->left_multiplies(a));
  }
  friend ndpoint operator/(const T &a, const ndpoint &y) {
    return ndpoint(y.p->left_divides(a));
  }
  friend ndpoint operator%(const T &a, const ndpoint &y) {
    return ndpoint(y.p->left_modulus(a));
  }
  friend ndpoint operator&(const T &a, const ndpoint &y) {
    return ndpoint(y.p->left_bit_and(a));
  }
  friend ndpoint operator|(const T &a, const ndpoint &y) {
    return ndpoint(y.p->left_bit_or(a));
  }
  friend ndpoint operator^(const T &a, const ndpoint &y) {
    return ndpoint(y.p->left_bit_xor(a));
  }
  friend ndpoint<bool> operator&&(const T &a, const ndpoint &y) {
    return ndpoint<bool>(y.p->left_logical_and(a));
  }
  friend ndpoint<bool> operator||(const T &a, const ndpoint &y) {
    return ndpoint<bool>(y.p->left_logical_or(a));
  }

  friend ndpoint operator+(const ndpoint &x, const T &b) {
    return ndpoint(*x.p + b);
  }
  friend ndpoint operator-(const ndpoint &x, const T &b) {
    return ndpoint(*x.p - b);
  }
  friend ndpoint operator*(const ndpoint &x, const T &b) {
    return ndpoint(*x.p * b);
  }
  friend ndpoint operator/(const ndpoint &x, const T &b) {
    return ndpoint(*x.p / b);
  }
  friend ndpoint operator%(const ndpoint &x, const T &b) {
    return ndpoint(*x.p % b);
  }
  friend ndpoint operator&(const ndpoint &x, const T &b) {
    return ndpoint(*x.p & b);
  }
  friend ndpoint operator|(const ndpoint &x, const T &b) {
    return ndpoint(*x.p | b);
  }
  friend ndpoint operator^(const ndpoint &x, const T &b) {
    return ndpoint(*x.p ^ b);
  }
  friend ndpoint<bool> operator&&(const ndpoint &x, const T &b) {
    return ndpoint(*x.p && b);
  }
  friend ndpoint<bool> operator||(const ndpoint &x, const T &b) {
    return ndpoint(*x.p || b);
  }

  ndpoint &operator+=(const ndpoint &x) {
    *p += *x.p;
    return *this;
  }
  ndpoint &operator-=(const ndpoint &x) {
    *p -= *x.p;
    return *this;
  }
  ndpoint &operator*=(const ndpoint &x) {
    *p *= *x.p;
    return *this;
  }
  ndpoint &operator/=(const ndpoint &x) {
    *p /= *x.p;
    return *this;
  }
  ndpoint &operator%=(const ndpoint &x) {
    *p %= *x.p;
    return *this;
  }
  ndpoint &operator&=(const ndpoint &x) {
    *p &= *x.p;
    return *this;
  }
  ndpoint &operator|=(const ndpoint &x) {
    *p |= *x.p;
    return *this;
  }
  ndpoint &operator^=(const ndpoint &x) {
    *p ^= *x.p;
    return *this;
  }

  ndpoint &operator+=(const T &a) {
    *p += a;
    return *this;
  }
  ndpoint &operator-=(const T &a) {
    *p -= a;
    return *this;
  }
  ndpoint &operator*=(const T &a) {
    *p *= a;
    return *this;
  }
  ndpoint &operator/=(const T &a) {
    *p /= a;
    return *this;
  }
  ndpoint &operator%=(const T &a) {
    *p %= a;
    return *this;
  }
  ndpoint &operator&=(const T &a) {
    *p &= a;
    return *this;
  }
  ndpoint &operator|=(const T &a) {
    *p |= a;
    return *this;
  }
  ndpoint &operator^=(const T &a) {
    *p ^= a;
    return *this;
  }

  friend ndpoint abs(const ndpoint &x) { return abs(*x.p); }
  friend ndpoint fabs(const ndpoint &x) { return fabs(*x.p); }

  friend ndpoint fmax(const ndpoint &x, const ndpoint &y) {
    return fmax(*x.p, *y.p);
  }
  friend ndpoint fmin(const ndpoint &x, const ndpoint &y) {
    return fmin(*x.p, *y.p);
  }
  friend ndpoint max(const ndpoint &x, const ndpoint &y) {
    return max(*x.p, *y.p);
  }
  friend ndpoint min(const ndpoint &x, const ndpoint &y) {
    return min(*x.p, *y.p);
  }

  friend ndpoint fmax(const T &a, const ndpoint &y) { return fmax(a, *y.p); }
  friend ndpoint fmin(const T &a, const ndpoint &y) { return fmin(a, *y.p); }
  friend ndpoint max(const T &a, const ndpoint &y) { return max(a, *y.p); }
  friend ndpoint min(const T &a, const ndpoint &y) { return min(a, *y.p); }

  friend ndpoint fmax(const ndpoint &x, const T &b) { return fmax(*x.p, b); }
  friend ndpoint fmin(const ndpoint &x, const T &b) { return fmin(*x.p, b); }
  friend ndpoint max(const ndpoint &x, const T &b) { return max(*x.p, b); }
  friend ndpoint min(const ndpoint &x, const T &b) { return min(*x.p, b); }

  friend T all(const ndpoint &x) { return all(*x.p); }
  friend T any(const ndpoint &x) { return any(*x.p); }
  friend T max_element(const ndpoint &x) { return max_element(*x.p); }
  friend T min_element(const ndpoint &x) { return min_element(*x.p); }
  friend T product(const ndpoint &x) { return product(*x.p); }
  friend T sum(const ndpoint &x) { return sum(*x.p); }

  friend bool operator==(const ndpoint &x, const ndpoint &y) {
    return *x.p == *y.p;
  }
  friend bool operator!=(const ndpoint &x, const ndpoint &y) {
    return *x.p != *y.p;
  }

  /** Output a point
   */
  friend std::ostream &operator<<(std::ostream &os, const ndpoint &x) {
    if (x.p)
      x.p->output(os);
    else
      os << "[INVALID]";
    return os;
  }
};

} // namespace RegionCalculus
} // namespace openPMD

#endif // #ifndef REGIONCALCULUS2_HPP
