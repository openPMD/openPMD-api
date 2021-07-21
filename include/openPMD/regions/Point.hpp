#pragma once

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
  using value_type = T;
  /** Return type of Point::size()
   */
  using size_type = std::ptrdiff_t;

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

  /** Create a Point with each component set to the same value `a`
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

  /** Create a Point with components set to the natural number
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

  /** Create a Point from a Point with different component type
   */
  template <typename U>
  constexpr Point(const Point<U, D> &x)
      : elts(fmap([](const U &a) { return T(a); }, x)) {}

  /** Create a Point from Pointers to first and one past the last element
   */
  constexpr Point(const T *begin, const T *end [[maybe_unused]])
      : elts((assert(begin + D == end),
              make([&](size_type d) { return begin[d]; }))) {}
  /** Create a Point from an initializer list
   *
   * Example: Point<int,3>{1,2,3}
   */
  constexpr Point(std::initializer_list<T> lst)
      : Point(lst.begin(), lst.end()) {}
  /** Create a Point from a C-style array
   */
  template <std::size_t DD = D, std::enable_if_t<DD != 0> * = nullptr>
  constexpr Point(const T (&arr)[DD]) : elts(&arr[0], &arr[D]) {}
  /** Create a Point from a std::array
   */
  constexpr Point(const std::array<T, D> &arr) : elts(arr) {}
  /** Create a Point from a std::vector
   */
  template <typename TT = T,
            std::enable_if_t<!std::is_same_v<TT, bool>> * = nullptr>
  constexpr Point(const std::vector<T> &vec)
      : Point(&*vec.begin(), &*vec.end()) {}
  constexpr Point(const std::vector<bool> &vec)
      : Point((assert(vec.size() == D),
               make([&](size_type d) { return vec[d]; }))) {}

  /** Convert a Point to a std::array
   */
  constexpr operator std::array<T, D>() const { return elts; }
  /** Convert a Point to a std::vector
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

  /** Get a component of a Point
   */
  constexpr const T &operator[](const size_type d) const { return elts[d]; }
  /** Get a component of a Point
   */
  constexpr T &operator[](const size_type d) { return elts[d]; }

  /** Remove a component from a Point
   *
   * This reduces the dimension of a Point by one.
   */
  constexpr Point<T, (D > 0 ? D - 1 : 0)> erase(size_type dir) const {
    assert(dir >= 0 && dir < D);
    return Point<T, (D > 0 ? D - 1 : 0)>::make(
        [&](size_type d) { return d < dir ? (*this)[d] : (*this)[d + 1]; });
  }
  /** Add a component to a Point
   *
   * This increases the dimension of a Point by one.
   */
  constexpr Point<T, D + 1> insert(size_type dir, const T &a) const {
    assert(dir >= 0 && dir <= D);
    return Point<T, D + 1>::make([&](size_type d) {
      return d < dir ? (*this)[d] : d == dir ? a : (*this)[d - 1];
    });
  }

  /** Reverse the components of a Point
   */
  constexpr Point reversed() const {
    return Point::make([&](size_type d) { return (*this)[D - 1 - d]; });
  }

  /** Apply unary plus operator element-wise
   */
  friend constexpr Point operator+(const Point &x) {
    return fmap([](const T &a) { return +a; }, x);
  }
  /** Element-wise negate
   */
  friend constexpr Point operator-(const Point &x) {
    return fmap([](const T &a) { return -a; }, x);
  }
  /** Element-wise bitwise not
   */
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  friend constexpr Point operator~(const Point &x) {
    if constexpr (std::is_same_v<T, bool>)
      // Special case to avoid compiler warnings
      return fmap([](const T &) { return true; }, x);
    else
      return fmap([](const T &a) { return ~a; }, x);
  }
  /** Element-wise logical not
   */
  friend constexpr Point<bool, D> operator!(const Point &x) {
    return fmap([](const T &a) { return !a; }, x);
  }

  /** Add element-wise
   */
  friend constexpr Point operator+(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a + b; }, x, y);
  }
  /** Subtract element-wise
   */
  friend constexpr Point operator-(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a - b; }, x, y);
  }
  /** Multiply element-wise
   */
  friend constexpr Point operator*(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a * b; }, x, y);
  }
  /** Divide element-wise
   */
  friend constexpr Point operator/(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a / b; }, x, y);
  }
  /** Element-wise modulo
   */
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  friend constexpr Point operator%(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a % b; }, x, y);
  }
  /** Element-wise bitwise and
   */
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  friend constexpr Point operator&(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a & b; }, x, y);
  }
  /** Element-wise bitwise or
   */
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  friend constexpr Point operator|(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a | b; }, x, y);
  }
  /** Element-wise bitwise exclusive or
   */
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  friend constexpr Point operator^(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a ^ b; }, x, y);
  }
  /** Element-wise logical and
   */
  friend constexpr Point<bool, D> operator&&(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a && b; }, x, y);
  }
  /** Element-wise logical or
   */
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
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  friend constexpr Point operator%(const T &a, const Point &y) {
    return fmap([&](const T &b) { return a % b; }, y);
  }
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  friend constexpr Point operator&(const T &a, const Point &y) {
    return fmap([&](const T &b) { return a & b; }, y);
  }
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  friend constexpr Point operator|(const T &a, const Point &y) {
    return fmap([&](const T &b) { return a | b; }, y);
  }
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  friend constexpr Point operator^(const T &a, const Point &y) {
    return fmap([&](const T &b) { return a ^ b; }, y);
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
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  friend constexpr Point operator%(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a % b; }, x);
  }
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  friend constexpr Point operator&(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a & b; }, x);
  }
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  friend constexpr Point operator|(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a | b; }, x);
  }
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  friend constexpr Point operator^(const Point &x, const T &b) {
    return fmap([&](const T &a) { return a ^ b; }, x);
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
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  constexpr Point &operator%=(const Point &x) {
    return *this = *this % x;
  }
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  constexpr Point &operator&=(const Point &x) {
    return *this = *this & x;
  }
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  constexpr Point &operator|=(const Point &x) {
    return *this = *this | x;
  }
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  constexpr Point &operator^=(const Point &x) {
    return *this = *this ^ x;
  }

  constexpr Point &operator+=(const T &a) { return *this = *this + a; }
  constexpr Point &operator-=(const T &a) { return *this = *this - a; }
  constexpr Point &operator*=(const T &a) { return *this = *this * a; }
  constexpr Point &operator/=(const T &a) { return *this = *this / a; }
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  constexpr Point &operator%=(const T &a) {
    return *this = *this % a;
  }
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  constexpr Point &operator&=(const T &a) {
    return *this = *this & a;
  }
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  constexpr Point &operator|=(const T &a) {
    return *this = *this | a;
  }
  template <typename TT = T,
            std::enable_if_t<std::is_integral_v<TT>> * = nullptr>
  constexpr Point &operator^=(const T &a) {
    return *this = *this ^ a;
  }

  /** Element-wise absolute value
   */
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
  /** Element-wise absolute value
   */
  friend constexpr Point fabs(const Point &x) {
    using std::fabs;
    return fmap([&](const T &a) { return fabs(a); }, x);
  }

  /** Element-wise maximum of two points
   */
  friend constexpr Point fmax(const Point &x, const Point &y) {
    using std::fmax;
    return fmap([](const T &a, const T &b) { return fmax(a, b); }, x, y);
  }
  /** Element-wise minimum of two points
   */
  friend constexpr Point fmin(const Point &x, const Point &y) {
    using std::fmin;
    return fmap([](const T &a, const T &b) { return fmin(a, b); }, x, y);
  }
  /** Element-wise maximum of two points
   */
  friend constexpr Point max(const Point &x, const Point &y) {
    using std::max;
    return fmap([](const T &a, const T &b) { return max(a, b); }, x, y);
  }
  /** Element-wise minimum of two points
   */
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

  /** Return true if all elements are true
   */
  friend constexpr bool all(const Point &x) {
    return fold([](bool r, const T &a) { return r && a; }, true, x);
  }
  /** Return true if any element is true
   */
  friend constexpr bool any(const Point &x) {
    return fold([](bool r, const T &a) { return r || a; }, false, x);
  }
  /** Return maximum element
   */
  friend constexpr T max_element(const Point &x) {
    using std::max;
    const T neutral = std::is_floating_point_v<T>
                          ? -std::numeric_limits<T>::infinity()
                          : std::numeric_limits<T>::lowest();
    return fold([](const T &r, const T &a) { return max(r, a); }, neutral, x);
  }
  /** Return minimum element
   */
  friend constexpr T min_element(const Point &x) {
    using std::min;
    const T neutral = std::is_floating_point_v<T>
                          ? std::numeric_limits<T>::infinity()
                          : std::numeric_limits<T>::max();
    return fold([](const T &r, const T &a) { return min(r, a); }, neutral, x);
  }
  /** Product of all elements
   */
  friend constexpr T product(const Point &x) {
    return fold([](const T &r, const T &a) { return r * a; }, T(1), x);
  }
  /** Sum of all elements
   */
  friend constexpr T sum(const Point &x) {
    return fold([](const T &r, const T &a) { return r + a; }, T(0), x);
  }

  /** Pointwise comparison
   */
  friend constexpr Point<bool, D> operator==(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a == b; }, x, y);
  }
  /** Pointwise comparison
   */
  friend constexpr Point<bool, D> operator!=(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a != b; }, x, y);
  }
  /** Pointwise comparison
   */
  friend constexpr Point<bool, D> operator<(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a < b; }, x, y);
  }
  /** Pointwise comparison
   */
  friend constexpr Point<bool, D> operator>(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a > b; }, x, y);
  }
  /** Pointwise comparison
   */
  friend constexpr Point<bool, D> operator<=(const Point &x, const Point &y) {
    return fmap([](const T &a, const T &b) { return a <= b; }, x, y);
  }
  /** Pointwise comparison
   */
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

  /** Output a Point
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

/** Specialization of `equal_to` for Point
 */
template <typename T, std::size_t D>
struct equal_to<openPMD::Regions::Point<T, D>> {
  constexpr bool operator()(const openPMD::Regions::Point<T, D> &x,
                            const openPMD::Regions::Point<T, D> &y) const {
    const equal_to<std::array<T, D>> eq;
    return eq(x.elts, y.elts);
  }
};

/** Specialization of `hash` for Point
 */
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

/** Specialization of `less` for Point
 */
template <typename T, std::size_t D>
struct less<openPMD::Regions::Point<T, D>> {
  constexpr bool operator()(const openPMD::Regions::Point<T, D> &x,
                            const openPMD::Regions::Point<T, D> &y) const {
    const less<std::array<T, D>> lt;
    return lt(x.elts, y.elts);
  }
};

} // namespace std
