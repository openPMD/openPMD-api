#ifndef REGIONS_NDPOINT_HPP
#define REGIONS_NDPOINT_HPP

#include "Helpers.hpp"
#include "Point.hpp"

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
namespace Regions {

/** Maximum number of dimensions supported by NDPoint
 */
constexpr std::size_t max_ndims = 5;

namespace detail {

// Abstract base helper class

template <typename T> class VPoint {
public:
  typedef T value_type;
  typedef std::ptrdiff_t size_type;

  virtual std::unique_ptr<VPoint> copy() const = 0;

  virtual ~VPoint() {}

  virtual std::unique_ptr<VPoint>
  fmap1(const std::function<T(const T &)> &f) const = 0;
  virtual std::unique_ptr<VPoint>
  fmap1(const std::function<T(const T &, const T &)> &f,
        const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint>
  fmap1(const std::function<T(const T &, const T &, const T &)> &f,
        const VPoint &x, const VPoint &y) const = 0;
  virtual T fold1(const std::function<T(const T &, const T &)> &op,
                  const T &r) const = 0;
  virtual T fold1(const std::function<T(const T &, const T &, const T &)> &op,
                  const T &r, const VPoint &x) const = 0;

  virtual void set_from_vector(const std::vector<T> &vec) = 0;
  virtual operator std::vector<T>() const = 0;

  virtual size_type size() const = 0;

  virtual size_type ndims() const = 0;
  virtual const T &operator[](const size_type d) const = 0;
  virtual T &operator[](const size_type d) = 0;
  virtual std::unique_ptr<VPoint> erase(const size_type d) const = 0;
  virtual std::unique_ptr<VPoint> insert(const size_type d,
                                         const T &a) const = 0;
  virtual std::unique_ptr<VPoint> reversed() const = 0;

  virtual std::unique_ptr<VPoint> operator+() const = 0;
  virtual std::unique_ptr<VPoint> operator-() const = 0;
  virtual std::unique_ptr<VPoint> operator~() const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator!() const = 0;

  virtual std::unique_ptr<VPoint> operator+(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint> operator-(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint> operator*(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint> operator/(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint> operator%(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint> operator&(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint> operator|(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint> operator^(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator&&(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator||(const VPoint &x) const = 0;

  virtual std::unique_ptr<VPoint> left_plus(const T &a) const = 0;
  virtual std::unique_ptr<VPoint> left_minus(const T &a) const = 0;
  virtual std::unique_ptr<VPoint> left_multiplies(const T &a) const = 0;
  virtual std::unique_ptr<VPoint> left_divides(const T &a) const = 0;
  virtual std::unique_ptr<VPoint> left_modulus(const T &a) const = 0;
  virtual std::unique_ptr<VPoint> left_bit_and(const T &a) const = 0;
  virtual std::unique_ptr<VPoint> left_bit_or(const T &a) const = 0;
  virtual std::unique_ptr<VPoint> left_bit_xor(const T &a) const = 0;
  virtual std::unique_ptr<VPoint<bool>> left_logical_and(const T &a) const = 0;
  virtual std::unique_ptr<VPoint<bool>> left_logical_or(const T &a) const = 0;

  virtual std::unique_ptr<VPoint> operator+(const T &b) const = 0;
  virtual std::unique_ptr<VPoint> operator-(const T &b) const = 0;
  virtual std::unique_ptr<VPoint> operator*(const T &b) const = 0;
  virtual std::unique_ptr<VPoint> operator/(const T &b) const = 0;
  virtual std::unique_ptr<VPoint> operator%(const T &b) const = 0;
  virtual std::unique_ptr<VPoint> operator&(const T &b) const = 0;
  virtual std::unique_ptr<VPoint> operator|(const T &b) const = 0;
  virtual std::unique_ptr<VPoint> operator^(const T &b) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator&&(const T &b) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator||(const T &b) const = 0;

  virtual VPoint &operator+=(const VPoint &x) = 0;
  virtual VPoint &operator-=(const VPoint &x) = 0;
  virtual VPoint &operator*=(const VPoint &x) = 0;
  virtual VPoint &operator/=(const VPoint &x) = 0;
  virtual VPoint &operator%=(const VPoint &x) = 0;
  virtual VPoint &operator&=(const VPoint &x) = 0;
  virtual VPoint &operator|=(const VPoint &x) = 0;
  virtual VPoint &operator^=(const VPoint &x) = 0;

  virtual VPoint &operator+=(const T &b) = 0;
  virtual VPoint &operator-=(const T &b) = 0;
  virtual VPoint &operator*=(const T &b) = 0;
  virtual VPoint &operator/=(const T &b) = 0;
  virtual VPoint &operator%=(const T &b) = 0;
  virtual VPoint &operator&=(const T &b) = 0;
  virtual VPoint &operator|=(const T &b) = 0;
  virtual VPoint &operator^=(const T &b) = 0;

  virtual std::unique_ptr<VPoint> abs1() const = 0;
  virtual std::unique_ptr<VPoint> fabs1() const = 0;

  virtual std::unique_ptr<VPoint> fmax1(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint> fmin1(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint> max1(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint> min1(const VPoint &x) const = 0;

  virtual std::unique_ptr<VPoint> fmax1(const T &b) const = 0;
  virtual std::unique_ptr<VPoint> fmin1(const T &b) const = 0;
  virtual std::unique_ptr<VPoint> max1(const T &b) const = 0;
  virtual std::unique_ptr<VPoint> min1(const T &b) const = 0;

  virtual T all1() const = 0;
  virtual T any1() const = 0;
  virtual T max_element1() const = 0;
  virtual T min_element1() const = 0;
  virtual T product1() const = 0;
  virtual T sum1() const = 0;

  virtual std::unique_ptr<VPoint<bool>> operator==(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator!=(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator<(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator>(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator<=(const VPoint &x) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator>=(const VPoint &x) const = 0;

  virtual std::unique_ptr<VPoint<bool>> operator==(const T &a) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator!=(const T &a) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator<(const T &a) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator>(const T &a) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator<=(const T &a) const = 0;
  virtual std::unique_ptr<VPoint<bool>> operator>=(const T &a) const = 0;

  virtual bool equal_to1(const VPoint &x) const = 0;
  virtual std::size_t hash1() const = 0;
  virtual bool less1(const VPoint &x) const = 0;

  virtual std::ostream &output(std::ostream &os) const = 0;
};

// Helper class wrapping Point<T,D>

template <typename T, std::size_t D> class WPoint final : public VPoint<T> {
  Point<T, D> p;

public:
  using typename VPoint<T>::value_type;
  using typename VPoint<T>::size_type;

  WPoint() : p{} {}

  WPoint(const WPoint &x) = default;
  WPoint(WPoint &&) = default;
  WPoint &operator=(const WPoint &) = default;
  WPoint &operator=(WPoint &&) = default;

  WPoint(const Point<T, D> &p_) : p(p_) {}
  WPoint(Point<T, D> &&p_) : p(std::move(p_)) {}
  operator Point<T, D>() const { return p; }

  WPoint(const std::array<T, D> &arr) : p(arr) {}
  WPoint(std::array<T, D> &&arr) : p(std::move(arr)) {}
  operator std::array<T, D>() const { return p; }

  std::unique_ptr<VPoint<T>> copy() const override {
    return std::make_unique<WPoint>(*this);
  }

  ~WPoint() override {}

  std::unique_ptr<VPoint<T>>
  fmap1(const std::function<T(const T &)> &f) const override {
    return std::make_unique<WPoint>(fmap(f, p));
  }
  std::unique_ptr<VPoint<T>>
  fmap1(const std::function<T(const T &, const T &)> &f,
        const VPoint<T> &x) const override {
    return std::make_unique<WPoint>(
        fmap(f, p, dynamic_cast<const WPoint &>(x).p));
  }
  std::unique_ptr<VPoint<T>>
  fmap1(const std::function<T(const T &, const T &, const T &)> &f,
        const VPoint<T> &x, const VPoint<T> &y) const override {
    return std::make_unique<WPoint>(fmap(f, p,
                                         dynamic_cast<const WPoint &>(x).p,
                                         dynamic_cast<const WPoint &>(y).p));
  }
  T fold1(const std::function<T(const T &, const T &)> &op,
          const T &r) const override {
    return fold(op, r, p);
  }
  T fold1(const std::function<T(const T &, const T &, const T &)> &op,
          const T &r, const VPoint<T> &x) const override {
    return fold(op, r, p, dynamic_cast<const WPoint &>(x).p);
  }

  void set_from_vector(const std::vector<T> &vec) override {
    *this = WPoint(vec);
  }
  operator std::vector<T>() const override { return std::vector<T>(p); }

  /*constexpr*/ size_type size() const override { return p.size(); }

  /*constexpr*/ size_type ndims() const override { return p.ndims(); }

  const T &operator[](const size_type d) const override { return p[d]; }
  T &operator[](const size_type d) override { return p[d]; }

  std::unique_ptr<VPoint<T>> erase(const size_type d
                                   [[maybe_unused]]) const override {
    if constexpr (D == 0)
      return std::unique_ptr<WPoint<T, D>>();
    else
      return std::make_unique<WPoint<T, D - 1>>(p.erase(d));
  }
  std::unique_ptr<VPoint<T>> insert(const size_type d [[maybe_unused]],
                                    const T &a) const override {
    if constexpr (D == max_ndims)
      return std::unique_ptr<WPoint<T, D>>();
    else
      return std::make_unique<WPoint<T, D + 1>>(p.insert(d, a));
  }

  std::unique_ptr<VPoint<T>> reversed() const override {
    return std::make_unique<WPoint>(p.reversed());
  }

  std::unique_ptr<VPoint<T>> operator+() const override {
    return std::make_unique<WPoint>(+p);
  }
  std::unique_ptr<VPoint<T>> operator-() const override {
    return std::make_unique<WPoint>(-p);
  }
  std::unique_ptr<VPoint<T>> operator~() const override {
    return std::make_unique<WPoint>(~p);
  }
  std::unique_ptr<VPoint<bool>> operator!() const override {
    return std::make_unique<WPoint<bool, D>>(!p);
  }

  std::unique_ptr<VPoint<T>> operator+(const VPoint<T> &x) const override {
    return std::make_unique<WPoint>(p + dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<T>> operator-(const VPoint<T> &x) const override {
    return std::make_unique<WPoint>(p - dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<T>> operator*(const VPoint<T> &x) const override {
    return std::make_unique<WPoint>(p * dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<T>> operator/(const VPoint<T> &x) const override {
    return std::make_unique<WPoint>(p / dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<T>> operator%(const VPoint<T> &x) const override {
    return std::make_unique<WPoint>(p % dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<T>> operator&(const VPoint<T> &x) const override {
    return std::make_unique<WPoint>(p & dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<T>> operator|(const VPoint<T> &x) const override {
    return std::make_unique<WPoint>(p | dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<T>> operator^(const VPoint<T> &x) const override {
    return std::make_unique<WPoint>(p ^ dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<bool>> operator&&(const VPoint<T> &x) const override {
    return std::make_unique<WPoint<bool, D>>(p &&
                                             dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<bool>> operator||(const VPoint<T> &x) const override {
    return std::make_unique<WPoint<bool, D>>(p ||
                                             dynamic_cast<const WPoint &>(x).p);
  }

  std::unique_ptr<VPoint<T>> left_plus(const T &a) const override {
    return std::make_unique<WPoint>(a + p);
  }
  std::unique_ptr<VPoint<T>> left_minus(const T &a) const override {
    return std::make_unique<WPoint>(a - p);
  }
  std::unique_ptr<VPoint<T>> left_multiplies(const T &a) const override {
    return std::make_unique<WPoint>(a * p);
  }
  std::unique_ptr<VPoint<T>> left_divides(const T &a) const override {
    return std::make_unique<WPoint>(a / p);
  }
  std::unique_ptr<VPoint<T>> left_modulus(const T &a) const override {
    return std::make_unique<WPoint>(a % p);
  }
  std::unique_ptr<VPoint<T>> left_bit_and(const T &a) const override {
    return std::make_unique<WPoint>(a & p);
  }
  std::unique_ptr<VPoint<T>> left_bit_or(const T &a) const override {
    return std::make_unique<WPoint>(a | p);
  }
  std::unique_ptr<VPoint<T>> left_bit_xor(const T &a) const override {
    return std::make_unique<WPoint>(a ^ p);
  }
  std::unique_ptr<VPoint<bool>> left_logical_and(const T &a) const override {
    return std::make_unique<WPoint<bool, D>>(a && p);
  }
  std::unique_ptr<VPoint<bool>> left_logical_or(const T &a) const override {
    return std::make_unique<WPoint<bool, D>>(a || p);
  }

  std::unique_ptr<VPoint<T>> operator+(const T &b) const override {
    return std::make_unique<WPoint>(p + b);
  }
  std::unique_ptr<VPoint<T>> operator-(const T &b) const override {
    return std::make_unique<WPoint>(p - b);
  }
  std::unique_ptr<VPoint<T>> operator*(const T &b) const override {
    return std::make_unique<WPoint>(p * b);
  }
  std::unique_ptr<VPoint<T>> operator/(const T &b) const override {
    return std::make_unique<WPoint>(p / b);
  }
  std::unique_ptr<VPoint<T>> operator%(const T &b) const override {
    return std::make_unique<WPoint>(p % b);
  }
  std::unique_ptr<VPoint<T>> operator&(const T &b) const override {
    return std::make_unique<WPoint>(p & b);
  }
  std::unique_ptr<VPoint<T>> operator|(const T &b) const override {
    return std::make_unique<WPoint>(p | b);
  }
  std::unique_ptr<VPoint<T>> operator^(const T &b) const override {
    return std::make_unique<WPoint>(p ^ b);
  }
  std::unique_ptr<VPoint<bool>> operator&&(const T &b) const override {
    return std::make_unique<WPoint<bool, D>>(p && b);
  }
  std::unique_ptr<VPoint<bool>> operator||(const T &b) const override {
    return std::make_unique<WPoint<bool, D>>(p || b);
  }

  VPoint<T> &operator+=(const VPoint<T> &x) override {
    p += dynamic_cast<const WPoint &>(x).p;
    return *this;
  }
  VPoint<T> &operator-=(const VPoint<T> &x) override {
    p -= dynamic_cast<const WPoint &>(x).p;
    return *this;
  }
  VPoint<T> &operator*=(const VPoint<T> &x) override {
    p *= dynamic_cast<const WPoint &>(x).p;
    return *this;
  }
  VPoint<T> &operator/=(const VPoint<T> &x) override {
    p /= dynamic_cast<const WPoint &>(x).p;
    return *this;
  }
  VPoint<T> &operator%=(const VPoint<T> &x) override {
    p %= dynamic_cast<const WPoint &>(x).p;
    return *this;
  }
  VPoint<T> &operator&=(const VPoint<T> &x) override {
    p &= dynamic_cast<const WPoint &>(x).p;
    return *this;
  }
  VPoint<T> &operator|=(const VPoint<T> &x) override {
    p |= dynamic_cast<const WPoint &>(x).p;
    return *this;
  }
  VPoint<T> &operator^=(const VPoint<T> &x) override {
    p ^= dynamic_cast<const WPoint &>(x).p;
    return *this;
  }

  VPoint<T> &operator+=(const T &b) override {
    p += b;
    return *this;
  }
  VPoint<T> &operator-=(const T &b) override {
    p -= b;
    return *this;
  }
  VPoint<T> &operator*=(const T &b) override {
    p *= b;
    return *this;
  }
  VPoint<T> &operator/=(const T &b) override {
    p /= b;
    return *this;
  }
  VPoint<T> &operator%=(const T &b) override {
    p %= b;
    return *this;
  }
  VPoint<T> &operator&=(const T &b) override {
    p &= b;
    return *this;
  }
  VPoint<T> &operator|=(const T &b) override {
    p |= b;
    return *this;
  }
  VPoint<T> &operator^=(const T &b) override {
    p ^= b;
    return *this;
  }

  std::unique_ptr<VPoint<T>> abs1() const override {
    return std::make_unique<WPoint>(abs(p));
  }
  std::unique_ptr<VPoint<T>> fabs1() const override {
    return std::make_unique<WPoint>(fabs(p));
  }

  std::unique_ptr<VPoint<T>> fmax1(const VPoint<T> &x) const override {
    return std::make_unique<WPoint>(fmax(p, dynamic_cast<const WPoint &>(x).p));
  }
  std::unique_ptr<VPoint<T>> fmin1(const VPoint<T> &x) const override {
    return std::make_unique<WPoint>(fmin(p, dynamic_cast<const WPoint &>(x).p));
  }
  std::unique_ptr<VPoint<T>> max1(const VPoint<T> &x) const override {
    return std::make_unique<WPoint>(max(p, dynamic_cast<const WPoint &>(x).p));
  }
  std::unique_ptr<VPoint<T>> min1(const VPoint<T> &x) const override {
    return std::make_unique<WPoint>(min(p, dynamic_cast<const WPoint &>(x).p));
  }

  std::unique_ptr<VPoint<T>> fmax1(const T &b) const override {
    return std::make_unique<WPoint>(fmax(p, b));
  }
  std::unique_ptr<VPoint<T>> fmin1(const T &b) const override {
    return std::make_unique<WPoint>(fmin(p, b));
  }
  std::unique_ptr<VPoint<T>> max1(const T &b) const override {
    return std::make_unique<WPoint>(max(p, b));
  }
  std::unique_ptr<VPoint<T>> min1(const T &b) const override {
    return std::make_unique<WPoint>(min(p, b));
  }

  T all1() const override { return all(p); }
  T any1() const override { return any(p); }
  T max_element1() const override { return max_element(p); }
  T min_element1() const override { return min_element(p); }
  T product1() const override { return product(p); }
  T sum1() const override { return sum(p); }

  std::unique_ptr<VPoint<bool>> operator==(const VPoint<T> &x) const override {
    return std::make_unique<WPoint<bool, D>>(p ==
                                             dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<bool>> operator!=(const VPoint<T> &x) const override {
    return std::make_unique<WPoint<bool, D>>(p !=
                                             dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<bool>> operator<(const VPoint<T> &x) const override {
    return std::make_unique<WPoint<bool, D>>(p <
                                             dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<bool>> operator>(const VPoint<T> &x) const override {
    return std::make_unique<WPoint<bool, D>>(p >
                                             dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<bool>> operator<=(const VPoint<T> &x) const override {
    return std::make_unique<WPoint<bool, D>>(p <=
                                             dynamic_cast<const WPoint &>(x).p);
  }
  std::unique_ptr<VPoint<bool>> operator>=(const VPoint<T> &x) const override {
    return std::make_unique<WPoint<bool, D>>(p >=
                                             dynamic_cast<const WPoint &>(x).p);
  }

  std::unique_ptr<VPoint<bool>> operator==(const T &a) const override {
    return std::make_unique<WPoint<bool, D>>(p == a);
  }
  std::unique_ptr<VPoint<bool>> operator!=(const T &a) const override {
    return std::make_unique<WPoint<bool, D>>(p != a);
  }
  std::unique_ptr<VPoint<bool>> operator<(const T &a) const override {
    return std::make_unique<WPoint<bool, D>>(p < a);
  }
  std::unique_ptr<VPoint<bool>> operator>(const T &a) const override {
    return std::make_unique<WPoint<bool, D>>(p > a);
  }
  std::unique_ptr<VPoint<bool>> operator<=(const T &a) const override {
    return std::make_unique<WPoint<bool, D>>(p <= a);
  }
  std::unique_ptr<VPoint<bool>> operator>=(const T &a) const override {
    return std::make_unique<WPoint<bool, D>>(p >= a);
  }

  bool equal_to1(const VPoint<T> &x) const override {
    return std::equal_to<Point<T, D>>()(p, dynamic_cast<const WPoint &>(x).p);
  }
  std::size_t hash1() const override { return std::hash<Point<T, D>>()(p); }
  bool less1(const VPoint<T> &x) const override {
    return std::less<Point<T, D>>()(p, dynamic_cast<const WPoint &>(x).p);
  }

  std::ostream &output(std::ostream &os) const override { return os << p; }
};

template <typename T>
std::unique_ptr<VPoint<T>> make_VPoint(const std::size_t D) {
  switch (D) {
  case 0:
    return std::make_unique<WPoint<T, 0>>();
  case 1:
    return std::make_unique<WPoint<T, 1>>();
  case 2:
    return std::make_unique<WPoint<T, 2>>();
  case 3:
    return std::make_unique<WPoint<T, 3>>();
  case 4:
    return std::make_unique<WPoint<T, 4>>();
  case 5:
    return std::make_unique<WPoint<T, 5>>();
  default:
    abort();
  }
}

template <typename T, typename F>
std::unique_ptr<VPoint<T>> make_VPoint(const std::size_t D, const F &f) {
  switch (D) {
  case 0:
    return std::make_unique<WPoint<T, 0>>(Point<T, 0>::make(f));
  case 1:
    return std::make_unique<WPoint<T, 1>>(Point<T, 1>::make(f));
  case 2:
    return std::make_unique<WPoint<T, 2>>(Point<T, 2>::make(f));
  case 3:
    return std::make_unique<WPoint<T, 3>>(Point<T, 3>::make(f));
  case 4:
    return std::make_unique<WPoint<T, 4>>(Point<T, 4>::make(f));
  case 5:
    return std::make_unique<WPoint<T, 5>>(Point<T, 5>::make(f));
  default:
    abort();
  }
}

} // namespace detail

/** A Point
 *
 * The dimension (number of component) of the Point is only known at
 * run-time. @see Point
 *
 * Points can represent either Points or distances. Points are
 * fixed-size vectors that support arithmetic operations.
 */
template <typename T> class NDPoint {
  template <typename U> using VPoint = detail::VPoint<U>;

  template <typename U> friend class NDPoint;
  friend struct std::equal_to<NDPoint>;
  friend struct std::hash<NDPoint>;
  friend struct std::less<NDPoint>;

  std::unique_ptr<VPoint<T>> p;

  NDPoint(std::unique_ptr<VPoint<T>> p_) : p(std::move(p_)) {}

public:
  /** Component type
   */
  typedef typename VPoint<T>::value_type value_type;
  /** Return type of Point::size()
   */
  typedef typename VPoint<T>::size_type size_type;

  /** Create an invalid Point
   */
  NDPoint() : p() {}
  /** Create a value-initialized Point with D components
   */
  NDPoint(const size_type D) : p(detail::make_VPoint<T>(D)) {}

  NDPoint(const NDPoint &x) : p(x.p ? x.p->copy() : nullptr) {}
  NDPoint(NDPoint &&) = default;
  NDPoint &operator=(const NDPoint &x) {
    p = x.p ? x.p->copy() : nullptr;
    return *this;
  }
  NDPoint &operator=(NDPoint &&) = default;

  template <std::size_t D>
  NDPoint(const Point<T, D> &p_)
      : p(std::make_unique<detail::WPoint<T, D>>(p_)) {}
  template <std::size_t D>
  NDPoint(Point<T, D> &&p_)
      : p(std::make_unique<detail::WPoint<T, D>>(std::move(p_))) {}
  template <std::size_t D> operator Point<T, D>() const {
    return Point<T, D>(dynamic_cast<const detail::WPoint<T, D>&>(*p));
  }

  template <std::size_t D>
  NDPoint(const std::array<T, D> &arr)
      : p(std::make_unique<detail::WPoint<T, D>>(arr)) {}
  template <std::size_t D>
  NDPoint(std::array<T, D> &&arr)
      : p(std::make_unique<detail::WPoint<T, D>>(std::move(arr))) {}
  template <std::size_t D> operator std::array<T, D>() const {
    return std::array<T, D>(dynamic_cast<const detail::WPoint<T, D>>(&p));
  }
  NDPoint(std::initializer_list<T> lst) : NDPoint(std::vector<T>(lst)) {}

private:
  template <typename U>
  static std::vector<T> convert_vector(const std::vector<U> &vec) {
    std::vector<T> res(vec.size());
    for (std::size_t n = 0; n < res.size(); ++n)
      res[n] = T(vec[n]);
    return res;
  }

public:
  template <typename U>
  NDPoint(const NDPoint<U> &x)
      : p(x.p ? NDPoint(convert_vector(std::vector<U>(x))) : nullptr) {}

  template <typename F> static NDPoint make(const size_type D, const F &f) {
    return NDPoint(detail::make_VPoint<T>(D, f));
  }
  static NDPoint pure(const size_type D, const T &val) {
    return make(D, [&](size_type) { return val; });
  }
  static NDPoint unit(const size_type D, const size_type dir) {
    return make(D, [&](size_type d) { return d == dir; });
  }
  static NDPoint iota(const size_type D) {
    return make(D, [&](size_type d) { return d; });
  }

  /** Check whether a Point is valid
   *
   * A valid Point knows its number of dimensions, and its components
   * are initialized. An invalid Point does not know its number of
   * dimensions and holds no data, similar to a null Pointer.
   *
   * Most other member functions must not be called for invalid
   * Points.
   */
  bool has_value() const { return bool(p); }

  template <typename F> friend NDPoint fmap(const F &f, const NDPoint &x) {
    return NDPoint(x.p->fmap1(f));
  }
  template <typename F>
  friend NDPoint fmap(const F &f, const NDPoint &x, const NDPoint &y) {
    return NDPoint(x.p->fmap1(f, *y.p));
  }
  template <typename F>
  friend NDPoint fmap(const F &f, const NDPoint &x, const NDPoint &y,
                      const NDPoint &z) {
    return NDPoint(x.p->fmap1(f, *y.p, *z.p));
  }
  template <typename Op>
  friend T fold(const Op &op, const T &r, const NDPoint &x) {
    return x.p->fold1(op, r);
  }
  template <typename Op>
  friend T fold(const Op &op, const T &r, const NDPoint &x, const NDPoint &y) {
    return x.p->fold1(op, r, *y.p);
  }

  NDPoint(const std::vector<T> &vec) : NDPoint(vec.size()) {
    p->set_from_vector(vec);
  }
  operator std::vector<T>() const { return std::vector<T>(p); }

  /** Number of comopnents (same as number of dimensions)
   */
  size_type size() const { return p->size(); }

  /** Number of dimensions (same as number of comopnents)
   */
  size_type ndims() const { return p->ndims(); }

  /** Get a component of a Point
   */
  const T &operator[](const size_type d) const { return (*p)[d]; }
  /** Get a component of a Point
   */
  T &operator[](const size_type d) { return (*p)[d]; }
  /** Remove a component from a point
   *
   * This reduces the dimension of a point by one.
   */
  constexpr NDPoint erase(size_type dir) const {
    return NDPoint(p->erase(dir));
  }
  /** Add a component to a point
   *
   * This increases the dimension of a point by one.
   */
  constexpr NDPoint insert(size_type dir, const T &a) const {
    return NDPoint(p->insert(dir, a));
  }
  /** Reverse the components of a point
   */
  constexpr NDPoint reversed() const { return NDPoint(p->reversed()); }

  friend NDPoint operator+(const NDPoint &x) { return NDPoint(+*x.p); }
  friend NDPoint operator-(const NDPoint &x) { return NDPoint(-*x.p); }
  friend NDPoint operator~(const NDPoint &x) { return NDPoint(~*x.p); }
  friend NDPoint<bool> operator!(const NDPoint &x) {
    return NDPoint<bool>(!*x.p);
  }

  friend NDPoint operator+(const NDPoint &x, const NDPoint &y) {
    return NDPoint(*x.p + *y.p);
  }
  friend NDPoint operator-(const NDPoint &x, const NDPoint &y) {
    return NDPoint(*x.p - *y.p);
  }
  friend NDPoint operator*(const NDPoint &x, const NDPoint &y) {
    return NDPoint(*x.p * *y.p);
  }
  friend NDPoint operator/(const NDPoint &x, const NDPoint &y) {
    return NDPoint(*x.p / *y.p);
  }
  friend NDPoint operator%(const NDPoint &x, const NDPoint &y) {
    return NDPoint(*x.p % *y.p);
  }
  friend NDPoint operator&(const NDPoint &x, const NDPoint &y) {
    return NDPoint(*x.p & *y.p);
  }
  friend NDPoint operator|(const NDPoint &x, const NDPoint &y) {
    return NDPoint(*x.p | *y.p);
  }
  friend NDPoint operator^(const NDPoint &x, const NDPoint &y) {
    return NDPoint(*x.p ^ *y.p);
  }
  friend NDPoint<bool> operator&&(const NDPoint &x, const NDPoint &y) {
    return NDPoint(*x.p && *y.p);
  }
  friend NDPoint<bool> operator||(const NDPoint &x, const NDPoint &y) {
    return NDPoint(*x.p || *y.p);
  }

  friend NDPoint operator+(const T &a, const NDPoint &y) {
    return NDPoint(y.p->left_plus(a));
  }
  friend NDPoint operator-(const T &a, const NDPoint &y) {
    return NDPoint(y.p->left_minus(a));
  }
  friend NDPoint operator*(const T &a, const NDPoint &y) {
    return NDPoint(y.p->left_multiplies(a));
  }
  friend NDPoint operator/(const T &a, const NDPoint &y) {
    return NDPoint(y.p->left_divides(a));
  }
  friend NDPoint operator%(const T &a, const NDPoint &y) {
    return NDPoint(y.p->left_modulus(a));
  }
  friend NDPoint operator&(const T &a, const NDPoint &y) {
    return NDPoint(y.p->left_bit_and(a));
  }
  friend NDPoint operator|(const T &a, const NDPoint &y) {
    return NDPoint(y.p->left_bit_or(a));
  }
  friend NDPoint operator^(const T &a, const NDPoint &y) {
    return NDPoint(y.p->left_bit_xor(a));
  }
  friend NDPoint<bool> operator&&(const T &a, const NDPoint &y) {
    return NDPoint<bool>(y.p->left_logical_and(a));
  }
  friend NDPoint<bool> operator||(const T &a, const NDPoint &y) {
    return NDPoint<bool>(y.p->left_logical_or(a));
  }

  friend NDPoint operator+(const NDPoint &x, const T &b) {
    return NDPoint(*x.p + b);
  }
  friend NDPoint operator-(const NDPoint &x, const T &b) {
    return NDPoint(*x.p - b);
  }
  friend NDPoint operator*(const NDPoint &x, const T &b) {
    return NDPoint(*x.p * b);
  }
  friend NDPoint operator/(const NDPoint &x, const T &b) {
    return NDPoint(*x.p / b);
  }
  friend NDPoint operator%(const NDPoint &x, const T &b) {
    return NDPoint(*x.p % b);
  }
  friend NDPoint operator&(const NDPoint &x, const T &b) {
    return NDPoint(*x.p & b);
  }
  friend NDPoint operator|(const NDPoint &x, const T &b) {
    return NDPoint(*x.p | b);
  }
  friend NDPoint operator^(const NDPoint &x, const T &b) {
    return NDPoint(*x.p ^ b);
  }
  friend NDPoint<bool> operator&&(const NDPoint &x, const T &b) {
    return NDPoint(*x.p && b);
  }
  friend NDPoint<bool> operator||(const NDPoint &x, const T &b) {
    return NDPoint(*x.p || b);
  }

  NDPoint &operator+=(const NDPoint &x) {
    *p += *x.p;
    return *this;
  }
  NDPoint &operator-=(const NDPoint &x) {
    *p -= *x.p;
    return *this;
  }
  NDPoint &operator*=(const NDPoint &x) {
    *p *= *x.p;
    return *this;
  }
  NDPoint &operator/=(const NDPoint &x) {
    *p /= *x.p;
    return *this;
  }
  NDPoint &operator%=(const NDPoint &x) {
    *p %= *x.p;
    return *this;
  }
  NDPoint &operator&=(const NDPoint &x) {
    *p &= *x.p;
    return *this;
  }
  NDPoint &operator|=(const NDPoint &x) {
    *p |= *x.p;
    return *this;
  }
  NDPoint &operator^=(const NDPoint &x) {
    *p ^= *x.p;
    return *this;
  }

  NDPoint &operator+=(const T &a) {
    *p += a;
    return *this;
  }
  NDPoint &operator-=(const T &a) {
    *p -= a;
    return *this;
  }
  NDPoint &operator*=(const T &a) {
    *p *= a;
    return *this;
  }
  NDPoint &operator/=(const T &a) {
    *p /= a;
    return *this;
  }
  NDPoint &operator%=(const T &a) {
    *p %= a;
    return *this;
  }
  NDPoint &operator&=(const T &a) {
    *p &= a;
    return *this;
  }
  NDPoint &operator|=(const T &a) {
    *p |= a;
    return *this;
  }
  NDPoint &operator^=(const T &a) {
    *p ^= a;
    return *this;
  }

  friend NDPoint abs(const NDPoint &x) { return x.p->abs1(); }
  friend NDPoint fabs(const NDPoint &x) { return x.p->fabs1(); }

  friend NDPoint fmax(const NDPoint &x, const NDPoint &y) {
    return x.p->fmax1(*y.p);
  }
  friend NDPoint fmin(const NDPoint &x, const NDPoint &y) {
    return x.p->fmin1(*y.p);
  }
  friend NDPoint max(const NDPoint &x, const NDPoint &y) {
    return x.p->max1(*y.p);
  }
  friend NDPoint min(const NDPoint &x, const NDPoint &y) {
    return x.p->min1(*y.p);
  }

  friend NDPoint fmax(const T &a, const NDPoint &y) { return y.p->fmax1(a); }
  friend NDPoint fmin(const T &a, const NDPoint &y) { return y.p->fmin1(a); }
  friend NDPoint max(const T &a, const NDPoint &y) { return y.p->max1(a); }
  friend NDPoint min(const T &a, const NDPoint &y) { return y.p->min1(a); }

  friend NDPoint fmax(const NDPoint &x, const T &b) { return x.p->fmax1(b); }
  friend NDPoint fmin(const NDPoint &x, const T &b) { return x.p->fmin1(b); }
  friend NDPoint max(const NDPoint &x, const T &b) { return x.p->max1(b); }
  friend NDPoint min(const NDPoint &x, const T &b) { return x.p->min1(b); }

  friend T all(const NDPoint &x) { return x.p->all1(); }
  friend T any(const NDPoint &x) { return x.p->any1(); }
  friend T max_element(const NDPoint &x) { return x.p->max_element1(); }
  friend T min_element(const NDPoint &x) { return x.p->min_element1(); }
  friend T product(const NDPoint &x) { return x.p->product1(); }
  friend T sum(const NDPoint &x) { return x.p->sum1(); }

  friend NDPoint<bool> operator==(const NDPoint &x, const NDPoint &y) {
    return *x.p == *y.p;
  }
  friend NDPoint<bool> operator!=(const NDPoint &x, const NDPoint &y) {
    return *x.p != *y.p;
  }
  friend NDPoint<bool> operator<(const NDPoint &x, const NDPoint &y) {
    return *x.p < *y.p;
  }
  friend NDPoint<bool> operator>(const NDPoint &x, const NDPoint &y) {
    return *x.p > *y.p;
  }
  friend NDPoint<bool> operator<=(const NDPoint &x, const NDPoint &y) {
    return *x.p <= *y.p;
  }
  friend NDPoint<bool> operator>=(const NDPoint &x, const NDPoint &y) {
    return *x.p >= *y.p;
  }

  friend NDPoint<bool> operator==(const NDPoint &x, const T &b) {
    return *x.p == b;
  }
  friend NDPoint<bool> operator!=(const NDPoint &x, const T &b) {
    return *x.p != b;
  }
  friend NDPoint<bool> operator<(const NDPoint &x, const T &b) {
    return *x.p < b;
  }
  friend NDPoint<bool> operator>(const NDPoint &x, const T &b) {
    return *x.p > b;
  }
  friend NDPoint<bool> operator<=(const NDPoint &x, const T &b) {
    return *x.p <= b;
  }
  friend NDPoint<bool> operator>=(const NDPoint &x, const T &b) {
    return *x.p >= b;
  }

  /** Output a point
   */
  friend std::ostream &operator<<(std::ostream &os, const NDPoint &x) {
    if (x.p)
      x.p->output(os);
    else
      os << "[INVALID]";
    return os;
  }
};

} // namespace Regions
} // namespace openPMD

namespace std {

template <typename T> struct equal_to<openPMD::Regions::NDPoint<T>> {
  constexpr bool operator()(const openPMD::Regions::NDPoint<T> &x,
                            const openPMD::Regions::NDPoint<T> &y) const {
    if (!x.has_value() && !y.has_value())
      return true;
    if (!x.has_value() || !y.has_value())
      return false;
    return x.p->equal_to1(*y.p);
  }
};

template <typename T> struct hash<openPMD::Regions::NDPoint<T>> {
  constexpr size_t operator()(const openPMD::Regions::NDPoint<T> &x) const {
    if (!x.has_value())
      return size_t(0xf458b18eca40aef1ULL);
    return x.p->hash1();
  }
};

template <typename T> struct less<openPMD::Regions::NDPoint<T>> {
  constexpr bool operator()(const openPMD::Regions::NDPoint<T> &x,
                            const openPMD::Regions::NDPoint<T> &y) const {
    if (!x.has_value())
      return true;
    if (!y.has_value())
      return false;
    return x.p->less1(*y.p);
  }
};

} // namespace std

#endif // #ifndef REGIONS_NDPOINT_HPP
