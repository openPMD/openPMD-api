#ifndef REGIONS_POINT_HPP
#define REGIONS_POINT_HPP

#include "Helpers.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace openPMD {
namespace Regions {

/** A D-dimensional point
 *
 * The dimension D needs to be known at compile time. @see NDPoint
 *
 * Points can represent either points or distances. Points are
 * fixed-size vectors that support arithmetic operations.
 */
template <typename T, std::size_t D> class Point {
  std::array<T, D> elts;

  friend struct std::equal_to<Point>;
  friend struct std::hash<Point>;
  friend struct std::less<Point>;

public:
  /** Component type
   */
  typedef T value_type;
  /** Return type of Point::size()
   */
  typedef std::ptrdiff_t size_type;

  /** Create a value-initialized Point
   *
   * For most types, this initializes all components to zero.
   */
  constexpr Point() : elts{} {} // value initialization

  Point(const Point &) = default;
  Point(Point &&) = default;
  Point &operator=(const Point &) = default;
  Point &operator=(Point &&) = default;

  /** Loop over the natural number sequence [0, ..., D-1], and evaluate f for
   * each number
   */
  template <typename F> static constexpr void loop(const F &f) {
    for (std::size_t d = 0; d < D; ++d)
      f(d);
  }
  /** Create a new Point by applying a function to the natural number sequence
   * [0, ..., D-1]
   */
  template <typename F> static constexpr Point make(const F &f) {
    return helpers::construct_array<T, D>(f);
  }

  /** Create a point with each component set to the same value a
   */
  static constexpr Point pure(const T &a) {
    return make([&](size_type) { return a; });
  }

  /** Create a unit Point, where component dir is one, and all other components
   * are zero
   */
  static constexpr Point unit(size_type dir) {
    return make([&](size_type d) { return d == dir; });
  }

  /** Create a point with components set to the natural number
   * sequence [0, ..., D-1]
   */
  static constexpr Point iota() {
    return Point::make([](size_type d) { return d; });
  }

  /** Map a function over all components of one or several Points
   *
   * Example:
   *   Point<int,3> pi, pj;
   *   Point<int,3> pk = fmap([](auto i, auto j) { return i+j; }, pi, pj);
   * This calculates the component-wise sum of pi and pj, i.e. pi + pj .
   */
  template <typename F, typename... Args,
            typename R = std::remove_cv_t<
                std::remove_reference_t<std::result_of_t<F(T, Args...)>>>>
  friend constexpr Point<R, D> fmap(const F &f, const Point &x,
                                    const Point<Args, D> &...args) {
    return Point<R, D>::make(
        [&](size_type d) { return f(x.elts[d], args.elts[d]...); });
  }
  /** Map a function over all components of one or several Points, returning
   * void
   *
   * Example:
   *   Point<int,3> pi;
   *   fmap_([](auto& i) { return i*=2; }, pi);
   * This doubles each component of pi, the same as pi *= 2 .
   */
  template <typename F, typename... Args>
  friend constexpr void fmap_(const F &f, const Point &x,
                              const Point<Args, D> &...args) {
    loop([&](size_type d) { f(x.elts[d], args.elts[d]...); });
  }

  /** Reduce over all components of one or several Points
   *
   * Example:
   *   Point<int,3> pi;
   *   int s = fold([](auto r, auto i) { return r+i; }, 0, pi);
   * This calculates the sum of all components ("horizonal sum") of pi,
   * same as sum(pi).
   */
  template <typename Op, typename R, typename... Args,
            typename = std::remove_cv_t<
                std::remove_reference_t<std::result_of_t<Op(R, T, Args...)>>>>
  friend constexpr R fold(const Op &op, R r, const Point &x,
                          const Point<Args, D> &...args) {
    loop([&](int d) { r = op(r, x[d], args[d]...); });
    return r;
  }

  /** Create a point from a point with different component type
   */
  template <typename U>
  constexpr Point(const Point<U, D> &x)
      : elts(fmap([](const U &a) { return T(a); }, x)) {}

  /** Create a point from Pointers to first and one past the last element
   */
  constexpr Point(const T *begin, const T *end __attribute__((__unused__)))
      : elts((assert(begin + D == end),
              make([&](size_type d) { return begin[d]; }))) {}
  /** Create a point from an initializer list
   *
   * Example: Point<int,3>{1,2,3}
   */
  constexpr Point(std::initializer_list<T> lst)
      : Point(lst.begin(), lst.end()) {}
  /** Create a point from a C-style array
   */
  template <std::size_t DD = D, std::enable_if_t<DD != 0> * = nullptr>
  constexpr Point(const T (&arr)[DD]) : elts(&arr[0], &arr[D]) {}
  /** Create a point from a std::array
   */
  constexpr Point(const std::array<T, D> &arr) : elts(arr) {}
  /** Create a point from a std::vector
   */
  template <typename TT = T,
            std::enable_if_t<!std::is_same_v<TT, bool>> * = nullptr>
  constexpr Point(const std::vector<T> &vec)
      : Point(&*vec.begin(), &*vec.end()) {}
  constexpr Point(const std::vector<bool> &vec)
      : Point((assert(vec.size() == D),
               make([&](size_type d) { return vec[d]; }))) {}

  /** Convert a point to a std::array
   */
  constexpr operator std::array<T, D>() const { return elts; }
  /** Convert a point to a std::vector
   */
  explicit constexpr operator std::vector<T>() const {
    return std::vector<T>(elts.begin(), elts.end());
  }

  /** Number of components (same as number of dimensions)
   */
  constexpr size_type size() const { return D; }

  /** Number of dimensions (same as number of components)
   */
  constexpr size_type ndims() const { return D; }

  /** Get a component of a point
   */
  constexpr const T &operator[](const size_type d) const { return elts[d]; }
  /** Get a component of a point
   */
  constexpr T &operator[](const size_type d) { return elts[d]; }

  /** Remove a component from a point
   *
   * This reduces the dimension of a point by one.
   */
  constexpr Point<T, (D > 0 ? D - 1 : 0)> erase(size_type dir) const {
    assert(dir >= 0 && dir < D);
    return Point<T, (D > 0 ? D - 1 : 0)>::make(
        [&](size_type d) { return d < dir ? (*this)[d] : (*this)[d + 1]; });
  }
  /** Add a component to a point
   *
   * This increases the dimension of a point by one.
   */
  constexpr Point<T, D + 1> insert(size_type dir, const T &a) const {
    assert(dir >= 0 && dir <= D);
    return Point<T, D + 1>::make([&](size_type d) {
      return d < dir ? (*this)[d] : d == dir ? a : (*this)[d - 1];
    });
  }

  /** Reverse the components of a point
   */
  constexpr Point reversed() const {
    return Point::make([&](size_type d) { return (*this)[D - 1 - d]; });
  }

  friend constexpr Point operator+(const Point &x) {
    return fmap([](const T &a) { return +a; }, x);
  }
  friend constexpr Point operator-(const Point &x) {
    return fmap([](const T &a) { return -a; }, x);
  }
  friend constexpr Point operator~(const Point &x) {
    if constexpr (std::is_same_v<T, bool>)
      // Special case to avoid compiler warnings
      return fmap([](const T &) { return true; }, x);
    else if constexpr (std::is_integral_v<T>)
      return fmap([](const T &a) { return ~a; }, x);
    std::abort();
  }
  friend constexpr Point<bool, D> operator!(const Point &x) {
    return fmap([](const T &a) { return !a; }, x);
  }

  friend constexpr Point operator+(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a + b; }, x, y);
  }
  friend constexpr Point operator-(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a - b; }, x, y);
  }
  friend constexpr Point operator*(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a * b; }, x, y);
  }
  friend constexpr Point operator/(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a / b; }, x, y);
  }
  friend constexpr Point operator%(const Point &x, const Point &y) {
    if constexpr (std::is_integral_v<T>)
      return fmap([](const T &a, const T &b) { return a % b; }, x, y);
    std::abort();
  }
  friend constexpr Point operator&(const Point &x, const Point &y) {
    if constexpr (std::is_integral_v<T>)
      return fmap([](const T &a, const T &b) { return a & b; }, x, y);
    std::abort();
  }
  friend constexpr Point operator|(const Point &x, const Point &y) {
    if constexpr (std::is_integral_v<T>)
      return fmap([](const T &a, const T &b) { return a | b; }, x, y);
    std::abort();
  }
  friend constexpr Point operator^(const Point &x, const Point &y) {
    if constexpr (std::is_integral_v<T>)
      return fmap([](const T &a, const T &b) { return a ^ b; }, x, y);
    std::abort();
  }
  friend constexpr Point<bool, D> operator&&(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a && b; }, x, y);
  }
  friend constexpr Point<bool, D> operator||(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a || b; }, x, y);
  }

  friend constexpr Point operator+(const T &a, const Point &y) {
    return fmap([&](const T &b) { return a + b; }, y);
  }
  friend constexpr Point operator-(const T &a, const Point &y) {
    return fmap([&](const T &b) { return a - b; }, y);
  }
  friend constexpr Point operator*(const T &a, const Point &y) {
    return fmap([&](const T &b) { return a * b; }, y);
  }
  friend constexpr Point operator/(const T &a, const Point &y) {
    return fmap([&](const T &b) { return a / b; }, y);
  }
  friend constexpr Point operator%(const T &a, const Point &y) {
    if constexpr (std::is_integral_v<T>)
      return fmap([&](const T &b) { return a % b; }, y);
    std::abort();
  }
  friend constexpr Point operator&(const T &a, const Point &y) {
    if constexpr (std::is_integral_v<T>)
      return fmap([&](const T &b) { return a & b; }, y);
    std::abort();
  }
  friend constexpr Point operator|(const T &a, const Point &y) {
    if constexpr (std::is_integral_v<T>)
      return fmap([&](const T &b) { return a | b; }, y);
    std::abort();
  }
  friend constexpr Point operator^(const T &a, const Point &y) {
    if constexpr (std::is_integral_v<T>)
      return fmap([&](const T &b) { return a ^ b; }, y);
    std::abort();
  }
  friend constexpr Point<bool, D> operator&&(const T &a, const Point &y) {
    return fmap([&](const T &b) { return a && b; }, y);
  }
  friend constexpr Point<bool, D> operator||(const T &a, const Point &y) {
    return fmap([&](const T &b) { return a || b; }, y);
  }

  friend constexpr Point operator+(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a + b; }, x);
  }
  friend constexpr Point operator-(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a - b; }, x);
  }
  friend constexpr Point operator*(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a * b; }, x);
  }
  friend constexpr Point operator/(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a / b; }, x);
  }
  friend constexpr Point operator%(const Point &x, const T &b) {
    if constexpr (std::is_integral_v<T>)
      return fmap([&](const T &a) { return a % b; }, x);
    std::abort();
  }
  friend constexpr Point operator&(const Point &x, const T &b) {
    if constexpr (std::is_integral_v<T>)
      return fmap([&](const T &a) { return a & b; }, x);
    std::abort();
  }
  friend constexpr Point operator|(const Point &x, const T &b) {
    if constexpr (std::is_integral_v<T>)
      return fmap([&](const T &a) { return a | b; }, x);
    std::abort();
  }
  friend constexpr Point operator^(const Point &x, const T &b) {
    if constexpr (std::is_integral_v<T>)
      return fmap([&](const T &a) { return a ^ b; }, x);
    std::abort();
  }
  friend constexpr Point<bool, D> operator&&(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a && b; }, x);
  }
  friend constexpr Point<bool, D> operator||(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a || b; }, x);
  }

  constexpr Point &operator+=(const Point &x) { return *this = *this + x; }
  constexpr Point &operator-=(const Point &x) { return *this = *this - x; }
  constexpr Point &operator*=(const Point &x) { return *this = *this * x; }
  constexpr Point &operator/=(const Point &x) { return *this = *this / x; }
  constexpr Point &operator%=(const Point &x) { return *this = *this % x; }
  constexpr Point &operator&=(const Point &x) { return *this = *this & x; }
  constexpr Point &operator|=(const Point &x) { return *this = *this | x; }
  constexpr Point &operator^=(const Point &x) { return *this = *this ^ x; }

  constexpr Point &operator+=(const T &a) { return *this = *this + a; }
  constexpr Point &operator-=(const T &a) { return *this = *this - a; }
  constexpr Point &operator*=(const T &a) { return *this = *this * a; }
  constexpr Point &operator/=(const T &a) { return *this = *this / a; }
  constexpr Point &operator%=(const T &a) { return *this = *this % a; }
  constexpr Point &operator&=(const T &a) { return *this = *this & a; }
  constexpr Point &operator|=(const T &a) { return *this = *this | a; }
  constexpr Point &operator^=(const T &a) { return *this = *this ^ a; }

  friend constexpr Point abs(const Point &x) {
    using std::abs;
    return fmap(
        [&](const T &a) {
          if constexpr (std::is_same_v<T, bool>)
            return a;
          else
            return abs(a);
        },
        x);
  }
  friend constexpr Point fabs(const Point &x) {
    using std::fabs;
    return fmap([&](const T &a) { return fabs(a); }, x);
  }

  friend constexpr Point fmax(const Point &x, const Point &y) {
    using std::fmax;
    return fmap([](const T &a, const T &b) { return fmax(a, b); }, x, y);
  }
  friend constexpr Point fmin(const Point &x, const Point &y) {
    using std::fmin;
    return fmap([](const T &a, const T &b) { return fmin(a, b); }, x, y);
  }
  friend constexpr Point max(const Point &x, const Point &y) {
    using std::max;
    return fmap([](const T &a, const T &b) { return max(a, b); }, x, y);
  }
  friend constexpr Point min(const Point &x, const Point &y) {
    using std::min;
    return fmap([](const T &a, const T &b) { return min(a, b); }, x, y);
  }

  friend constexpr Point fmax(const T &a, const Point &y) {
    using std::fmax;
    return fmap([&](const T &b) { return fmax(a, b); }, y);
  }
  friend constexpr Point fmin(const T &a, const Point &y) {
    using std::fmin;
    return fmap([&](const T &b) { return fmin(a, b); }, y);
  }
  friend constexpr Point max(const T &a, const Point &y) {
    using std::max;
    return fmap([&](const T &b) { return max(a, b); }, y);
  }
  friend constexpr Point min(const T &a, const Point &y) {
    using std::min;
    return fmap([&](const T &b) { return min(a, b); }, y);
  }

  friend constexpr Point fmax(const Point &x, const T &b) {
    using std::fmax;
    return fmap([&](const T &a) { return fmax(a, b); }, x);
  }
  friend constexpr Point fmin(const Point &x, const T &b) {
    using std::fmin;
    return fmap([&](const T &a) { return fmin(a, b); }, x);
  }
  friend constexpr Point max(const Point &x, const T &b) {
    using std::max;
    return fmap([&](const T &a) { return max(a, b); }, x);
  }
  friend constexpr Point min(const Point &x, const T &b) {
    using std::min;
    return fmap([&](const T &a) { return min(a, b); }, x);
  }

  friend constexpr bool all(const Point &x) {
    return fold([](bool r, const T &a) { return r && a; }, true, x);
  }
  friend constexpr bool any(const Point &x) {
    return fold([](bool r, const T &a) { return r || a; }, false, x);
  }
  friend constexpr T max_element(const Point &x) {
    using std::max;
    const T neutral = std::is_floating_point_v<T>
                          ? -std::numeric_limits<T>::infinity()
                          : std::numeric_limits<T>::lowest();
    return fold([](const T &r, const T &a) { return max(r, a); }, neutral, x);
  }
  friend constexpr T min_element(const Point &x) {
    using std::min;
    const T neutral = std::is_floating_point_v<T>
                          ? std::numeric_limits<T>::infinity()
                          : std::numeric_limits<T>::max();
    return fold([](const T &r, const T &a) { return min(r, a); }, neutral, x);
  }
  friend constexpr T product(const Point &x) {
    return fold([](const T &r, const T &a) { return r * a; }, T(1), x);
  }
  friend constexpr T sum(const Point &x) {
    return fold([](const T &r, const T &a) { return r + a; }, T(0), x);
  }

  friend constexpr Point<bool, D> operator==(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a == b; }, x, y);
  }
  friend constexpr Point<bool, D> operator!=(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a != b; }, x, y);
  }
  friend constexpr Point<bool, D> operator<(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a < b; }, x, y);
  }
  friend constexpr Point<bool, D> operator>(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a > b; }, x, y);
  }
  friend constexpr Point<bool, D> operator<=(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a <= b; }, x, y);
  }
  friend constexpr Point<bool, D> operator>=(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a >= b; }, x, y);
  }

  friend constexpr Point<bool, D> operator==(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a == b; }, x);
  }
  friend constexpr Point<bool, D> operator!=(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a != b; }, x);
  }
  friend constexpr Point<bool, D> operator<(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a < b; }, x);
  }
  friend constexpr Point<bool, D> operator>(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a > b; }, x);
  }
  friend constexpr Point<bool, D> operator<=(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a <= b; }, x);
  }
  friend constexpr Point<bool, D> operator>=(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a >= b; }, x);
  }

  /** Output a point
   */
  friend std::ostream &operator<<(std::ostream &os, const Point &x) {
    os << "[";
    for (std::size_t d = 0; d < D; ++d) {
      if (d != 0)
        os << ",";
      os << x[d];
    }
    os << "]";
    return os;
  }
};

} // namespace Regions
} // namespace openPMD

namespace std {

template <typename T, std::size_t D>
struct equal_to<openPMD::Regions::Point<T, D>> {
  constexpr bool operator()(const openPMD::Regions::Point<T, D> &x,
                            const openPMD::Regions::Point<T, D> &y) const {
    const equal_to<std::array<T, D>> eq;
    return eq(x.elts, y.elts);
  }
};

template <typename T, std::size_t D>
struct hash<openPMD::Regions::Point<T, D>> {
  constexpr size_t operator()(const openPMD::Regions::Point<T, D> &x) const {
    const hash<T> h;
    return fold(
        [&](size_t r, const T &b) {
          return openPMD::Regions::helpers::hash_combine(r, h(b));
        },
        size_t(0xb22da17173243869ULL), x);
  }
};

template <typename T, std::size_t D>
struct less<openPMD::Regions::Point<T, D>> {
  constexpr bool operator()(const openPMD::Regions::Point<T, D> &x,
                            const openPMD::Regions::Point<T, D> &y) const {
    const less<std::array<T, D>> lt;
    return lt(x.elts, y.elts);
  }
};

} // namespace std

#endif // #ifndef REGIONS_POINT_HPP
