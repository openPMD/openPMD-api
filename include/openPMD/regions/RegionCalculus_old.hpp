#ifndef REGIONCALCULUS_HPP
#define REGIONCALCULUS_HPP

// #include "Config.hpp"
// #include "Helpers.hpp"

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
#include <yaml-cpp/yaml.h>
#endif

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// Whether extra, expensive run-time tests should be enabled. Set to 0 or 1.
#define REGIONCALCULUS_DEBUG 0

// Whether the more complex and more efficient tree-based implementation should
// be used. Set to 0 or 1.
#define REGIONCALCULUS_TREE 1

namespace RegionCalculus {

using std::abs;
using std::array;
using std::equal_to;
using std::hash;
using std::is_sorted;
using std::less;
using std::make_pair;
using std::make_tuple;
using std::map;
using std::max;
using std::min;
using std::move;
using std::numeric_limits;
using std::ostream;
using std::pair;
using std::ptrdiff_t;
using std::size_t;
using std::sort;
using std::tuple;
using std::unique;
using std::unique_ptr;
using std::vector;

////////////////////////////////////////////////////////////////////////////////
// C++ helper functions
////////////////////////////////////////////////////////////////////////////////

// make_unique is only available in C++14
template <typename T, typename... Args>
std::unique_ptr<T> make_unique1(Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Combine hashes
template <typename T> size_t hash_combine(size_t seed, const T &x) {
  hash<T> h;
  // Taken from Boost
  return seed ^
         h(x) + size_t(0x00e6052366ac4440eULL) + (seed << 6) + (seed >> 2);
}

namespace detail {
// Abort when called
template <typename T> struct error {
  T operator()(const T &) const { std::abort(); }
  T operator()(const T &, const T &) const { std::abort(); }
  T operator()(const T &, const T &, const T &) const { std::abort(); }
  T &operator()(T &) const { std::abort(); }
  T &operator()(T &, const T &) const { std::abort(); }
  T &operator()(T &, const T &, const T &) const { std::abort(); }
};
} // namespace detail

// Call function object F if T is integral; otherwise, abort
template <typename T, typename F> struct call_if_integral {
  typedef typename T::value_type S;
  typedef typename std::conditional<std::is_integral<S>::value, F,
                                    detail::error<T>>::type type;
  constexpr T operator()(const T &x) const { return type()(x); }
  constexpr T operator()(const T &x, const T &y) const { return type()(x, y); }
  constexpr T operator()(const T &x, const T &y, const T &z) const {
    return type()(x, y, z);
  }
  T &operator()(T &x) const { return type()(x); }
  T &operator()(T &x, const T &y) const { return type()(x, y); }
  T &operator()(T &x, const T &y, const T &z) const { return type()(x, y, z); }
};

////////////////////////////////////////////////////////////////////////////////
// Numerical functions
////////////////////////////////////////////////////////////////////////////////

namespace detail {

template <typename T>
typename std::enable_if<!std::is_unsigned<T>::value, T>::type abs_wrapper(T x) {
  using std::abs;
  return abs(x);
}

template <typename T>
typename std::enable_if<std::is_unsigned<T>::value, T>::type abs_wrapper(T x) {
  return x;
}
} // namespace detail

////////////////////////////////////////////////////////////////////////////////
// Reduction
////////////////////////////////////////////////////////////////////////////////

// op is functional
template <typename Op, typename T,
          typename R = typename std::result_of<Op(T, T)>::type>
T reduce(Op &&op, vector<T> &xs) {
  assert(!xs.empty());
  for (size_t dist = 1; dist < xs.size(); dist *= 2)
    for (size_t i = 0; i + dist < xs.size(); i += 2 * dist)
      xs[i] = std::forward<Op>(op)(move(xs[i]), move(xs[i + dist]));
  return move(xs[0]);
}

// // op is assignment-like
// template <typename Op, typename T,
//           typename R = typename std::result_of<Op(T &, T)>::type>
// T reduce(Op &&op, vector<T> &xs) {
//   assert(!xs.empty());
//   for (size_t dist = 1; dist < xs.size(); dist *= 2)
//     for (size_t i = 0; i + dist < x.size(); i += 2 * dist)
//       std::forward<Op>(op)(xs[i], xs[i + dist]);
//   return move(xs[0]);
// }

template <typename F, typename Op, typename R, typename B, typename E>
R reduce(F &&f, Op &&op, R &&z, const B &b, const E &e) {
  vector<R> rs;
  for (auto i = b; i != e; ++i)
    rs.push_back(std::forward<F>(f)(*i));
  if (rs.empty())
    return std::forward<R>(z);
  return reduce(std::forward<Op>(op), rs);
}

template <typename F, typename Op, typename I,
          typename T = typename std::iterator_traits<I>::value_type,
          typename R1 = typename std::result_of<F(T)>::type,
          typename R = typename std::decay<R1>::type>
R reduce(F &&f, Op &&op, const I &b, const I &e) {
  return reduce(std::forward<F>(f), std::forward<Op>(op), R(), b, e);
}

template <typename F, typename Op, typename C,
          typename T = typename C::value_type,
          typename R1 = typename std::result_of<F(T)>::type,
          typename R = typename std::decay<R1>::type>
R reduce(F &&f, Op &&op, const C &c) {
  return reduce(std::forward<F>(f), std::forward<Op>(op), std::begin(c),
                std::end(c));
}

////////////////////////////////////////////////////////////////////////////////
// Point
////////////////////////////////////////////////////////////////////////////////

namespace detail {
template <typename T> struct largeint { typedef T type; };
template <> struct largeint<short> { typedef long long type; };
template <> struct largeint<int> { typedef long long type; };
template <> struct largeint<long> { typedef long long type; };
template <> struct largeint<unsigned short> {
  typedef unsigned long long type;
};
template <> struct largeint<unsigned int> { typedef unsigned long long type; };
template <> struct largeint<unsigned long> { typedef unsigned long long type; };
} // namespace detail

template <typename T, int D> struct point {
  typedef T value_type;
  typedef int size_type;
  constexpr size_type rank() const { return D; }
  array<T, D> elt;
  point() {
    for (int d = 0; d < D; ++d)
      elt[d] = T(0);
  }
  point(const array<T, D> &p) : elt(p) {}
  template <typename U> explicit point(const array<U, D> &p) {
    for (int d = 0; d < D; ++d)
      elt[d] = T(p[d]);
  }
  point(const vector<T> &p) {
    assert(p.size() == D);
    for (int d = 0; d < D; ++d)
      elt[d] = p[d];
  }
  template <typename U> explicit point(const vector<U> &p) {
    assert(p.size() == D);
    for (int d = 0; d < D; ++d)
      elt[d] = T(p[d]);
  }
  point(const point &p) = default;
  point(point &&p) = default;
  explicit point(T x) {
    for (int d = 0; d < D; ++d)
      elt[d] = x;
  }
  template <typename U> explicit point(const point<U, D> &p) {
    for (int d = 0; d < D; ++d)
      elt[d] = T(p.elt[d]);
  }
  operator array<T, D>() const {
    array<T, D> r;
    for (int d = 0; d < D; ++d)
      r[d] = elt[d];
    return r;
  }
  template <typename U> explicit operator array<U, D>() const {
    array<U, D> r;
    for (int d = 0; d < D; ++d)
      r[d] = U(elt[d]);
    return r;
  }
  operator vector<T>() const {
    vector<T> r(D);
    for (int d = 0; d < D; ++d)
      r[d] = elt[d];
    return r;
  }
  template <typename U> explicit operator vector<U>() const {
    vector<U> r(D);
    for (int d = 0; d < D; ++d)
      r[d] = U(elt[d]);
    return r;
  }
  explicit point(T x0, T x1) {
    static_assert(D == 2, "");
    elt[0] = x0;
    elt[1] = x1;
  }
  explicit point(T x0, T x1, T x2) {
    static_assert(D == 3, "");
    elt[0] = x0;
    elt[1] = x1;
    elt[2] = x2;
  }
  explicit point(T x0, T x1, T x2, T x3) {
    static_assert(D == 4, "");
    elt[0] = x0;
    elt[1] = x1;
    elt[2] = x2;
    elt[3] = x3;
  }
  point &operator=(const point &p) = default;
  point &operator=(point &&p) = default;

  // Access and conversion
  const T &operator[](int d) const { return elt[d]; }
  T &operator[](int d) { return elt[d]; }
  point<T, (D > 0 ? D - 1 : 0)> subpoint(int dir) const {
    assert(dir >= 0 && dir < D);
    point<T, (D > 0 ? D - 1 : 0)> r;
    for (int d = 0; d < D - 1; ++d)
      r.elt[d] = elt[d + (d >= dir)];
    return r;
  }
  point<T, D + 1> superpoint(int dir, T x) const {
    point<T, D + 1> r;
    for (int d = 0; d < D; ++d)
      r.elt[d + (d >= dir)] = elt[d];
    r.elt[dir] = x;
    return r;
  }
  point reversed() const {
    point r;
    for (int d = 0; d < D; ++d)
      r.elt[d] = elt[D - 1 - d];
    return r;
  }

  // Unary operators
  point operator+() const {
    point r;
    for (int d = 0; d < D; ++d)
      r.elt[d] = +elt[d];
    return r;
  }
  point operator-() const {
    point r;
    for (int d = 0; d < D; ++d)
      r.elt[d] = -elt[d];
    return r;
  }

  point operator~() const {
    point r;
    for (int d = 0; d < D; ++d)
      r.elt[d] = ~elt[d];
    return r;
  }
  point<bool, D> operator!() const {
    point<bool, D> r;
    for (int d = 0; d < D; ++d)
      r.elt[d] = !elt[d];
    return r;
  }

  // Assignment operators
  point &operator+=(const point &p) {
    for (int d = 0; d < D; ++d)
      elt[d] += p.elt[d];
    return *this;
  }
  point &operator-=(const point &p) {
    for (int d = 0; d < D; ++d)
      elt[d] -= p.elt[d];
    return *this;
  }
  point &operator*=(const point &p) {
    for (int d = 0; d < D; ++d)
      elt[d] *= p.elt[d];
    return *this;
  }
  point &operator/=(const point &p) {
    for (int d = 0; d < D; ++d)
      elt[d] /= p.elt[d];
    return *this;
  }
  point &operator%=(const point &p) {
    for (int d = 0; d < D; ++d)
      elt[d] %= p.elt[d];
    return *this;
  }
  point &operator&=(const point &p) {
    for (int d = 0; d < D; ++d)
      elt[d] &= p.elt[d];
    return *this;
  }
  point &operator|=(const point &p) {
    for (int d = 0; d < D; ++d)
      elt[d] |= p.elt[d];
    return *this;
  }
  point &operator^=(const point &p) {
    for (int d = 0; d < D; ++d)
      elt[d] ^= p.elt[d];
    return *this;
  }

  // Binary operators
  point operator+(const point &p) const { return point(*this) += p; }
  point operator-(const point &p) const { return point(*this) -= p; }
  point operator*(const point &p) const { return point(*this) *= p; }
  point operator/(const point &p) const { return point(*this) /= p; }
  point operator%(const point &p) const { return point(*this) %= p; }
  point operator&(const point &p) const { return point(*this) &= p; }
  point operator|(const point &p) const { return point(*this) |= p; }
  point operator^(const point &p) const { return point(*this) ^= p; }
  point<bool, D> operator&&(const point &p) const {
    return point<bool, D>(*this) &= point<bool, D>(p);
  }
  point<bool, D> operator||(const point &p) const {
    return point<bool, D>(*this) |= point<bool, D>(p);
  }

  // Unary functions
  point abs() const {
    point r;
    for (int d = 0; d < D; ++d)
      r.elt[d] = detail::abs_wrapper(elt[d]);
    return r;
  }

  // Binary functions
  point min(const point &p) const {
    using std::min;
    point r;
    for (int d = 0; d < D; ++d)
      r.elt[d] = min(elt[d], p.elt[d]);
    return r;
  }
  point max(const point &p) const {
    using std::max;
    point r;
    for (int d = 0; d < D; ++d)
      r.elt[d] = max(elt[d], p.elt[d]);
    return r;
  }

  // Comparison operators
  point<bool, D> operator==(const point &p) const {
    point<bool, D> r;
    for (int d = 0; d < D; ++d)
      r.elt[d] = elt[d] == p.elt[d];
    return r;
  }
  point<bool, D> operator!=(const point &p) const { return !(*this == p); }
  point<bool, D> operator<(const point &p) const {
    point<bool, D> r;
    for (int d = 0; d < D; ++d)
      r.elt[d] = elt[d] < p.elt[d];
    return r;
  }
  point<bool, D> operator>(const point &p) const { return p < *this; }
  point<bool, D> operator>=(const point &p) const { return !(*this < p); }
  point<bool, D> operator<=(const point &p) const { return !(*this > p); }

  bool equal_to(const point &p) const {
    std::equal_to<array<T, D>> eq;
    return eq(elt, p.elt);
  }
  bool less(const point &p) const {
    std::less<T> lt;
    // Use Fortran array index ordering, where the highest dimensions are the
    // most significand
    for (int d = D - 1; d >= 0; --d) {
      if (lt(elt[d], p.elt[d]))
        return true;
      if (lt(p.elt[d], elt[d]))
        return false;
    }
    return false;
  }
  size_t hash() const {
    size_t r = size_t(0xb89a122a8c3f540eULL);
    for (int d = 0; d < D; ++d)
      r = hash_combine(r, elt[d]);
    return r;
  }

  // Reductions
  bool all() const {
    bool r = true;
    for (int d = 0; d < D; ++d)
      r = r && elt[d];
    return r;
  }
  bool any() const {
    bool r = false;
    for (int d = 0; d < D; ++d)
      r = r || elt[d];
    return r;
  }
  T minval() const {
    using std::min;
    T r = numeric_limits<T>::max();
    for (int d = 0; d < D; ++d)
      r = min(r, elt[d]);
    return r;
  }
  T maxval() const {
    using std::max;
    T r = numeric_limits<T>::min();
    for (int d = 0; d < D; ++d)
      r = max(r, elt[d]);
    return r;
  }
  T sum() const {
    T r = T(0);
    for (int d = 0; d < D; ++d)
      r += elt[d];
    return r;
  }
  typedef typename detail::largeint<T>::type prod_t;
  prod_t prod() const {
    prod_t r = prod_t(1);
    for (int d = 0; d < D; ++d)
      r *= elt[d];
    return r;
  }

  // I/O

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  explicit point(const YAML::Node &node) {
    assert(node.Tag() ==
           "tag:github.com/eschnett/SimulationIO/asdf-cxx/point-1.0.0");
    *this = point(node.as<vector<T>>());
  }
  friend void operator>>(const YAML::Node &node, point &p) { p = point(node); }
#endif

  ostream &output(ostream &os) const {
    os << "[";
    for (int d = 0; d < D; ++d) {
      if (d > 0)
        os << ",";
      os << elt[d];
    }
    os << "]";
    return os;
  }
  friend ostream &operator<<(ostream &os, const point &p) {
    return p.output(os);
  }

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  YAML::Emitter &output(YAML::Emitter &w) const {
    w << YAML::LocalTag("sio", "point-1.0.0");
    w << YAML::Flow << YAML::BeginSeq;
    for (int d = 0; d < D; ++d)
      w << elt[d];
    w << YAML::EndSeq;
    return w;
  }
  friend YAML::Emitter &operator<<(YAML::Emitter &w, const point &p) {
    return p.output(w);
  }
#endif
};

// Unary functions
template <typename T, int D> point<T, D> abs(const point<T, D> &p) {
  return p.abs();
}

// Binary functions
template <typename T, int D>
point<T, D> min(const point<T, D> &p, const point<T, D> &q) {
  return p.min(q);
}
template <typename T, int D>
point<T, D> max(const point<T, D> &p, const point<T, D> &q) {
  return p.max(q);
}

// Reductions
template <typename T, int D> bool all(const point<T, D> &p) { return p.all(); }
template <typename T, int D> bool any(const point<T, D> &p) { return p.any(); }
template <typename T, int D> T minval(const point<T, D> &p) {
  return p.minval();
}
template <typename T, int D> T maxval(const point<T, D> &p) {
  return p.maxval();
}
template <typename T, int D> T sum(const point<T, D> &p) { return p.sum(); }
template <typename T, int D>
typename point<T, D>::prod_t prod(const point<T, D> &p) {
  return p.prod();
}

// Function objects
template <typename T> struct bit_not;
template <typename T, int D> struct bit_not<point<T, D>> {
  constexpr point<T, D> operator()(const point<T, D> &p) const { return ~p; }
};

template <typename T> struct modulus_eq;
template <typename T, int D> struct modulus_eq<point<T, D>> {
  point<T, D> &operator()(point<T, D> &p, const point<T, D> &q) const {
    return p %= q;
  }
};
template <typename T> struct bit_and_eq;
template <typename T, int D> struct bit_and_eq<point<T, D>> {
  point<T, D> &operator()(point<T, D> &p, const point<T, D> &q) const {
    return p &= q;
  }
};
template <typename T> struct bit_or_eq;
template <typename T, int D> struct bit_or_eq<point<T, D>> {
  point<T, D> &operator()(point<T, D> &p, const point<T, D> &q) const {
    return p |= q;
  }
};
template <typename T> struct bit_xor_eq;
template <typename T, int D> struct bit_xor_eq<point<T, D>> {
  point<T, D> &operator()(point<T, D> &p, const point<T, D> &q) const {
    return p ^= q;
  }
};

template <typename T> struct modulus;
template <typename T, int D> struct modulus<point<T, D>> {
  constexpr point<T, D> operator()(const point<T, D> &p,
                                   const point<T, D> &q) const {
    return p % q;
  }
};
template <typename T> struct bit_and;
template <typename T, int D> struct bit_and<point<T, D>> {
  constexpr point<T, D> operator()(const point<T, D> &p,
                                   const point<T, D> &q) const {
    return p & q;
  }
};
template <typename T> struct bit_or;
template <typename T, int D> struct bit_or<point<T, D>> {
  constexpr point<T, D> operator()(const point<T, D> &p,
                                   const point<T, D> &q) const {
    return p | q;
  }
};
template <typename T> struct bit_xor;
template <typename T, int D> struct bit_xor<point<T, D>> {
  constexpr point<T, D> operator()(const point<T, D> &p,
                                   const point<T, D> &q) const {
    return p ^ q;
  }
};
} // namespace RegionCalculus

namespace std {
template <typename T, int D> struct equal_to<RegionCalculus::point<T, D>> {
  bool operator()(const RegionCalculus::point<T, D> &p,
                  const RegionCalculus::point<T, D> &q) const {
    return p.equal_to(q);
  }
};
template <typename T, int D> struct less<RegionCalculus::point<T, D>> {
  bool operator()(const RegionCalculus::point<T, D> &p,
                  const RegionCalculus::point<T, D> &q) const {
    return p.less(q);
  }
};
template <typename T, int D> struct hash<RegionCalculus::point<T, D>> {
  size_t operator()(const RegionCalculus::point<T, D> &p) const {
    return p.hash();
  }
};
} // namespace std

////////////////////////////////////////////////////////////////////////////////
// Box
////////////////////////////////////////////////////////////////////////////////

namespace RegionCalculus {
template <typename T, int D> struct box;

template <typename T> struct box<T, 0> {
  constexpr static int D = 0;

  bool m_full;

  box() : m_full(false) {}
  box(const box &b) = default;
  box(box &&b) = default;
  explicit box(bool b) : m_full(b) {}
  box(const point<T, D> &lo, const point<T, D> &hi) : m_full(true) {}
  explicit box(const point<T, D> &p) : m_full(true) {}
  box(const vector<T> &lo, const vector<T> &hi) : m_full(true) {}
  box &operator=(const box &p) = default;
  box &operator=(box &&p) = default;
  template <typename U> box(const box<U, D> &b) : m_full(b.m_full) {}

  // Predicates
  bool empty() const { return !m_full; }
  point<T, D> lower() const { return point<T, D>(); }
  point<T, D> upper() const { return point<T, D>(); }
  point<T, D> shape() const { return point<T, D>(); }
  typedef typename point<T, D>::prod_t prod_t;
  prod_t size() const { return m_full; }

  // Shift and scale operators
  box &operator>>=(const point<T, D> &p) { return *this; }
  box &operator<<=(const point<T, D> &p) { return *this; }
  box &operator*=(const point<T, D> &p) { return *this; }
  box operator>>(const point<T, D> &p) const { return *this; }
  box operator<<(const point<T, D> &p) const { return *this; }
  box operator*(const point<T, D> &p) const { return *this; }
  box grow(const point<T, D> &dlo, const point<T, D> &dup) const {
    return *this;
  }
  box grow(const point<T, D> &d) const { return grow(d, d); }
  box grow(T n) const { return grow(point<T, D>(n)); }
  box shrink(const point<T, D> &dlo, const point<T, D> &dup) const {
    return *this;
  }
  box shrink(const point<T, D> &d) const { return shrink(d, d); }
  box shrink(T n) const { return grow(point<T, D>(n)); }

  // Comparison operators
  bool operator==(const box &b) const { return m_full == b.m_full; }
  bool operator!=(const box &b) const { return !(*this == b); }

  bool equal_to(const box &p) const { return m_full == p.m_full; }
  bool less(const box &p) const { return m_full < p.m_full; }
  size_t hash() const {
    std::hash<bool> h;
    return h(m_full) ^ size_t(0x4a473053c081f0efULL);
  }

  // Set comparison operators
  bool contains(const point<T, D> &p) const { return !empty(); }
  bool isdisjoint(const box &b) const { return empty() | b.empty(); }
  bool operator<=(const box &b) const { return m_full <= b.m_full; }
  bool operator>=(const box &b) const { return b <= *this; }
  bool operator<(const box &b) const { return *this <= b && *this != b; }
  bool operator>(const box &b) const { return b < *this; }
  bool is_subset_of(const box &b) const { return *this <= b; }
  bool is_superset_of(const box &b) const { return *this >= b; }
  bool is_strict_subset_of(const box &b) const { return *this < b; }
  bool is_strict_superset_of(const box &b) const { return *this > b; }

  // Set operations
  box bounding_box(const box &b) const { return box(m_full | b.m_full); }

  box operator&(const box &b) const { return box(m_full & b.m_full); }
  box intersection(const box &b) const { return *this & b; }

  vector<box> operator-(const box &b) const {
    if (m_full > b.m_full)
      return vector<box>(1, box(true));
    return vector<box>();
  }
  vector<box> difference(const box &b) const { return *this - b; }

  vector<box> operator|(const box &b) const {
    if (m_full & b.m_full)
      return vector<box>(1, box(true));
    return vector<box>();
  }
  vector<box> setunion(const box &b) const { return *this | b; }

  vector<box> operator^(const box &b) const {
    if (m_full ^ b.m_full)
      return vector<box>(1, box(true));
    return vector<box>();
  }
  vector<box> symmetric_difference(const box &b) const { return *this ^ b; }

  // I/O

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  explicit box(const YAML::Node &node) {
    assert(node.Tag() ==
           "tag:github.com/eschnett/SimulationIO/asdf-cxx/box-1.0.0");
    m_full = node["full"].as<bool>();
  }
  friend void operator>>(const YAML::Node &node, box &b) { b = box(node); }
#endif

  ostream &output(ostream &os) const { return os << "(" << m_full << ")"; }
  friend ostream &operator<<(ostream &os, const box &b) { return b.output(os); }

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  YAML::Emitter &output(YAML::Emitter &w) const {
    w << YAML::LocalTag("sio", "box-1.0.0");
    w << YAML::Flow << YAML::BeginMap;
    w << YAML::Key << "full" << YAML::Value << m_full;
    w << YAML::EndMap;
    return w;
  }
  friend YAML::Emitter &operator<<(YAML::Emitter &w, const box &b) {
    return b.output(w);
  }
#endif
};

template <typename T, int D> struct box {
  point<T, D> lo, hi;
  box() = default;
  box(const box &b) = default;
  box(box &&b) = default;
  box(const point<T, D> &lo, const point<T, D> &hi) : lo(lo), hi(hi) {}
  explicit box(const point<T, D> &p) : lo(p), hi(p + point<T, D>(1)) {}
  box(const array<T, D> &lo, const array<T, D> &hi) : lo(lo), hi(hi) {}
  box(const vector<T> &lo, const vector<T> &hi) : lo(lo), hi(hi) {}
  box &operator=(const box &p) = default;
  box &operator=(box &&p) = default;
  template <typename U> box(const box<U, D> &b) : lo(b.lo), hi(b.hi) {}

  // Predicates
  bool empty() const { return any(hi <= lo); }
  point<T, D> lower() const { return lo; /* empty() ? point<T, D>(0) : lo; */ }
  point<T, D> upper() const { return hi; /* empty() ? point<T, D>(0) : hi; */ }
  point<T, D> shape() const { return max(hi - lo, point<T, D>(0)); }
  typedef typename point<T, D>::prod_t prod_t;
  prod_t size() const { return prod(shape()); }

  // Shift and scale operators
  box &operator>>=(const point<T, D> &p) {
    lo += p;
    hi += p;
    return *this;
  }
  box &operator<<=(const point<T, D> &p) {
    lo -= p;
    hi -= p;
    return *this;
  }
  box &operator*=(const point<T, D> &p) {
    lo *= p;
    hi *= p;
    return *this;
  }
  box operator>>(const point<T, D> &p) const { return box(*this) >>= p; }
  box operator<<(const point<T, D> &p) const { return box(*this) <<= p; }
  box operator*(const point<T, D> &p) const { return box(*this) *= p; }
  box grow(const point<T, D> &dlo, const point<T, D> &dup) const {
    box nb(*this);
    if (!empty()) {
      nb.lo -= dlo;
      nb.hi += dup;
    }
    return nb;
  }
  box grow(const point<T, D> &d) const { return grow(d, d); }
  box grow(T n) const { return grow(point<T, D>(n)); }
  box shrink(const point<T, D> &dlo, const point<T, D> &dup) const {
    return grow(-dlo, -dup);
  }
  box shrink(const point<T, D> &d) const { return shrink(d, d); }
  box shrink(T n) const { return shrink(point<T, D>(n)); }

  // Comparison operators
  bool operator==(const box &b) const {
    if (empty() && b.empty())
      return true;
    if (empty() || b.empty())
      return false;
    std::equal_to<point<T, D>> eq;
    return eq(lo, b.lo) && eq(hi, b.hi);
  }
  bool operator!=(const box &b) const { return !(*this == b); }

  bool equal_to(const box &b) const {
    if (empty() && b.empty())
      return true;
    if (empty() || b.empty())
      return false;
    std::equal_to<point<T, D>> eq;
    return eq(lo, b.lo) && eq(hi, b.hi);
  }
  bool less(const box &b) const {
    // Empty boxes are less than non-empty ones
    if (b.empty())
      return false;
    if (empty())
      return true;
    std::less<point<T, D>> lt;
    if (lt(lo, b.lo))
      return true;
    if (lt(b.lo, lo))
      return false;
    return lt(hi, b.hi);
  }
  size_t hash() const {
    return hash_combine(hash_combine(size_t(0x8ba458a873481993ULL), lo), hi);
  }

  // Set comparison operators
  bool contains(const point<T, D> &p) const {
    if (empty())
      return false;
    return all(p >= lo && p < hi);
  }
  bool isdisjoint(const box &b) const { return (*this & b).empty(); }
  bool operator<=(const box &b) const {
    if (empty())
      return true;
    if (b.empty())
      return false;
    return all(lo >= b.lo && hi <= b.hi);
  }
  bool operator>=(const box &b) const { return b <= *this; }
  bool operator<(const box &b) const { return *this <= b && *this != b; }
  bool operator>(const box &b) const { return b < *this; }
  bool is_subset_of(const box &b) const { return *this <= b; }
  bool is_superset_of(const box &b) const { return *this >= b; }
  bool is_strict_subset_of(const box &b) const { return *this < b; }
  bool is_strict_superset_of(const box &b) const { return *this > b; }

  // Set operations
  box bounding_box(const box &b) const {
    if (empty())
      return b;
    if (b.empty())
      return *this;
    auto r = box(min(lo, b.lo), max(hi, b.hi));
// Postcondition
#if REGIONCALCULUS_DEBUG
    assert(*this <= r && b <= r);
#endif
    return r;
  }

  box operator&(const box &b) const {
    auto nlo = max(lo, b.lo);
    auto nhi = min(hi, b.hi);
    auto r = box(nlo, nhi);
// Postcondition
#if REGIONCALCULUS_DEBUG
    assert(r <= *this && r <= b);
#endif
    return r;
  }
  box intersection(const box &b) const { return *this & b; }

private:
  void split(const point<T, D> &p, vector<box> &rs) const {
    assert(!empty());
#if REGIONCALCULUS_DEBUG
    const auto old_rs_size = rs.size();
#endif
    for (int m = 0; m < (1 << D); ++m) {
      point<T, D> newlo = lo, newhi = hi;
      bool is_inside = true;
      for (int d = 0; d < D; ++d) {
        const bool lohi = m & (1 << d);
        if (p.elt[d] > lo.elt[d] && p.elt[d] < hi.elt[d]) {
          if (!lohi)
            newhi.elt[d] = p.elt[d];
          else
            newlo.elt[d] = p.elt[d];
        } else {
          is_inside &= !lohi;
        }
      }
      if (is_inside)
        rs.push_back(box(newlo, newhi));
    }
#if REGIONCALCULUS_DEBUG
    // Postcondition
    prod_t vol = prod_t(0);
    for (auto i = old_rs_size; i < rs.size(); ++i) {
      assert(!rs[i].empty());
      assert(rs[i] <= *this);
      vol += rs[i].size();
    }
    assert(vol == size());
    for (size_t i = 0; i < rs.size(); ++i)
      for (size_t j = i + 1; j < rs.size(); ++j)
        assert(rs[i].isdisjoint(rs[j]));
#endif
  }

public:
  vector<box> operator-(const box &b) const {
    if (empty())
      return vector<box>();
    if (b.empty())
      return vector<box>(1, *this);
    vector<box> bs1;
    split(b.lo, bs1);
    vector<box> bs2;
    for (const auto &b1 : bs1)
      b1.split(b.hi, bs2);
    vector<box> rs;
    for (const auto &b2 : bs2) {
      assert(b2.isdisjoint(b) || b2 <= b);
      if (b2.isdisjoint(b))
        rs.push_back(b2);
    }
#if REGIONCALCULUS_DEBUG
    // Postcondition
    prod_t vol = prod_t(0);
    for (const auto &r : rs) {
      assert(!r.empty());
      assert(r <= *this && !(r <= b));
      vol += r.size();
    }
    assert(vol >= max(prod_t(0), size() - b.size()) && vol <= size());
    for (size_t i = 0; i < rs.size(); ++i)
      for (size_t j = i + 1; j < rs.size(); ++j)
        assert(rs[i].isdisjoint(rs[j]));
#endif
    return rs;
  }
  vector<box> difference(const box &b) const { return *this - b; }

  vector<box> operator|(const box &b) const {
    auto rs = *this - b;
    if (!b.empty())
      rs.push_back(b);
#if REGIONCALCULUS_DEBUG
    // Postcondition
    prod_t vol = prod_t(0);
    for (const auto &r : rs) {
      assert(!r.empty());
      assert(r <= *this || r <= b);
      vol += r.size();
    }
    assert(vol >= size() && vol <= size() + b.size());
    for (size_t i = 0; i < rs.size(); ++i)
      for (size_t j = i + 1; j < rs.size(); ++j)
        assert(rs[i].isdisjoint(rs[j]));
#endif
    return rs;
  }
  vector<box> setunion(const box &b) const { return *this | b; }

  vector<box> operator^(const box &b) const {
    auto rs = *this - b;
    auto rs1 = b - *this;
    // TODO: Avoid this concatenation
    rs.insert(rs.end(), rs1.begin(), rs1.end());
#if REGIONCALCULUS_DEBUG
    // Postcondition
    prod_t vol = prod_t(0);
    for (const auto &r : rs) {
      assert(!r.empty());
      assert((r <= *this) ^ (r <= b));
      vol += r.size();
    }
    assert(vol >= abs(size() - b.size()) && vol <= size() + b.size());
    for (size_t i = 0; i < rs.size(); ++i)
      for (size_t j = i + 1; j < rs.size(); ++j)
        assert(rs[i].isdisjoint(rs[j]));
#endif
    return rs;
  }
  vector<box> symmetric_difference(const box &b) const { return *this ^ b; }

  // I/O

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  explicit box(const YAML::Node &node) {
    assert(node.Tag() ==
           "tag:github.com/eschnett/SimulationIO/asdf-cxx/box-1.0.0");
    lo = point<T, D>(node["low"]);
    hi = point<T, D>(node["high"]);
  }
  friend void operator>>(const YAML::Node &node, box &b) { b = box(node); }
#endif

  ostream &output(ostream &os) const {
    return os << "(" << lo << ":" << hi << ")";
  }
  friend ostream &operator<<(ostream &os, const box &b) { return b.output(os); }

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  YAML::Emitter &output(YAML::Emitter &w) const {
    w << YAML::LocalTag("sio", "box-1.0.0");
    w << YAML::Flow << YAML::BeginMap;
    w << YAML::Key << "low" << YAML::Value << lo;
    w << YAML::Key << "high" << YAML::Value << hi;
    w << YAML::EndMap;
    return w;
  }
  friend YAML::Emitter &operator<<(YAML::Emitter &w, const box &b) {
    return b.output(w);
  }
#endif
};
} // namespace RegionCalculus

namespace std {
template <typename T, int D> struct equal_to<RegionCalculus::box<T, D>> {
  bool operator()(const RegionCalculus::box<T, D> &p,
                  const RegionCalculus::box<T, D> &q) const {
    return p.equal_to(q);
  }
};
template <typename T, int D> struct less<RegionCalculus::box<T, D>> {
  bool operator()(const RegionCalculus::box<T, D> &p,
                  const RegionCalculus::box<T, D> &q) const {
    return p.less(q);
  }
};
template <typename T, int D> struct hash<RegionCalculus::box<T, D>> {
  size_t operator()(const RegionCalculus::box<T, D> &b) const {
    return b.hash();
  }
};
} // namespace std

////////////////////////////////////////////////////////////////////////////////
// Region
////////////////////////////////////////////////////////////////////////////////

#if !REGIONCALCULUS_TREE

namespace RegionCalculus {
template <typename T, int D> struct region {
  vector<box<T, D>> boxes;
  region() = default;
  region(const region &r) = default;
  region(region &&r) = default;

  region(const box<T, D> &b) {
    if (!b.empty())
      boxes.push_back(b);
#if REGIONCALCULUS_DEBUG
    assert(invariant());
#endif
  }
  region(const vector<box<T, D>> &bs) : boxes(bs) {
    normalize();
    assert(invariant());
  }
  region(vector<box<T, D>> &&bs) : boxes(move(bs)) {
    normalize();
    assert(invariant());
  }
  operator vector<box<T, D>>() const { return boxes; }
  region &operator=(const region &r) = default;
  region &operator=(region &&r) = default;
  template <typename U> region(const region<U, D> &r) {
    boxes.reserve(r.boxes.size());
    for (const auto &b : r.boxes)
      boxes.push_back(box<T, D>(b));
    assert(invariant());
  }

private:
  void append(const region &r) {
    boxes.insert(boxes.end(), r.boxes.begin(), r.boxes.end());
  }

  // Normalization
  void normalize() { sort(boxes.begin(), boxes.end(), std::less<box<T, D>>()); }

public:
  // Invariant
  bool invariant() const {
    for (size_t i = 0; i < boxes.size(); ++i) {
      if (boxes[i].empty())
        return false;
      for (size_t j = i + 1; j < boxes.size(); ++j) {
        if (!boxes[i].isdisjoint(boxes[j]))
          return false;
        if (!boxes[i].less(boxes[j]))
          return false;
      }
    }
    return true;
  }

  // Predicates
  bool empty() const { return boxes.empty(); }
  typedef typename point<T, D>::prod_t prod_t;
  prod_t size() const {
    prod_t sz = T(0);
    for (const auto &b : boxes)
      sz += b.size();
    return sz;
  }

  // Shift and scale operators
  region operator>>(const point<T, D> &d) const {
    region nr(*this);
    for (auto &b : nr.boxes)
      b >>= d;
    return nr;
  }
  region operator<<(const point<T, D> &d) const { return *this >> -d; }
  region grow(const point<T, D> &dlo, const point<T, D> &dup) const {
    // Cannot shrink
    assert(all(dlo + dup >= point<T, D>(T(0))));
    region nr;
    for (const auto &b : boxes)
      nr = nr | b.grow(dlo, dup);
    return nr;
  }
  region grow(const point<T, D> &d) const { return grow(d, d); }
  region grow(T n) const { return grow(point<T, D>(n)); }
  region shrink(const point<T, D> &dlo, const point<T, D> &dup) const {
    // Cannot grow
    assert(all(dlo + dup >= point<T, D>(T(0))));
    auto maxdist = maxval(max(abs(dlo), abs(dup)));
    auto world = bounding_box();
    region world2 = world.grow(2 * maxdist);
    return world2 - (world2 - *this).grow(dlo, dup);
  }
  region shrink(const point<T, D> &d) const { return shrink(d, d); }
  region shrink(T n) const { return shrink(point<T, D>(n)); }

  // Set operations
  box<T, D> bounding_box() const {
    box<T, D> r;
    for (const auto &b : boxes)
      r = r.bounding_box(b);
    return r;
  }

  region operator&(const box<T, D> &b) const {
    region nr;
    for (const auto &rb : boxes) {
      auto nb = rb & b;
      if (!nb.empty())
        nr.boxes.push_back(nb);
    }
    nr.normalize();
#if REGIONCALCULUS_DEBUG
    assert(invariant());
#endif
    return nr;
  }
  region operator&(const region &r) const {
    region nr;
    for (const auto &b : r.boxes)
      nr.append(*this & b);
    nr.normalize();
#if REGIONCALCULUS_DEBUG
    assert(invariant());
#endif
    return nr;
  }
  region &operator&=(const box<T, D> &b) { return *this = *this & b; }
  region &operator&=(const region &r) { return *this = *this & r; }
  region intersection(const box<T, D> &b) const { return *this & b; }
  region intersection(const region &r) const { return *this & r; }

  region operator-(const box<T, D> &b) const {
    region nr;
    for (const auto &rb : boxes)
      nr.append(region(rb - b));
    nr.normalize();
#if REGIONCALCULUS_DEBUG
    assert(invariant());
#endif
    return nr;
  }
  region operator-(const region &r) const {
    region nr = *this;
    for (const auto &b : r.boxes)
      nr = nr - b;
    nr.normalize();
#if REGIONCALCULUS_DEBUG
    assert(invariant());
#endif
    return nr;
  }
  region &operator-=(const box<T, D> &b) { return *this = *this - b; }
  region &operator-=(const region &r) { return *this = *this - r; }
  region difference(const box<T, D> &b) const { return *this - b; }
  region difference(const region &r) const { return *this - r; }

  region operator|(const region &r) const {
    region nr = *this - r;
    nr.append(r);
    nr.normalize();
#if REGIONCALCULUS_DEBUG
    assert(invariant());
#endif
    return nr;
  }
  region operator|(const box<T, D> &b) const { return *this | region(b); }
  region &operator|=(const box<T, D> &b) { return *this = *this | b; }
  region &operator|=(const region &r) { return *this = *this | r; }
  region setunion(const region &r) const { return *this | r; }
  region setunion(const box<T, D> &b) const { return *this | b; }

  region operator^(const region &r) const {
    region nr = *this - r;
    nr.append(r - *this);
    nr.normalize();
#if REGIONCALCULUS_DEBUG
    assert(invariant());
#endif
    return nr;
  }
  region operator^(const box<T, D> &b) const { return *this ^ region(b); }
  region &operator^=(const box<T, D> &b) { return *this = *this ^ b; }
  region &operator^=(const region &r) { return *this = *this ^ r; }
  region symmetric_difference(const region &r) const { return *this ^ r; }
  region symmetric_difference(const box<T, D> &b) const { return *this ^ b; }

  // Set comparison operators
  bool contains(const point<T, D> &p) const {
    for (const auto &b : boxes)
      if (b.contains(p))
        return true;
    return false;
  }
  bool isdisjoint(const box<T, D> &b) const {
    for (const auto &rb : boxes)
      if (!rb.isdisjoint(b))
        return false;
    return true;
  }
  bool isdisjoint(const region &r) const {
    for (const auto &b : r.boxes)
      if (!isdisjoint(b))
        return false;
    return true;
  }

  // Comparison operators
  bool operator<=(const region &r) const { return (*this - r).empty(); }
  bool operator>=(const region &r) const { return r <= *this; }
  bool operator<(const region &r) const {
    return *this <= r && size() < r.size();
  }
  bool operator>(const region &r) const { return r < *this; }
  bool is_subset_of(const region &r) const { return *this <= r; }
  bool is_superset_of(const region &r) const { return *this >= r; }
  bool is_strict_subset_of(const region &r) const { return *this < r; }
  bool is_strict_superset_of(const region &r) const { return *this > r; }
  bool operator==(const region &r) const { return (*this ^ r).empty(); }
  bool operator!=(const region &r) const { return !(*this == r); }

  bool equal_to(const region &r) const {
    std::equal_to<vector<T>> eq;
    return eq(boxes, r.boxes);
  }
  bool less(const region &r) const {
    std::less<vector<T>> lt;
    return lt(boxes, r.boxes);
  }
  size_t hash() const {
    std::hash<vector<T>> h;
    return h(boxes) ^ size_t(0x4861d2118c306aefULL);
  }

  // I/O

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  explicit region(const YAML::Node &node) {
    assert(node.Tag() ==
           "tag:github.com/eschnett/SimulationIO/asdf-cxx/region-1.0.0");
    dim = node["dimension"].as<int>();
    assert(dim == D);
    boxes = node["boxes"].as<vector<box<T, D>>>();
  }
#endif

  ostream &output(ostream &os) const {
    os << "{";
    for (size_t i = 0; i < boxes.size(); ++i) {
      if (i > 0)
        os << ",";
      os << boxes[i];
    }
    os << "}";
    return os;
  }
  friend ostream &operator<<(ostream &os, const region &r) {
    return r.output(os);
  }

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  YAML::Emitter &output(YAML::Emitter &w) const {
    w << YAML::LocalTag("sio", "region-1.0.0");
    w << YAML::Flow << YAML::BeginMap;
    w << YAML::Key << "dimension" << YAML::Value << D;
    w << YAML::Key << "boxes" << YAML::Value << boxes;
    w << YAMLL::EndMap;
    return w;
  }
  friend YAML::Emitter &operator<<(YAML::Emitter &w, const region &r) {
    return r.output(w);
  }
#endif
};
} // namespace RegionCalculus

namespace std {
template <typename T, int D> struct equal_to<RegionCalculus::region<T, D>> {
  bool operator()(const RegionCalculus::region<T, D> &p,
                  const RegionCalculus::region<T, D> &q) const {
    return p.equal_to(q);
  }
};
template <typename T, int D> struct less<RegionCalculus::region<T, D>> {
  bool operator()(const RegionCalculus::region<T, D> &p,
                  const RegionCalculus::region<T, D> &q) const {
    return p.less(q);
  }
};
template <typename T, int D> struct hash<RegionCalculus::region<T, D>> {
  size_t operator()(const RegionCalculus::region<T, D> &p) const {
    return p.hash();
  }
};
} // namespace std

#else // #if REGIONCALCULUS_TREE

namespace RegionCalculus {
template <typename T, int D> struct region;

template <typename T> struct region<T, 0> {
  constexpr static int D = 0;

  bool m_full;

  region() : m_full(false) {}
  region(const region &) = default;
  region(region &&) = default;
  region &operator=(const region &) = default;
  region &operator=(region &&) = default;

  explicit region(bool b) : m_full(b) {}
  region(const box<T, D> &b) : m_full(b.m_full) {}
  region(const point<T, D> &p) : m_full(true) {}
  region(const vector<box<T, D>> &bs) {
    m_full = false;
    for (const auto &b : bs)
      m_full |= !b.empty();
  }
  template <typename U> region(const region<U, D> &r) : m_full(r.m_full) {}

  // Invariant
  bool invariant() const { return true; }

  // Predicates
  bool empty() const { return !m_full; }
  typedef typename point<T, D>::prod_t prod_t;
  prod_t size() const { return m_full; }
  ptrdiff_t chi_size() const { return 1; }

  // Conversion to boxes
  operator vector<box<T, D>>() const {
    if (empty())
      return vector<box<T, D>>();
    return vector<box<T, D>>(1, box<T, D>(true));
  }

  // Shift and scale operators
  region operator>>(const point<T, D> &d) const { return *this; }
  region operator<<(const point<T, D> &d) const { return *this; }
  region grow(const point<T, D> &dlo, const point<T, D> &dup) const {
    return *this;
  }
  region grow(const point<T, D> &d) const { return grow(d, d); }
  region grow(T n) const { return grow(point<T, D>(n)); }
  region shrink(const point<T, D> &dlo, const point<T, D> &dup) const {
    return *this;
  }
  region shrink(const point<T, D> &d) const { return shrink(d, d); }
  region shrink(T n) const { return shrink(point<T, D>(n)); }

  // Set operations
  box<T, D> bounding_box() const { return box<T, D>(m_full); }

  region operator&(const region &other) const {
    return region(m_full & other.m_full);
  }
  region operator|(const region &other) const {
    return region(m_full | other.m_full);
  }
  region operator^(const region &other) const {
    return region(m_full ^ other.m_full);
  }
  region operator-(const region &other) const {
    return region(m_full & !other.m_full);
  }

  region &operator^=(const region &other) { return *this = *this ^ other; }
  region &operator&=(const region &other) { return *this = *this & other; }
  region &operator|=(const region &other) { return *this = *this | other; }
  region &operator-=(const region &other) { return *this = *this - other; }

  region intersection(const region &other) const { return *this & other; }
  region setunion(const region &other) const { return *this | other; }
  region symmetric_difference(const region &other) const {
    return *this ^ other;
  }
  region difference(const region &other) const { return *this - other; }

  // Set comparison operators
  bool contains(const point<T, D> &p) const { return m_full; }
  bool isdisjoint(const region &other) const {
    return !(m_full & other.m_full);
  }

  // Comparison operators
  bool operator<=(const region &other) const { return !m_full || other.m_full; }
  bool operator>=(const region &other) const { return other <= *this; }
  bool operator<(const region &other) const { return !m_full & other.m_full; }
  bool operator>(const region &other) const { return other < *this; }
  bool is_subset_of(const region &other) const { return *this <= other; }
  bool is_superset_of(const region &other) const { return *this >= other; }
  bool is_strict_subset_of(const region &other) const { return *this < other; }
  bool is_strict_superset_of(const region &other) const { return *this > other; }
  bool operator==(const region &other) const { return m_full == other.m_full; }
  bool operator!=(const region &other) const { return !(*this == other); }

  bool equal_to(const region &other) const { return m_full == other.m_full; }
  bool less(const region &other) const { return m_full < other.m_full; }
  size_t hash() const {
    std::hash<bool> h;
    return h(m_full) ^ size_t(0x07da947bfacbea06ULL);
  }

  // I/O

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  explicit region(const YAML::Node &node) {
    assert(node.Tag() ==
           "tag:github.com/eschnett/SimulationIO/asdf-cxx/region-1.0.0");
    auto dim = node["dimension"].as<int>();
    assert(dim == 0);
    m_full = node["full"].as<bool>();
  }
#endif

  ostream &output(ostream &os) const {
    os << "{";
    if (m_full)
      os << "(1)";
    os << "}";
    return os;
  }
  friend ostream &operator<<(ostream &os, const region &r) {
    return r.output(os);
  }

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  YAML::Emitter &output(YAML::Emitter &w) const {
    w << YAML::LocalTag("sio", "region-1.0.0");
    w << YAML::Flow << YAML::BeginMap;
    w << YAML::Key << "dimension" << YAML::Value << 0;
    w << YAML::Key << "full" << YAML::Value << m_full;
    w << YAML::EndMap;
    return w;
  }
  friend YAML::Emitter &operator<<(YAML::Emitter &w, const region &r) {
    return r.output(w);
  }
#endif
};

template <typename T> struct region<T, 1> {
  constexpr static int D = 1;

  typedef region<T, D - 1> subregion_t; // This is essentially a bool
  typedef vector<T> subregions_t;
  subregions_t subregions;

  region() = default;
  region(const region &) = default;
  region(region &&) = default;
  region &operator=(const region &) = default;
  region &operator=(region &&) = default;

  region(const box<T, D> &b) {
    if (b.empty())
      return;
    subregions = {b.lower()[0], b.upper()[0]};
    assert(invariant());
  }
  region(const point<T, D> &p) : region({p[0], p[0] + 1}) {}
  template <typename U> region(const region<U, D> &r) {
    subregions.reserve(r.subregions.size());
    for (const auto &pos : r.subregions)
      subregions.push_back(T(pos));
  }

private:
  template <typename F> void traverse_subregions(const F &f) const {
    subregion_t decoded_subregion;
    for (const auto &pos : subregions) {
      decoded_subregion ^= subregion_t(true);
      f(pos, decoded_subregion);
    }
    assert(decoded_subregion.empty());
  }

  template <typename F>
  void traverse_subregions(const F &f, const region &other) const {
    subregion_t decoded_subregion0, decoded_subregion;

    typedef typename subregions_t::const_iterator subregions_iter_t;
    subregions_iter_t iter0 = subregions.begin();
    subregions_iter_t iter1 = other.subregions.begin();
    const subregions_iter_t end0 = subregions.end();
    const subregions_iter_t end1 = other.subregions.end();
    while (iter0 != end0 && iter1 != end1) {
      const T next_pos0 = *iter0;
      const T next_pos1 = *iter1;
      const T pos = min(next_pos0, next_pos1);
      const bool active0 = next_pos0 == pos;
      const bool active1 = next_pos1 == pos;
      decoded_subregion0 ^= subregion_t(active0);
      decoded_subregion ^= subregion_t(active1);
      f(pos, decoded_subregion0, decoded_subregion);
      if (active0)
        ++iter0;
      if (active1)
        ++iter1;
    }
    for (; iter0 != end0; ++iter0) {
      const T pos = *iter0;
      decoded_subregion0 ^= subregion_t(true);
      f(pos, decoded_subregion0, subregion_t(false));
    }
    for (; iter1 != end1; ++iter1) {
      const T pos = *iter1;
      decoded_subregion ^= subregion_t(true);
      f(pos, subregion_t(false), decoded_subregion);
    }
    assert(decoded_subregion0.empty());
    assert(decoded_subregion.empty());
  }

  template <typename F> region unary_operator(const F &op) const {
    region res;
    // res.reserve(subregions.size());
    subregion_t old_decoded_subregion;
    traverse_subregions(
        [&](const T pos, const subregion_t &decoded_subregion0) {
          auto decoded_subregion = op(decoded_subregion0);
          auto subregion = decoded_subregion ^ old_decoded_subregion;
          if (!subregion.empty())
            res.subregions.push_back(pos);
          old_decoded_subregion = decoded_subregion;
        });
    assert(old_decoded_subregion.empty());
    assert(res.invariant());
    return res;
  }

  template <typename F>
  region binary_operator(const F &op, const region &other) const {
    region res;
    // res.reserve(subregions.size() + other.subregions.size());
    subregion_t old_decoded_subregion;
    traverse_subregions(
        [&](const T pos, const subregion_t &decoded_subregion0,
            const subregion_t &decoded_subregion1) {
          auto decoded_subregion = op(decoded_subregion0, decoded_subregion1);
          auto subregion = decoded_subregion ^ old_decoded_subregion;
          if (!subregion.empty())
            res.subregions.push_back(pos);
          old_decoded_subregion = decoded_subregion;
        },
        other);
    assert(old_decoded_subregion.empty());
    assert(res.invariant());
    return res;
  }

public:
  // Invariant
  bool invariant() const {
#if REGIONCALCULUS_DEBUG
    const size_t nboxes = subregions.size();
    for (size_t i = 1; i < nboxes; ++i)
      if (subregions[i] <= subregions[i - 1])
        return false;
    if (chi_size() % 2 != 0)
      return false;
#endif
    return true;
  }

  // Predicates
  bool empty() const { return subregions.empty(); }

  typedef typename point<T, D>::prod_t prod_t;
  prod_t size() const {
    prod_t total_size = 0;
    typedef typename subregions_t::const_iterator subregions_iter_t;
    subregions_iter_t iter = subregions.begin();
    const subregions_iter_t end = subregions.end();
    while (iter != end) {
      const auto pos0 = *iter++;
      const auto pos1 = *iter++;
      total_size += pos1 - pos0;
    }
    return total_size;
  }

  ptrdiff_t chi_size() const { return subregions.size(); }

private:
  static vector<T> subregions_from_bounds(vector<T> lbnds, vector<T> ubnds) {
    const size_t nboxes = lbnds.size();
    assert(ubnds.size() == nboxes);
    vector<T> subregions;
    if (nboxes == 0)
      return subregions;
    sort(lbnds.begin(), lbnds.end());
    sort(ubnds.begin(), ubnds.end());
    size_t nactive = 0;
    size_t lpos = 0, upos = 0;
    while (lpos < nboxes) {
      const auto lbnd = lbnds[lpos];
      assert(upos < nboxes);
      const auto ubnd = ubnds[upos];
      // Process lower bounds before upper bounds
      if (lbnd <= ubnd) {
        if (nactive == 0)
          subregions.push_back(lbnd);
        ++nactive;
        ++lpos;
      } else {
        assert(nactive > 0);
        --nactive;
        if (nactive == 0)
          subregions.push_back(ubnd);
        ++upos;
      }
    }
    assert(nactive > 0);
    assert(upos < nboxes);
    assert(upos + nactive == nboxes);
    subregions.push_back(ubnds[nboxes - 1]);
#if REGIONCALCULUS_DEBUG
    {
      region reg;
      for (size_t i = 0; i < nboxes; ++i)
        reg |= region(box<T, D>(point<T, 1>(lbnds[i]), point<T, 1>(ubnds[i])));
      assert(subregions == reg.subregions);
    }
#endif
    return subregions;
  }

public:
  // Conversion from and to boxes
  region(const vector<box<T, D>> &boxes) {
    vector<T> lbnds, ubnds;
    lbnds.reserve(boxes.size());
    ubnds.reserve(boxes.size());
    for (const auto &box : boxes) {
      lbnds.push_back(box.lower()[0]);
      ubnds.push_back(box.upper()[0]);
    }
    subregions = subregions_from_bounds(move(lbnds), move(ubnds));
    assert(invariant());
  }

  operator vector<box<T, D>>() const {
    vector<box<T, D>> res;
    res.reserve(subregions.size() / 2);
    typedef typename subregions_t::const_iterator subregions_iter_t;
    subregions_iter_t iter = subregions.begin();
    const subregions_iter_t end = subregions.end();
    while (iter != end) {
      const auto pos0 = *iter++;
      const auto pos1 = *iter++;
      res.emplace_back(box<T, D>(point<T, 1>(pos0), point<T, 1>(pos1)));
    }
#if REGIONCALCULUS_DEBUG
    assert(is_sorted(res.begin(), res.end()));
    {
      region reg;
      for (const auto &b : res) {
        assert(region(b).isdisjoint(reg));
        reg |= b;
      }
      assert(reg == *this);
    }
#endif
    return res;
  }

  // Shift and scale operators
  region operator>>(const point<T, D> &d) const {
    region nr;
    nr.subregions.reserve(subregions.size());
    for (const auto &pos : subregions)
      nr.subregions.push_back(pos + d[0]);
    return nr;
  }
  region operator<<(const point<T, D> &d) const { return *this >> -d; }
  region grow(const point<T, D> &dlo, const point<T, D> &dup) const {
    // Cannot shrink
    assert(all(dlo + dup >= point<T, D>(T(0))));
    vector<T> lbnds, ubnds;
    lbnds.reserve(subregions.size());
    ubnds.reserve(subregions.size());
    typedef typename subregions_t::const_iterator subregions_iter_t;
    subregions_iter_t iter = subregions.begin();
    const subregions_iter_t end = subregions.end();
    while (iter != end) {
      const auto pos0 = *iter++ - dlo[0];
      const auto pos1 = *iter++ + dup[0];
      lbnds.push_back(pos0);
      ubnds.push_back(pos1);
    }
    region nr;
    nr.subregions = subregions_from_bounds(move(lbnds), move(ubnds));
    assert(nr.invariant());
#if REGIONCALCULUS_DEBUG
    {
      const vector<box<T, D>> boxes(*this);
      region reg;
      for (const auto &box : boxes)
        reg |= box.grow(dlo, dup);
      assert(nr == reg);
    }
#endif
    return nr;
  }
  region grow(const point<T, D> &d) const { return grow(d, d); }
  region grow(T n) const { return grow(point<T, D>(n)); }
  region shrink(const point<T, D> &dlo, const point<T, D> &dup) const {
    // Cannot grow
    assert(all(dlo + dup >= point<T, D>(T(0))));
    region nr;
    typedef typename subregions_t::const_iterator subregions_iter_t;
    subregions_iter_t iter = subregions.begin();
    const subregions_iter_t end = subregions.end();
    while (iter != end) {
      const auto pos0 = *iter++ + dlo[0];
      const auto pos1 = *iter++ - dup[0];
      if (pos1 > pos0) {
        nr.subregions.push_back(pos0);
        nr.subregions.push_back(pos1);
      }
    }
    assert(nr.invariant());
#if REGIONCALCULUS_DEBUG
    {
      auto world = bounding_box().grow(1);
      region reg =
          region(world.grow(dup, dlo)) - (region(world) - *this).grow(dup, dlo);
      assert(nr == reg);
    }
#endif
    return nr;
  }
  region shrink(const point<T, D> &d) const { return shrink(d, d); }
  region shrink(T n) const { return shrink(point<T, D>(n)); }

  // Set operations
  box<T, D> bounding_box() const {
    if (empty())
      return box<T, D>();
    return box<T, D>(point<T, 1>(*subregions.begin()),
                     point<T, 1>(*subregions.rbegin()));
  }

  region operator^(const region &other) const {
    return binary_operator([](const subregion_t &set0,
                              const subregion_t &set1) { return set0 ^ set1; },
                           other);
  }

  region operator&(const region &other) const {
    return binary_operator([](const subregion_t &set0,
                              const subregion_t &set1) { return set0 & set1; },
                           other);
  }

  region operator|(const region &other) const {
    return binary_operator([](const subregion_t &set0,
                              const subregion_t &set1) { return set0 | set1; },
                           other);
  }

  region operator-(const region &other) const {
    return binary_operator([](const subregion_t &set0,
                              const subregion_t &set1) { return set0 - set1; },
                           other);
  }

  region &operator^=(const region &other) { return *this = *this ^ other; }
  region &operator&=(const region &other) { return *this = *this & other; }
  region &operator|=(const region &other) { return *this = *this | other; }
  region &operator-=(const region &other) { return *this = *this - other; }

  region intersection(const region &other) const { return *this & other; }
  region setunion(const region &other) const { return *this | other; }
  region symmetric_difference(const region &other) const {
    return *this ^ other;
  }
  region difference(const region &other) const { return *this - other; }

  // Set comparison operators
  bool contains(const point<T, D> &p) const {
    if (subregions.empty())
      return false;
    if (p[0] < *subregions.begin() || p[0] >= *subregions.rbegin())
      return false;
    const auto pos = std::find_if(subregions.begin(), subregions.end(),
                                  [p](T pos) { return p[0] >= pos; });
    return (pos - subregions.begin()) % 2 == 0;
  }
  bool isdisjoint(const region &other) const { return (*this & other).empty(); }

  // Comparison operators
  bool operator<=(const region &other) const { return (*this - other).empty(); }
  bool operator>=(const region &other) const { return other <= *this; }
  bool operator<(const region &other) const {
    return *this != other && *this <= other;
  }
  bool operator>(const region &other) const { return other < *this; }
  bool is_subset_of(const region &other) const { return *this <= other; }
  bool is_superset_of(const region &other) const { return *this >= other; }
  bool is_strict_subset_of(const region &other) const { return *this < other; }
  bool is_strict_superset_of(const region &other) const { return *this > other; }
  bool operator==(const region &other) const {
    return subregions == other.subregions;
  }
  bool operator!=(const region &other) const { return !(*this == other); }

  bool equal_to(const region &other) const {
    std::equal_to<subregions_t> eq;
    return eq(subregions, other.subregions);
  }
  bool less(const region &other) const {
    std::less<subregions_t> lt;
    return lt(subregions, other.subregions);
  }
  size_t hash() const {
    size_t r = size_t(0x725f347c326789eeULL);
    for (const auto &pos : subregions)
      r = hash_combine(r, pos);
    return r;
  }

  // I/O

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  explicit region(const YAML::Node &node) {
    assert(node.Tag() ==
           "tag:github.com/eschnett/SimulationIO/asdf-cxx/region-1.0.0");
    auto dim = node["dimension"].as<int>();
    assert(dim == D);
    // const auto &boxes = node["boxes"].as<vector<box<T, D>>>();
    vector<box<T, D>> boxes;
    for (const auto &b : node["boxes"])
      boxes.push_back(box<T, D>(b));
    *this = region(boxes);
  }
  friend void operator>>(const YAML::Node &node, region &r) {
    r = region(node);
  }
#endif

  ostream &output(ostream &os) const {
    // os << "{";
    // for (const auto &pos_subregion : subregions)
    //   os << pos_subregion.first << ":" << pos_subregion.second << ",";
    // os << "}";
    os << "{";
    const vector<box<T, D>> boxes(*this);
    for (size_t i = 0; i < boxes.size(); ++i) {
      if (i > 0)
        os << ",";
      os << boxes[i];
    }
    os << "}";
    return os;
  }
  friend ostream &operator<<(ostream &os, const region &r) {
    return r.output(os);
  }

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  YAML::Emitter &output(YAML::Emitter &w) const {
    w << YAML::LocalTag("sio", "region-1.0.0");
    w << YAML::Flow << YAML::BeginMap;
    w << YAML::Key << "dimension" << YAML::Value << D;
    w << YAML::Key << "boxes" << YAML::Value << vector<box<T, D>>(*this);
    w << YAML::EndMap;
    return w;
  }
  friend YAML::Emitter &operator<<(YAML::Emitter &w, const region &r) {
    return r.output(w);
  }
#endif
};

template <typename T, int D> struct region {
  typedef region<T, D - 1> subregion_t;
  typedef vector<pair<T, subregion_t>> subregions_t;
  subregions_t subregions;

  region() = default;
  region(const region &) = default;
  region(region &&) = default;
  region &operator=(const region &) = default;
  region &operator=(region &&) = default;

  region(const box<T, D> &b) {
    if (b.empty())
      return;
    box<T, D - 1> subbox(b.lower().subpoint(D - 1), b.upper().subpoint(D - 1));
    subregions = {{b.lower()[D - 1], subregion_t(subbox)},
                  {b.upper()[D - 1], subregion_t(subbox)}};
    assert(invariant());
  }
  region(const point<T, D> &p) : region(box<T, D>(p)) {}
  template <typename U> region(const region<U, D> &r) {
    subregions.reserve(r.subregions.size());
    for (const auto &pos_subregion : r.subregions) {
      T pos(pos_subregion.first);
      subregion_t subregion(pos_subregion.second);
      subregions.emplace_back(make_pair(pos, move(subregion)));
    }
  }

private:
  template <typename F> void traverse_subregions(const F &f) const {
    subregion_t decoded_subregion;
    for (const auto &pos_subregion : subregions) {
      const T pos = pos_subregion.first;
      const auto &subregion = pos_subregion.second;
      decoded_subregion ^= subregion;
      f(pos, decoded_subregion);
    }
    assert(decoded_subregion.empty());
  }

  template <typename F>
  void traverse_subregions(const F &f, const region &other) const {
    subregion_t decoded_subregion0, decoded_subregion;

    typedef typename subregions_t::const_iterator subregions_iter_t;
    subregions_iter_t iter0 = subregions.begin();
    subregions_iter_t iter1 = other.subregions.begin();
    const subregions_iter_t end0 = subregions.end();
    const subregions_iter_t end1 = other.subregions.end();
    while (iter0 != end0 || iter1 != end1) {
      const T next_pos0 =
          iter0 != end0 ? iter0->first : numeric_limits<T>::max();
      const T next_pos1 =
          iter1 != end1 ? iter1->first : numeric_limits<T>::max();
      const T pos = min(next_pos0, next_pos1);
      const bool active0 = next_pos0 == pos;
      const bool active1 = next_pos1 == pos;
      subregion_t dummy;
      const subregion_t &subregion0 = active0 ? iter0->second : dummy;
      const subregion_t &subregion = active1 ? iter1->second : dummy;
      if (active0)
        decoded_subregion0 ^= subregion0;
      if (active1)
        decoded_subregion ^= subregion;

      f(pos, decoded_subregion0, decoded_subregion);

      if (active0)
        ++iter0;
      if (active1)
        ++iter1;
    }
    assert(decoded_subregion0.empty());
    assert(decoded_subregion.empty());
  }

  template <typename F> region unary_operator(const F &op) const {
    region res;
    subregion_t old_decoded_subregion;
    traverse_subregions(
        [&](const T pos, const subregion_t &decoded_subregion0) {
          auto decoded_subregion = op(decoded_subregion0);
          auto subregion = decoded_subregion ^ old_decoded_subregion;
          if (!subregion.empty())
            res.subregions.emplace_back(make_pair(pos, move(subregion)));
          old_decoded_subregion = move(decoded_subregion);
        });
    assert(old_decoded_subregion.empty());
    assert(res.invariant());
    return res;
  }

  template <typename F>
  region binary_operator(const F &op, const region &other) const {
    region res;
    subregion_t old_decoded_subregion;
    traverse_subregions(
        [&](const T pos, const subregion_t &decoded_subregion0,
            const subregion_t &decoded_subregion1) {
          auto decoded_subregion = op(decoded_subregion0, decoded_subregion1);
          auto subregion = decoded_subregion ^ old_decoded_subregion;
          if (!subregion.empty())
            res.subregions.emplace_back(make_pair(pos, move(subregion)));
          old_decoded_subregion = move(decoded_subregion);
        },
        other);
    assert(old_decoded_subregion.empty());
    assert(res.invariant());
    return res;
  }

public:
  // Invariant
  bool invariant() const {
#if REGIONCALCULUS_DEBUG
    for (const auto &pos_subregion : subregions) {
      const auto &subregion = pos_subregion.second;
      if (subregion.empty() || !subregion.invariant())
        return false;
    }
    if (chi_size() % 2 != 0)
      return false;
#endif
    return true;
  }

  // Predicates
  bool empty() const { return subregions.empty(); }

  typedef typename point<T, D>::prod_t prod_t;
  prod_t size() const {
    prod_t total_size = 0;
    T old_pos = numeric_limits<T>::min(); // location of last subregion
    prod_t old_subregion_size = 0; // number of points in the last subregion
    traverse_subregions([&](const T pos, const subregion_t &subregion) {
      const prod_t subregion_size = subregion.size();
      total_size += old_subregion_size == 0
                        ? 0
                        : prod_t(pos - old_pos) * old_subregion_size;
      old_pos = pos;
      old_subregion_size = subregion_size;
    });
    assert(old_subregion_size == 0);
    return total_size;
  }

  ptrdiff_t chi_size() const {
    ptrdiff_t sz = 0;
    for (const auto &pos_subregion : subregions) {
      const auto &subregion = pos_subregion.second;
      sz += subregion.chi_size();
    }
    return sz;
  }

  // Conversion from and to boxes
private:
  static region
  region_from_boxes(const typename vector<box<T, D>>::const_iterator &begin,
                    const typename vector<box<T, D>>::const_iterator &end) {
    auto sz = end - begin;
    if (sz == 0)
      return region();
    if (sz == 1)
      return region(*begin);
    const auto mid = begin + sz / 2;
    return region_from_boxes(begin, mid) | region_from_boxes(mid, end);
  }

public:
  region(const vector<box<T, D>> &boxes) {
    *this = region_from_boxes(boxes.begin(), boxes.end());
#if REGIONCALCULUS_DEBUG
    {
      region reg;
      for (const auto &box : boxes)
        reg |= region(box);
      assert(*this == reg);
    }
#endif
  }

  operator vector<box<T, D>>() const {
    vector<box<T, D>> res;
    map<box<T, D - 1>, T> old_subboxes;
    traverse_subregions([&](const T pos, const subregion_t &subregion) {
      // Convert subregion to boxes
      const vector<box<T, D - 1>> subboxes1(subregion);

      auto iter0 = old_subboxes.begin();
      auto iter1 = subboxes1.begin();
      const auto end0 = old_subboxes.end();
      const auto end1 = subboxes1.end();
#if REGIONCALCULUS_DEBUG
      assert(is_sorted(iter1, end1));
#endif
      map<box<T, D - 1>, T> subboxes;
      while (iter0 != end0 || iter1 != end1) {
        bool active0 = iter0 != end0;
        bool active1 = iter1 != end1;
        box<T, D - 1> dummy;
        const box<T, D - 1> &subbox0 = active0 ? iter0->first : dummy;
        const box<T, D - 1> &subbox1 = active1 ? *iter1 : dummy;
        // When both subboxes are active, keep only the first (as determined by
        // less<>)
        std::equal_to<subregion_t> eq;
        std::less<subregion_t> lt;
        if (active0 && active1) {
          active0 = !lt(subbox0, subbox1);
          active1 = !lt(subbox1, subbox0);
        }

        const T old_pos = iter0->second;
        if (active0 && active1 && eq(subbox0, subbox1)) {
          // The current bbox continues unchanged -- keep it
          subboxes[subbox1] = old_pos;
        } else {
          if (active0)
            // The current box changed; finalize it
            res.push_back(box<T, D>(subbox0.lower().superpoint(D - 1, old_pos),
                                    subbox0.upper().superpoint(D - 1, pos)));
          if (active1)
            // There is a new box; add it
            subboxes[subbox1] = pos;
        }

        if (active0)
          ++iter0;
        if (active1)
          ++iter1;
      }
      old_subboxes = move(subboxes);
    });
    assert(old_subboxes.empty());
#if REGIONCALCULUS_DEBUG
    assert(is_sorted(res.begin(), res.end()));
    {
      region reg;
      for (const auto &b : res) {
        assert(region(b).isdisjoint(reg));
        reg |= b;
      }
      assert(reg == *this);
    }
#endif
    return res;
  }

  // Shift and scale operators
  region operator>>(const point<T, D> &d) const {
    region nr;
    nr.subregions.reserve(subregions.size());
    T dx = d[D - 1];
    auto subd = d.subpoint(D - 1);
    for (const auto &pos_subregion : subregions) {
      const T pos = pos_subregion.first;
      const auto &subregion = pos_subregion.second;
      nr.subregions.emplace_back(make_pair(pos + dx, subregion >> subd));
    }
    assert(nr.invariant());
    return nr;
  }
  region operator<<(const point<T, D> &d) const { return *this >> -d; }
  region grow(const point<T, D> &dlo, const point<T, D> &dup) const {
    // Cannot shrink
    assert(all(dlo + dup >= point<T, D>(T(0))));
    // region nr;
    // for (const auto &box : vector<box<T, D>>(*this))
    //   nr |= box.grow(dlo, dup);
    // return nr;
    return reduce([&](const box<T, D> &b) { return region(b.grow(dlo, dup)); },
                  [](const region &x, const region &y) { return x | y; },
                  vector<box<T, D>>(*this));
  }
  region grow(const point<T, D> &d) const { return grow(d, d); }
  region grow(T n) const { return grow(point<T, D>(n)); }
  region shrink(const point<T, D> &dlo, const point<T, D> &dup) const {
    // Cannot grow
    assert(all(dlo + dup >= point<T, D>(T(0))));
    auto world = bounding_box().grow(1);
    return region(world.grow(dup, dlo)) -
           (region(world) - *this).grow(dup, dlo);
  }
  region shrink(const point<T, D> &d) const { return shrink(d, d); }
  region shrink(T n) const { return shrink(point<T, D>(n)); }

  // Set operations
  box<T, D> bounding_box() const {
    if (empty())
      return box<T, D>();
    point<T, D - 1> pmin(numeric_limits<T>::max()),
        pmax(numeric_limits<T>::min());
    for (const auto &pos_subregion : subregions) {
      const auto &subregion = pos_subregion.second;
      auto subbox = subregion.bounding_box();
      pmin = min(pmin, subbox.lower());
      pmax = max(pmax, subbox.upper());
    }
    const T xmin = subregions.begin()->first;
    const T xmax = subregions.rbegin()->first;
    return box<T, D>(pmin.superpoint(D - 1, xmin),
                     pmax.superpoint(D - 1, xmax));
  }

  region operator^(const region &other) const {
    // TODO: If other is much smaller than this, direct insertion may be faster
    return binary_operator([](const subregion_t &set0,
                              const subregion_t &set1) { return set0 ^ set1; },
                           other);
  }

  region operator&(const region &other) const {
    return binary_operator([](const subregion_t &set0,
                              const subregion_t &set1) { return set0 & set1; },
                           other);
  }

  region operator|(const region &other) const {
    return binary_operator([](const subregion_t &set0,
                              const subregion_t &set1) { return set0 | set1; },
                           other);
  }

  region operator-(const region &other) const {
    // return *this & (*this ^ other);
    return binary_operator([](const subregion_t &set0,
                              const subregion_t &set1) { return set0 - set1; },
                           other);
  }

  region &operator^=(const region &other) { return *this = *this ^ other; }
  region &operator&=(const region &other) { return *this = *this & other; }
  region &operator|=(const region &other) { return *this = *this | other; }
  region &operator-=(const region &other) { return *this = *this - other; }

  region intersection(const region &other) const { return *this & other; }
  region setunion(const region &other) const { return *this | other; }
  region symmetric_difference(const region &other) const {
    return *this ^ other;
  }
  region difference(const region &other) const { return *this - other; }

  // Set comparison operators
  bool contains(const point<T, D> &p) const { return !isdisjoint(region(p)); }
  bool isdisjoint(const region &other) const { return (*this & other).empty(); }

  // Comparison operators
  bool operator<=(const region &other) const { return (*this - other).empty(); }
  bool operator>=(const region &other) const { return other <= *this; }
  bool operator<(const region &other) const {
    return *this != other && *this <= other;
  }
  bool operator>(const region &other) const { return other < *this; }
  bool is_subset_of(const region &other) const { return *this <= other; }
  bool is_superset_of(const region &other) const { return *this >= other; }
  bool is_strict_subset_of(const region &other) const { return *this < other; }
  bool is_strict_superset_of(const region &other) const { return *this > other; }
  bool operator==(const region &other) const {
    return subregions == other.subregions;
  }
  bool operator!=(const region &other) const { return !(*this == other); }

  bool equal_to(const region &other) const {
    std::equal_to<subregions_t> eq;
    return eq(subregions, other.subregions);
  }
  bool less(const region &other) const {
    std::less<subregions_t> lt;
    return lt(subregions, other.subregions);
  }
  size_t hash() const {
    size_t r = size_t(0x4eecc6384bcd469dULL);
    for (const auto &p : subregions)
      r = hash_combine(hash_combine(r, p.first), p.second);
    return r;
  }

  // I/O

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  explicit region(const YAML::Node &node) {
    assert(node.Tag() ==
           "tag:github.com/eschnett/SimulationIO/asdf-cxx/region-1.0.0");
    auto dim = node["dimension"].as<int>();
    assert(dim == D);
    // const auto &boxes = node["boxes"].as<vector<box<T, D>>>();
    vector<box<T, D>> boxes;
    for (const auto &b : node["boxes"])
      boxes.push_back(box<T, D>(b));
    *this = region(boxes);
  }
  friend void operator>>(const YAML::Node &node, region &r) {
    r = region(node);
  }
#endif

  ostream &output(ostream &os) const {
    // os << "{";
    // for (const auto &pos_subregion : subregions)
    //   os << pos_subregion.first << ":" << pos_subregion.second << ",";
    // os << "}";
    os << "{";
    const vector<box<T, D>> boxes(*this);
    for (size_t i = 0; i < boxes.size(); ++i) {
      if (i > 0)
        os << ",";
      os << boxes[i];
    }
    os << "}";
    return os;
  }
  friend ostream &operator<<(ostream &os, const region &r) {
    return r.output(os);
  }

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  YAML::Emitter &output(YAML::Emitter &w) const {
    w << YAML::LocalTag("sio", "region-1.0.0");
    w << YAML::Flow << YAML::BeginMap;
    w << YAML::Key << "dimension" << YAML::Value << D;
    w << YAML::Key << "boxes" << YAML::Value << vector<box<T, D>>(*this);
    w << YAML::EndMap;
    return w;
  }
  friend YAML::Emitter &operator<<(YAML::Emitter &w, const region &r) {
    return r.output(w);
  }
#endif
};
} // namespace RegionCalculus

namespace std {
template <typename T, int D> struct equal_to<RegionCalculus::region<T, D>> {
  bool operator()(const RegionCalculus::region<T, D> &p,
                  const RegionCalculus::region<T, D> &q) const {
    return p.equal_to(q);
  }
};
template <typename T, int D> struct less<RegionCalculus::region<T, D>> {
  bool operator()(const RegionCalculus::region<T, D> &p,
                  const RegionCalculus::region<T, D> &q) const {
    return p.less(q);
  }
};
template <typename T, int D> struct hash<RegionCalculus::region<T, D>> {
  size_t operator()(const RegionCalculus::region<T, D> &p) const {
    return p.hash();
  }
};
} // namespace std

#endif // #if REGIONCALCULUS_TREE

////////////////////////////////////////////////////////////////////////////////
// Dimension-independent wrappers
////////////////////////////////////////////////////////////////////////////////

namespace RegionCalculus {

// Virtual classes

template <typename T> struct vpoint {
  virtual ~vpoint() {}

  virtual unique_ptr<vpoint> copy() const = 0;

  static unique_ptr<vpoint> make(int d);
  static unique_ptr<vpoint> make(int d, T x);
  static unique_ptr<vpoint> make(const vector<T> &val);
  template <typename U> static unique_ptr<vpoint> make(const vector<U> &val);
  virtual operator vector<T>() const = 0;
  template <typename U> operator vector<U>() const {
    auto rT(vector<T>(*this));
    vector<U> rU(rT.size());
    for (size_t i = 0; i < rU.size(); ++i)
      rU[i] = U(move(rT[i]));
    return rU;
  }
  template <typename U> static unique_ptr<vpoint> make(const vpoint<U> &p);

  virtual int rank() const = 0;

  // Access and conversion
  virtual T operator[](int d) const = 0;
  virtual T &operator[](int d) = 0;
  virtual unique_ptr<vpoint> subpoint(int dir) const = 0;
  virtual unique_ptr<vpoint> superpoint(int dir, T x) const = 0;
  virtual unique_ptr<vpoint> reversed() const = 0;

  // Unary operators
  virtual unique_ptr<vpoint> operator+() const = 0;
  virtual unique_ptr<vpoint> operator-() const = 0;
  virtual unique_ptr<vpoint> operator~() const = 0;
  virtual unique_ptr<vpoint<bool>> operator!() const = 0;

  // Assignment operators
  virtual vpoint &operator+=(const vpoint &p) = 0;
  virtual vpoint &operator-=(const vpoint &p) = 0;
  virtual vpoint &operator*=(const vpoint &p) = 0;
  virtual vpoint &operator/=(const vpoint &p) = 0;
  virtual vpoint &operator%=(const vpoint &p) = 0;
  virtual vpoint &operator&=(const vpoint &p) = 0;
  virtual vpoint &operator|=(const vpoint &p) = 0;
  virtual vpoint &operator^=(const vpoint &p) = 0;

  // Binary operators
  virtual unique_ptr<vpoint> operator+(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint> operator-(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint> operator*(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint> operator/(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint> operator%(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint> operator&(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint> operator|(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint> operator^(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint<bool>> operator&&(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint<bool>> operator||(const vpoint &p) const = 0;

  // Unary functions
  virtual unique_ptr<vpoint> abs() const = 0;

  // Binary functions
  virtual unique_ptr<vpoint> min(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint> max(const vpoint &p) const = 0;

  // Comparison operators
  virtual unique_ptr<vpoint<bool>> operator==(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint<bool>> operator!=(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint<bool>> operator<(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint<bool>> operator>(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint<bool>> operator<=(const vpoint &p) const = 0;
  virtual unique_ptr<vpoint<bool>> operator>=(const vpoint &p) const = 0;

  virtual bool equal_to(const vpoint &p) const = 0;
  virtual bool less(const vpoint &p) const = 0;
  virtual size_t hash() const = 0;

  // Reductions
  virtual bool any() const = 0;
  virtual bool all() const = 0;
  virtual T minval() const = 0;
  virtual T maxval() const = 0;
  virtual T sum() const = 0;
  typedef typename point<T, 0>::prod_t prod_t;
  virtual prod_t prod() const = 0;

  // I/O
#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  static unique_ptr<vpoint> make(const YAML::Node &node);
#endif
  virtual ostream &output(ostream &os) const = 0;
  friend ostream &operator<<(ostream &os, const vpoint &p) {
    return p.output(os);
  }
#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  virtual YAML::Emitter &output(YAML::Emitter &w) const = 0;
  friend YAML::Emitter &operator<<(YAML::Emitter &w, const vpoint &p) {
    return p.output(w);
  }
#endif
};

template <typename T> struct vbox {
  virtual ~vbox() {}

  virtual unique_ptr<vbox> copy() const = 0;

  static unique_ptr<vbox> make(int d);
  static unique_ptr<vbox> make(const vpoint<T> &lo, const vpoint<T> &hi);
  template <typename U> static unique_ptr<vbox> make(const vbox<U> &p);

  virtual int rank() const = 0;

  // Predicates
  virtual bool empty() const = 0;
  virtual unique_ptr<vpoint<T>> shape() const = 0;
  virtual unique_ptr<vpoint<T>> lower() const = 0;
  virtual unique_ptr<vpoint<T>> upper() const = 0;
  typedef typename box<T, 0>::prod_t prod_t;
  virtual prod_t size() const = 0;

  // Shift and scale operators
  virtual vbox &operator>>=(const vpoint<T> &p) = 0;
  virtual vbox &operator<<=(const vpoint<T> &p) = 0;
  virtual vbox &operator*=(const vpoint<T> &p) = 0;
  virtual unique_ptr<vbox> operator>>(const vpoint<T> &p) const = 0;
  virtual unique_ptr<vbox> operator<<(const vpoint<T> &p) const = 0;
  virtual unique_ptr<vbox> operator*(const vpoint<T> &p) const = 0;
  virtual unique_ptr<vbox> grow(const vpoint<T> &dlo,
                                const vpoint<T> &dup) const = 0;
  virtual unique_ptr<vbox> grow(const vpoint<T> &d) const = 0;
  virtual unique_ptr<vbox> grow(T n) const = 0;
  virtual unique_ptr<vbox> shrink(const vpoint<T> &dlo,
                                  const vpoint<T> &dup) const = 0;
  virtual unique_ptr<vbox> shrink(const vpoint<T> &d) const = 0;
  virtual unique_ptr<vbox> shrink(T n) const = 0;

  // Comparison operators
  virtual bool operator==(const vbox &b) const = 0;

  virtual bool equal_to(const vbox &b) const = 0;
  virtual bool less(const vbox &b) const = 0;
  virtual size_t hash() const = 0;

  // Set comparison operators
  virtual bool contains(const vpoint<T> &p) const = 0;
  virtual bool isdisjoint(const vbox &b) const = 0;
  virtual bool operator<=(const vbox &b) const = 0;
  virtual bool operator<(const vbox &b) const = 0;

  // Set operations
  virtual unique_ptr<vbox> bounding_box(const vbox &b) const = 0;
  virtual unique_ptr<vbox> operator&(const vbox &b) const = 0;
  // virtual unique_ptr<vbox> operator-(const vbox &b) const = 0;
  // virtual unique_ptr<vbox> operator|(const vbox &b) const = 0;
  // virtual unique_ptr<vbox> operator^(const vbox &b) const = 0;

  // I/O
#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  static unique_ptr<vbox> make(const YAML::Node &node);
#endif
  virtual ostream &output(ostream &os) const = 0;
  friend ostream &operator<<(ostream &os, const vbox &b) {
    return b.output(os);
  }
#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  virtual YAML::Emitter &output(YAML::Emitter &w) const = 0;
  friend YAML::Emitter &operator<<(YAML::Emitter &w, const vbox &b) {
    return b.output(w);
  }
#endif
};

template <typename T> struct vregion {
  virtual ~vregion() {}

  virtual unique_ptr<vregion> copy() const = 0;

  static unique_ptr<vregion> make(int d);
  static unique_ptr<vregion> make(const vbox<T> &b);
  static unique_ptr<vregion> make(const vector<unique_ptr<vbox<T>>> &bs);
  virtual operator vector<unique_ptr<vbox<T>>>() const = 0;
  template <typename U> static unique_ptr<vregion> make(const vregion<U> &p);

  virtual int rank() const = 0;

  // Predicates
  virtual bool invariant() const = 0;
  virtual bool empty() const = 0;
  typedef typename region<T, 0>::prod_t prod_t;
  virtual prod_t size() const = 0;

  // Shift and scale operators
  virtual unique_ptr<vregion> operator>>(const vpoint<T> &d) const = 0;
  virtual unique_ptr<vregion> operator<<(const vpoint<T> &d) const = 0;
  virtual unique_ptr<vregion> grow(const vpoint<T> &dlo,
                                   const vpoint<T> &dup) const = 0;
  virtual unique_ptr<vregion> grow(const vpoint<T> &d) const = 0;
  virtual unique_ptr<vregion> grow(T n) const = 0;
  virtual unique_ptr<vregion> shrink(const vpoint<T> &dlo,
                                     const vpoint<T> &dup) const = 0;
  virtual unique_ptr<vregion> shrink(const vpoint<T> &d) const = 0;
  virtual unique_ptr<vregion> shrink(T n) const = 0;

  // Set operations
  virtual unique_ptr<vbox<T>> bounding_box() const = 0;
  virtual unique_ptr<vregion> operator&(const vbox<T> &b) const = 0;
  virtual unique_ptr<vregion> operator&(const vregion &r) const = 0;
  virtual unique_ptr<vregion> operator-(const vbox<T> &b) const = 0;
  virtual unique_ptr<vregion> operator-(const vregion &r) const = 0;
  virtual unique_ptr<vregion> operator|(const vbox<T> &b) const = 0;
  virtual unique_ptr<vregion> operator|(const vregion &r) const = 0;
  virtual unique_ptr<vregion> operator^(const vbox<T> &b) const = 0;
  virtual unique_ptr<vregion> operator^(const vregion &r) const = 0;

  // Set comparison operators
  virtual bool contains(const vpoint<T> &p) const = 0;
  virtual bool isdisjoint(const vbox<T> &b) const = 0;
  virtual bool isdisjoint(const vregion &r) const = 0;

  // Comparison operators
  virtual bool operator<=(const vregion &r) const = 0;
  virtual bool operator>=(const vregion &r) const = 0;
  virtual bool operator<(const vregion &r) const = 0;
  virtual bool operator>(const vregion &r) const = 0;
  virtual bool operator==(const vregion &r) const = 0;
  virtual bool operator!=(const vregion &r) const = 0;

  virtual bool equal_to(const vregion &r) const = 0;
  virtual bool less(const vregion &r) const = 0;
  virtual size_t hash() const = 0;

  // I/O
#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  static unique_ptr<vregion> make(const YAML::Node &node);
#endif
  virtual ostream &output(ostream &os) const = 0;
  friend ostream &operator<<(ostream &os, const vregion &r) {
    return r.output(os);
  }
#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  virtual YAML::Emitter &output(YAML::Emitter &w) const = 0;
  friend YAML::Emitter &operator<<(YAML::Emitter &w, const vregion &r) {
    return r.output(w);
  }
#endif
};

////////////////////////////////////////////////////////////////////////////////

// Wrapper classes (using pointers)

template <typename T, int D> struct wpoint : vpoint<T> {
  point<T, D> val;

  wpoint(const wpoint &p) = default;
  wpoint(wpoint &&p) = default;
  wpoint &operator=(const wpoint &p) = default;
  wpoint &operator=(wpoint &&p) = default;
  unique_ptr<vpoint<T>> copy() const { return make_unique1<wpoint>(*this); }

  wpoint(const point<T, D> &p) : val(p) {}

  wpoint() : val() {}
  wpoint(T x) : val(x) {}
  wpoint(const array<T, D> &p) : val(p) {}
  template <typename U> wpoint(const array<U, D> &p) : val(p) {}
  wpoint(const vector<T> &p) : val(p) {}
  template <typename U> wpoint(const vector<U> &p) : val(p) {}
  operator vector<T>() const { return vector<T>(val); }
  template <typename U> operator vector<U>() const {
    vector<T> rT(val);
    vector<U> rU(rT.size());
    for (size_t i = 0; i < rU.size(); ++i)
      rU[i] = U(move(rT[i]));
    return rU;
  }
  template <typename U> wpoint(const wpoint<U, D> &p) : val(p.val) {}

  int rank() const { return D; }

  // Access and conversion
  T operator[](int d) const { return val[d]; }
  T &operator[](int d) { return val[d]; }
  unique_ptr<vpoint<T>> subpoint(int dir) const {
    return make_unique1<wpoint<T, (D > 0 ? D - 1 : 0)>>(val.subpoint(dir));
  }
  unique_ptr<vpoint<T>> superpoint(int dir, T x) const {
    // This is intentionally wrong for D >= 4 to avoid infinite recursion to
    // ever larger ranks
    return make_unique1<wpoint<T, (D < 4 ? D + 1 : 0)>>(val.superpoint(dir, x));
  }
  unique_ptr<vpoint<T>> reversed() const {
    return make_unique1<wpoint>(val.reversed());
  }

  // Unary operators
  unique_ptr<vpoint<T>> operator+() const { return make_unique1<wpoint>(+val); }
  unique_ptr<vpoint<T>> operator-() const { return make_unique1<wpoint>(-val); }
  unique_ptr<vpoint<T>> operator~() const {
    return make_unique1<wpoint>(
        typename call_if_integral<point<T, D>, bit_not<point<T, D>>>::type()(
            val));
  }
  unique_ptr<vpoint<bool>> operator!() const {
    return make_unique1<wpoint<bool, D>>(!val);
  }

  // Assignment operators
  vpoint<T> &operator+=(const vpoint<T> &p) {
    val += dynamic_cast<const wpoint &>(p).val;
    return *this;
  }
  vpoint<T> &operator-=(const vpoint<T> &p) {
    val -= dynamic_cast<const wpoint &>(p).val;
    return *this;
  }
  vpoint<T> &operator*=(const vpoint<T> &p) {
    val *= dynamic_cast<const wpoint &>(p).val;
    return *this;
  }
  vpoint<T> &operator/=(const vpoint<T> &p) {
    val /= dynamic_cast<const wpoint &>(p).val;
    return *this;
  }
  vpoint<T> &operator%=(const vpoint<T> &p) {
    typename call_if_integral<point<T, D>, modulus_eq<point<T, D>>>::type()(
        val, dynamic_cast<const wpoint &>(p).val);
    return *this;
  }
  vpoint<T> &operator&=(const vpoint<T> &p) {
    typename call_if_integral<point<T, D>, bit_and_eq<point<T, D>>>::type()(
        val, dynamic_cast<const wpoint &>(p).val);
    return *this;
  }
  vpoint<T> &operator|=(const vpoint<T> &p) {
    typename call_if_integral<point<T, D>, bit_or_eq<point<T, D>>>::type()(
        val, dynamic_cast<const wpoint &>(p).val);
    return *this;
  }
  vpoint<T> &operator^=(const vpoint<T> &p) {
    typename call_if_integral<point<T, D>, bit_xor_eq<point<T, D>>>::type()(
        val, dynamic_cast<const wpoint &>(p).val);
    return *this;
  }

  // Binary operators
  unique_ptr<vpoint<T>> operator+(const vpoint<T> &p) const {
    return make_unique1<wpoint>(val + dynamic_cast<const wpoint &>(p).val);
  }
  unique_ptr<vpoint<T>> operator-(const vpoint<T> &p) const {
    return make_unique1<wpoint>(val - dynamic_cast<const wpoint &>(p).val);
  }
  unique_ptr<vpoint<T>> operator*(const vpoint<T> &p) const {
    return make_unique1<wpoint>(val * dynamic_cast<const wpoint &>(p).val);
  }
  unique_ptr<vpoint<T>> operator/(const vpoint<T> &p) const {
    return make_unique1<wpoint>(val / dynamic_cast<const wpoint &>(p).val);
  }
  unique_ptr<vpoint<T>> operator%(const vpoint<T> &p) const {
    return make_unique1<wpoint>(
        typename call_if_integral<point<T, D>, modulus<point<T, D>>>::type()(
            val, dynamic_cast<const wpoint &>(p).val));
  }
  unique_ptr<vpoint<T>> operator&(const vpoint<T> &p) const {
    return make_unique1<wpoint>(
        typename call_if_integral<point<T, D>, bit_and<point<T, D>>>::type()(
            val, dynamic_cast<const wpoint &>(p).val));
  }
  unique_ptr<vpoint<T>> operator|(const vpoint<T> &p) const {
    return make_unique1<wpoint>(
        typename call_if_integral<point<T, D>, bit_or<point<T, D>>>::type()(
            val, dynamic_cast<const wpoint &>(p).val));
  }
  unique_ptr<vpoint<T>> operator^(const vpoint<T> &p) const {
    return make_unique1<wpoint>(
        typename call_if_integral<point<T, D>, bit_xor<point<T, D>>>::type()(
            val, dynamic_cast<const wpoint &>(p).val));
  }
  unique_ptr<vpoint<bool>> operator&&(const vpoint<T> &p) const {
    return make_unique1<wpoint<bool, D>>(val &&
                                         dynamic_cast<const wpoint &>(p).val);
  }
  unique_ptr<vpoint<bool>> operator||(const vpoint<T> &p) const {
    return make_unique1<wpoint<bool, D>>(val ||
                                         dynamic_cast<const wpoint &>(p).val);
  }

  // Unary functions
  unique_ptr<vpoint<T>> abs() const { return make_unique1<wpoint>(val.abs()); }

  // Binary functions
  unique_ptr<vpoint<T>> min(const vpoint<T> &p) const {
    return make_unique1<wpoint>(val.min(dynamic_cast<const wpoint &>(p).val));
  }
  unique_ptr<vpoint<T>> max(const vpoint<T> &p) const {
    return make_unique1<wpoint>(val.max(dynamic_cast<const wpoint &>(p).val));
  }

  // Comparison operators
  unique_ptr<vpoint<bool>> operator==(const vpoint<T> &p) const {
    return make_unique1<wpoint<bool, D>>(val ==
                                         dynamic_cast<const wpoint &>(p).val);
  }
  unique_ptr<vpoint<bool>> operator!=(const vpoint<T> &p) const {
    return make_unique1<wpoint<bool, D>>(val !=
                                         dynamic_cast<const wpoint &>(p).val);
  }
  unique_ptr<vpoint<bool>> operator<(const vpoint<T> &p) const {
    return make_unique1<wpoint<bool, D>>(val <
                                         dynamic_cast<const wpoint &>(p).val);
  }
  unique_ptr<vpoint<bool>> operator>(const vpoint<T> &p) const {
    return make_unique1<wpoint<bool, D>>(val >
                                         dynamic_cast<const wpoint &>(p).val);
  }
  unique_ptr<vpoint<bool>> operator<=(const vpoint<T> &p) const {
    return make_unique1<wpoint<bool, D>>(val <=
                                         dynamic_cast<const wpoint &>(p).val);
  }
  unique_ptr<vpoint<bool>> operator>=(const vpoint<T> &p) const {
    return make_unique1<wpoint<bool, D>>(val >=
                                         dynamic_cast<const wpoint &>(p).val);
  }

  bool equal_to(const vpoint<T> &p) const {
    return val.equal_to(dynamic_cast<const wpoint &>(p).val);
  }
  bool less(const vpoint<T> &p) const {
    return val.less(dynamic_cast<const wpoint &>(p).val);
  }
  size_t hash() const { return val.hash(); }

  // Reductions
  bool any() const { return val.any(); }
  bool all() const { return val.all(); }
  T minval() const { return val.minval(); }
  T maxval() const { return val.maxval(); }
  T sum() const { return val.sum(); }
  typedef typename vpoint<T>::prod_t prod_t;
  prod_t prod() const { return val.prod(); }

  // I/O
#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  explicit wpoint(const YAML::Node &node) : val(node) {}
#endif
  ostream &output(ostream &os) const { return val.output(os); }
  // friend ostream &operator<<(ostream &os, const wpoint &p) {
  //   return p.output(os);
  // }
#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  YAML::Emitter &output(YAML::Emitter &w) const { return val.output(w); }
  // friend YAML::Emitter &operator<<(YAML::Emitter &w, const wpoint &p) {
  //   return p.output(w);
  // }
#endif
};

template <typename T, int D> struct wbox : vbox<T> {
  box<T, D> val;

  wbox(const wbox &b) = default;
  wbox(wbox &&b) = default;
  wbox &operator=(const wbox &b) = default;
  wbox &operator=(wbox &&b) = default;
  unique_ptr<vbox<T>> copy() const { return make_unique1<wbox>(*this); }

  wbox(const box<T, D> &b) : val(b) {}

  wbox() : val() {}
  wbox(const wpoint<T, D> &lo, const wpoint<T, D> &hi) : val(lo.val, hi.val) {}
  template <typename U> wbox(const wbox<U, D> &p) : val(p.val) {}

  int rank() const { return D; }

  // Predicates
  bool empty() const { return val.empty(); }
  unique_ptr<vpoint<T>> lower() const {
    return make_unique1<wpoint<T, D>>(val.lower());
  }
  unique_ptr<vpoint<T>> upper() const {
    return make_unique1<wpoint<T, D>>(val.upper());
  }
  unique_ptr<vpoint<T>> shape() const {
    return make_unique1<wpoint<T, D>>(val.shape());
  }
  typedef typename vbox<T>::prod_t prod_t;
  prod_t size() const { return val.size(); }

  // Shift and scale operators
  vbox<T> &operator>>=(const vpoint<T> &p) {
    val >>= dynamic_cast<const wpoint<T, D> &>(p).val;
    return *this;
  }
  vbox<T> &operator<<=(const vpoint<T> &p) {
    val <<= dynamic_cast<const wpoint<T, D> &>(p).val;
    return *this;
  }
  vbox<T> &operator*=(const vpoint<T> &p) {
    val *= dynamic_cast<const wpoint<T, D> &>(p).val;
    return *this;
  }
  unique_ptr<vbox<T>> operator>>(const vpoint<T> &p) const {
    return make_unique1<wbox>(val >> dynamic_cast<const wpoint<T, D> &>(p).val);
  }
  unique_ptr<vbox<T>> operator<<(const vpoint<T> &p) const {
    return make_unique1<wbox>(val << dynamic_cast<const wpoint<T, D> &>(p).val);
  }
  unique_ptr<vbox<T>> operator*(const vpoint<T> &p) const {
    return make_unique1<wbox>(val * dynamic_cast<const wpoint<T, D> &>(p).val);
  }
  unique_ptr<vbox<T>> grow(const vpoint<T> &dlo, const vpoint<T> &dup) const {
    return make_unique1<wbox>(
        val.grow(dynamic_cast<const wpoint<T, D> &>(dlo).val,
                 dynamic_cast<const wpoint<T, D> &>(dup).val));
  }
  unique_ptr<vbox<T>> grow(const vpoint<T> &d) const {
    return make_unique1<wbox>(
        val.grow(dynamic_cast<const wpoint<T, D> &>(d).val));
  }
  unique_ptr<vbox<T>> grow(T n) const {
    return make_unique1<wbox>(val.grow(n));
  }
  unique_ptr<vbox<T>> shrink(const vpoint<T> &dlo, const vpoint<T> &dup) const {
    return make_unique1<wbox>(
        val.shrink(dynamic_cast<const wpoint<T, D> &>(dlo).val,
                   dynamic_cast<const wpoint<T, D> &>(dup).val));
  }
  unique_ptr<vbox<T>> shrink(const vpoint<T> &d) const {
    return make_unique1<wbox>(
        val.shrink(dynamic_cast<const wpoint<T, D> &>(d).val));
  }
  unique_ptr<vbox<T>> shrink(T n) const {
    return make_unique1<wbox>(val.shrink(n));
  }

  // Comparison operators
  bool operator==(const vbox<T> &b) const {
    return rank() == b.rank() && val == dynamic_cast<const wbox<T, D> &>(b).val;
  }

  bool equal_to(const vbox<T> &b) const {
    return val.equal_to(dynamic_cast<const wbox<T, D> &>(b).val);
  }
  bool less(const vbox<T> &b) const {
    return val.less(dynamic_cast<const wbox<T, D> &>(b).val);
  }
  size_t hash() const { return val.hash(); }

  // Set comparison operators
  bool contains(const vpoint<T> &p) const {
    return val.contains(dynamic_cast<const wpoint<T, D> &>(p).val);
  }
  bool isdisjoint(const vbox<T> &b) const {
    return val.isdisjoint(dynamic_cast<const wbox &>(b).val);
  }
  bool operator<=(const vbox<T> &b) const {
    return val <= dynamic_cast<const wbox &>(b).val;
  }
  bool operator<(const vbox<T> &b) const {
    return val < dynamic_cast<const wbox &>(b).val;
  }

  // Set operations
  unique_ptr<vbox<T>> bounding_box(const vbox<T> &b) const {
    return make_unique1<wbox>(
        val.bounding_box(dynamic_cast<const wbox &>(b).val));
  }
  unique_ptr<vbox<T>> operator&(const vbox<T> &b) const {
    return make_unique1<wbox>(val & dynamic_cast<const wbox &>(b).val);
  }
  // unique_ptr<vbox<T>> operator-(const vbox<T> &b) const {
  //   return make_unique1<wbox>(val - dynamic_cast<const wbox &>(b).val);
  // }
  // unique_ptr<vbox<T>> operator|(const vbox<T> &b) const {
  //   return make_unique1<wbox>(val | dynamic_cast<const wbox &>(b).val);
  // }
  // unique_ptr<vbox<T>> operator^(const vbox<T> &b) const {
  //   return make_unique1<wbox>(val ^ dynamic_cast<const wbox &>(b).val);
  // }

  // I/O
#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  explicit wbox(const YAML::Node &node) : val(node) {}
#endif
  ostream &output(ostream &os) const { return val.output(os); }
  // friend ostream &operator<<(ostream &os, const wbox &b) {
  //   return b.output(os);
  // }
#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  YAML::Emitter &output(YAML::Emitter &w) const { return val.output(w); }
  // friend YAML::Emitter &operator<<(YAML::Emitter &w, const wbox &b) {
  //   return b.output(w);
  // }
#endif
};

template <typename T, int D> struct wregion : vregion<T> {
  region<T, D> val;

  wregion(const wregion &r) = default;
  wregion(wregion &&r) = default;
  wregion &operator=(const wregion &r) = default;
  wregion &operator=(wregion &&r) = default;
  unique_ptr<vregion<T>> copy() const {
    return unique_ptr<vregion<T>>(new wregion(*this));
  }

  wregion(const region<T, D> &r) : val(r) {}
  wregion(region<T, D> &&r) : val(move(r)) {}

  wregion() = default;
  wregion(const wbox<T, D> &b) : val(b.val) {}
  wregion(const vector<unique_ptr<vbox<T>>> &bs) {
    vector<box<T, D>> rs;
    for (const auto &b : bs)
      rs.push_back(dynamic_cast<const wbox<T, D> &>(b).val);
    val = region<T, D>(rs);
  }
  operator vector<unique_ptr<vbox<T>>>() const {
    vector<unique_ptr<vbox<T>>> bs;
    for (const auto &b : vector<box<T, D>>(val))
      bs.push_back(make_unique1<wbox<T, D>>(b));
    return bs;
  }
  template <typename U> wregion(const wregion<U, D> &p) : val(p.val) {}

  int rank() const { return D; }

  // Predicates
  bool invariant() const { return val.invariant(); }
  bool empty() const { return val.empty(); }
  typedef typename vregion<T>::prod_t prod_t;
  prod_t size() const { return val.size(); }

  // Shift and scale operators
  unique_ptr<vregion<T>> operator>>(const vpoint<T> &d) const {
    return make_unique1<wregion>(val >>
                                 dynamic_cast<const wpoint<T, D> &>(d).val);
  }
  unique_ptr<vregion<T>> operator<<(const vpoint<T> &d) const {
    return make_unique1<wregion>(val
                                 << dynamic_cast<const wpoint<T, D> &>(d).val);
  }
  unique_ptr<vregion<T>> grow(const vpoint<T> &dlo,
                              const vpoint<T> &dup) const {
    return make_unique1<wregion>(
        val.grow(dynamic_cast<const wpoint<T, D> &>(dlo).val,
                 dynamic_cast<const wpoint<T, D> &>(dup).val));
  }
  unique_ptr<vregion<T>> grow(const vpoint<T> &d) const {
    return make_unique1<wregion>(
        val.grow(dynamic_cast<const wpoint<T, D> &>(d).val));
  }
  unique_ptr<vregion<T>> grow(T n) const {
    return make_unique1<wregion>(val.grow(n));
  }
  unique_ptr<vregion<T>> shrink(const vpoint<T> &dlo,
                                const vpoint<T> &dup) const {
    return make_unique1<wregion>(
        val.shrink(dynamic_cast<const wpoint<T, D> &>(dlo).val,
                   dynamic_cast<const wpoint<T, D> &>(dup).val));
  }
  unique_ptr<vregion<T>> shrink(const vpoint<T> &d) const {
    return make_unique1<wregion>(
        val.shrink(dynamic_cast<const wpoint<T, D> &>(d).val));
  }
  unique_ptr<vregion<T>> shrink(T n) const {
    return make_unique1<wregion>(val.shrink(n));
  }

  // Set operations
  unique_ptr<vbox<T>> bounding_box() const {
    return make_unique1<wbox<T, D>>(val.bounding_box());
  }
  unique_ptr<vregion<T>> operator&(const vbox<T> &b) const {
    return make_unique1<wregion>(val & dynamic_cast<const wbox<T, D> &>(b).val);
  }
  unique_ptr<vregion<T>> operator&(const vregion<T> &r) const {
    return make_unique1<wregion>(val & dynamic_cast<const wregion &>(r).val);
  }
  unique_ptr<vregion<T>> operator-(const vbox<T> &b) const {
    return make_unique1<wregion>(val - dynamic_cast<const wbox<T, D> &>(b).val);
  }
  unique_ptr<vregion<T>> operator-(const vregion<T> &r) const {
    return make_unique1<wregion>(val - dynamic_cast<const wregion &>(r).val);
  }
  unique_ptr<vregion<T>> operator|(const vbox<T> &b) const {
    return make_unique1<wregion>(val | dynamic_cast<const wbox<T, D> &>(b).val);
  }
  unique_ptr<vregion<T>> operator|(const vregion<T> &r) const {
    return make_unique1<wregion>(val | dynamic_cast<const wregion &>(r).val);
  }
  unique_ptr<vregion<T>> operator^(const vbox<T> &b) const {
    return make_unique1<wregion>(val ^ dynamic_cast<const wbox<T, D> &>(b).val);
  }
  unique_ptr<vregion<T>> operator^(const vregion<T> &r) const {
    return make_unique1<wregion>(val ^ dynamic_cast<const wregion &>(r).val);
  }

  // Set comparison operators
  bool contains(const vpoint<T> &p) const {
    return val.contains(dynamic_cast<const wpoint<T, D> &>(p).val);
  }
  bool isdisjoint(const vbox<T> &b) const {
    return val.isdisjoint(dynamic_cast<const wbox<T, D> &>(b).val);
  }
  bool isdisjoint(const vregion<T> &r) const {
    return val.isdisjoint(dynamic_cast<const wregion &>(r).val);
  }

  // Comparison operators
  bool operator<=(const vregion<T> &r) const {
    return val <= dynamic_cast<const wregion &>(r).val;
  }
  bool operator>=(const vregion<T> &r) const {
    return val >= dynamic_cast<const wregion &>(r).val;
  }
  bool operator<(const vregion<T> &r) const {
    return val < dynamic_cast<const wregion &>(r).val;
  }
  bool operator>(const vregion<T> &r) const {
    return val > dynamic_cast<const wregion &>(r).val;
  }
  bool operator==(const vregion<T> &r) const {
    return rank() == r.rank() && val == dynamic_cast<const wregion &>(r).val;
  }
  bool operator!=(const vregion<T> &r) const {
    return rank() != r.rank() || val != dynamic_cast<const wregion &>(r).val;
  }

  bool equal_to(const vregion<T> &r) const {
    return val.equal_to(dynamic_cast<const wregion &>(r).val);
  }
  bool less(const vregion<T> &r) const {
    return val.less(dynamic_cast<const wregion &>(r).val);
  }
  size_t hash() const { return val.hash(); }

  // I/O
#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  explicit wregion(const YAML::Node &node) : val(node) {}
#endif
  ostream &output(ostream &os) const { return val.output(os); }
  // friend ostream &operator<<(ostream &os, const wregion &r) {
  //   return r.output(os);
  // }
#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  YAML::Emitter &output(YAML::Emitter &w) const { return val.output(w); }
  // friend YAML::Emitter &operator<<(YAML::Emitter &w, const wregion &r) {
  //   return r.output(w);
  // }
#endif
};

////////////////////////////////////////////////////////////////////////////////

// Dispatching functions (replacements for constructors)

template <typename T> unique_ptr<vpoint<T>> vpoint<T>::make(int d) {
  switch (d) {
  case 0:
    return make_unique1<wpoint<T, 0>>();
  case 1:
    return make_unique1<wpoint<T, 1>>();
  case 2:
    return make_unique1<wpoint<T, 2>>();
  case 3:
    return make_unique1<wpoint<T, 3>>();
  case 4:
    return make_unique1<wpoint<T, 4>>();
  default:
    assert(0);
  }
}

template <typename T> unique_ptr<vpoint<T>> vpoint<T>::make(int d, T x) {
  switch (d) {
  case 0:
    return make_unique1<wpoint<T, 0>>(x);
  case 1:
    return make_unique1<wpoint<T, 1>>(x);
  case 2:
    return make_unique1<wpoint<T, 2>>(x);
  case 3:
    return make_unique1<wpoint<T, 3>>(x);
  case 4:
    return make_unique1<wpoint<T, 4>>(x);
  default:
    assert(0);
  }
}

template <typename T>
unique_ptr<vpoint<T>> vpoint<T>::make(const vector<T> &val) {
  switch (val.size()) {
  case 0:
    return make_unique1<wpoint<T, 0>>(val);
  case 1:
    return make_unique1<wpoint<T, 1>>(val);
  case 2:
    return make_unique1<wpoint<T, 2>>(val);
  case 3:
    return make_unique1<wpoint<T, 3>>(val);
  case 4:
    return make_unique1<wpoint<T, 4>>(val);
  default:
    assert(0);
  }
}

template <typename T>
template <typename U>
unique_ptr<vpoint<T>> vpoint<T>::make(const vector<U> &val) {
  switch (val.size()) {
  case 0:
    return make_unique1<wpoint<T, 0>>(val);
  case 1:
    return make_unique1<wpoint<T, 1>>(val);
  case 2:
    return make_unique1<wpoint<T, 2>>(val);
  case 3:
    return make_unique1<wpoint<T, 3>>(val);
  case 4:
    return make_unique1<wpoint<T, 4>>(val);
  default:
    assert(0);
  }
}

template <typename T>
template <typename U>
unique_ptr<vpoint<T>> vpoint<T>::make(const vpoint<U> &p) {
  switch (p.rank()) {
  case 0:
    return make_unique1<wpoint<T, 0>>(dynamic_cast<const wpoint<U, 0> &>(p));
  case 1:
    return make_unique1<wpoint<T, 1>>(dynamic_cast<const wpoint<U, 1> &>(p));
  case 2:
    return make_unique1<wpoint<T, 2>>(dynamic_cast<const wpoint<U, 2> &>(p));
  case 3:
    return make_unique1<wpoint<T, 3>>(dynamic_cast<const wpoint<U, 3> &>(p));
  case 4:
    return make_unique1<wpoint<T, 4>>(dynamic_cast<const wpoint<U, 4> &>(p));
  default:
    assert(0);
  }
}

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
template <typename T>
unique_ptr<vpoint<T>> vpoint<T>::make(const YAML::Node &node) {
  const auto &v = node.as<vector<T>>();
  switch (v.size()) {
  case 0:
    return make_unique1<wpoint<T, 0>>(v);
  case 1:
    return make_unique1<wpoint<T, 1>>(v);
  case 2:
    return make_unique1<wpoint<T, 2>>(v);
  case 3:
    return make_unique1<wpoint<T, 3>>(v);
  case 4:
    return make_unique1<wpoint<T, 4>>(v);
  default:
    assert(0);
  }
}
#endif

template <typename T> unique_ptr<vbox<T>> vbox<T>::make(int d) {
  switch (d) {
  case 0:
    return make_unique1<wbox<T, 0>>();
  case 1:
    return make_unique1<wbox<T, 1>>();
  case 2:
    return make_unique1<wbox<T, 2>>();
  case 3:
    return make_unique1<wbox<T, 3>>();
  case 4:
    return make_unique1<wbox<T, 4>>();
  default:
    assert(0);
  }
}

template <typename T>
unique_ptr<vbox<T>> vbox<T>::make(const vpoint<T> &lo, const vpoint<T> &hi) {
  switch (lo.rank()) {
  case 0:
    return make_unique1<wbox<T, 0>>(dynamic_cast<const wpoint<T, 0> &>(lo),
                                    dynamic_cast<const wpoint<T, 0> &>(hi));
  case 1:
    return make_unique1<wbox<T, 1>>(dynamic_cast<const wpoint<T, 1> &>(lo),
                                    dynamic_cast<const wpoint<T, 1> &>(hi));
  case 2:
    return make_unique1<wbox<T, 2>>(dynamic_cast<const wpoint<T, 2> &>(lo),
                                    dynamic_cast<const wpoint<T, 2> &>(hi));
  case 3:
    return make_unique1<wbox<T, 3>>(dynamic_cast<const wpoint<T, 3> &>(lo),
                                    dynamic_cast<const wpoint<T, 3> &>(hi));
  case 4:
    return make_unique1<wbox<T, 4>>(dynamic_cast<const wpoint<T, 4> &>(lo),
                                    dynamic_cast<const wpoint<T, 4> &>(hi));
  default:
    assert(0);
  }
}

template <typename T>
template <typename U>
unique_ptr<vbox<T>> vbox<T>::make(const vbox<U> &b) {
  switch (b.rank()) {
  case 0:
    return make_unique1<wbox<T, 0>>(dynamic_cast<const wbox<U, 0> &>(b));
  case 1:
    return make_unique1<wbox<T, 1>>(dynamic_cast<const wbox<U, 1> &>(b));
  case 2:
    return make_unique1<wbox<T, 2>>(dynamic_cast<const wbox<U, 2> &>(b));
  case 3:
    return make_unique1<wbox<T, 3>>(dynamic_cast<const wbox<U, 3> &>(b));
  case 4:
    return make_unique1<wbox<T, 4>>(dynamic_cast<const wbox<U, 4> &>(b));
  default:
    assert(0);
  }
}

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
template <typename T>
unique_ptr<vbox<T>> vbox<T>::make(const YAML::Node &node) {
  const auto &full = node["full"];
  if (full.IsDefined())
    // return make_unique1<wbox<T, 0>>(node);
    return make_unique1<wbox<T, 0>>(box<T, 0>(full.as<bool>()));
  const auto &lo = vpoint<T>::make(node["low"]);
  const auto &hi = vpoint<T>::make(node["high"]);
  return vbox<T>::make(*lo, *hi);
}
#endif

template <typename T> unique_ptr<vregion<T>> vregion<T>::make(int d) {
  switch (d) {
  case 0:
    return make_unique1<wregion<T, 0>>();
  case 1:
    return make_unique1<wregion<T, 1>>();
  case 2:
    return make_unique1<wregion<T, 2>>();
  case 3:
    return make_unique1<wregion<T, 3>>();
  case 4:
    return make_unique1<wregion<T, 4>>();
  default:
    assert(0);
  }
}

template <typename T>
unique_ptr<vregion<T>> vregion<T>::make(const vbox<T> &b) {
  switch (b.rank()) {
  case 0:
    return make_unique1<wregion<T, 0>>(dynamic_cast<const wbox<T, 0> &>(b));
  case 1:
    return make_unique1<wregion<T, 1>>(dynamic_cast<const wbox<T, 1> &>(b));
  case 2:
    return make_unique1<wregion<T, 2>>(dynamic_cast<const wbox<T, 2> &>(b));
  case 3:
    return make_unique1<wregion<T, 3>>(dynamic_cast<const wbox<T, 3> &>(b));
  case 4:
    return make_unique1<wregion<T, 4>>(dynamic_cast<const wbox<T, 4> &>(b));
  default:
    assert(0);
  }
}

template <typename T>
unique_ptr<vregion<T>> vregion<T>::make(const vector<unique_ptr<vbox<T>>> &bs) {
  if (bs.empty())
    // Cannot determine rank
    return nullptr;
  switch (bs[0]->rank()) {
  case 0: {
    vector<box<T, 0>> rs;
    for (const auto &b : bs)
      rs.push_back(dynamic_cast<const wbox<T, 0> &>(*b).val);
    return make_unique1<wregion<T, 0>>(rs);
  }
  case 1: {
    vector<box<T, 1>> rs;
    for (const auto &b : bs)
      rs.push_back(dynamic_cast<const wbox<T, 1> &>(*b).val);
    return make_unique1<wregion<T, 1>>(rs);
  }
  case 2: {
    vector<box<T, 2>> rs;
    for (const auto &b : bs)
      rs.push_back(dynamic_cast<const wbox<T, 2> &>(*b).val);
    return make_unique1<wregion<T, 2>>(rs);
  }
  case 3: {
    vector<box<T, 3>> rs;
    for (const auto &b : bs)
      rs.push_back(dynamic_cast<const wbox<T, 3> &>(*b).val);
    return make_unique1<wregion<T, 3>>(rs);
  }
  case 4: {
    vector<box<T, 4>> rs;
    for (const auto &b : bs)
      rs.push_back(dynamic_cast<const wbox<T, 4> &>(*b).val);
    return make_unique1<wregion<T, 4>>(rs);
  }
  default:
    assert(0);
  }
}

template <typename T>
template <typename U>
unique_ptr<vregion<T>> vregion<T>::make(const vregion<U> &r) {
  switch (r.rank()) {
  case 0:
    return make_unique1<wregion<T, 0>>(dynamic_cast<const wregion<U, 0> &>(r));
  case 1:
    return make_unique1<wregion<T, 1>>(dynamic_cast<const wregion<U, 1> &>(r));
  case 2:
    return make_unique1<wregion<T, 2>>(dynamic_cast<const wregion<U, 2> &>(r));
  case 3:
    return make_unique1<wregion<T, 3>>(dynamic_cast<const wregion<U, 3> &>(r));
  case 4:
    return make_unique1<wregion<T, 4>>(dynamic_cast<const wregion<U, 4> &>(r));
  default:
    assert(0);
  }
}

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
template <typename T>
unique_ptr<vregion<T>> vregion<T>::make(const YAML::Node &node) {
  auto dim = node["dimension"].as<int>();
  switch (dim) {
  case 0:
    return make_unique1<wregion<T, 0>>(node);
  case 1:
    return make_unique1<wregion<T, 1>>(node);
  case 2:
    return make_unique1<wregion<T, 2>>(node);
  case 3:
    return make_unique1<wregion<T, 3>>(node);
  case 4:
    return make_unique1<wregion<T, 4>>(node);
  default:
    assert(0);
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////

// Dimension-independent classes (hiding the pointers)

template <typename T> struct dpoint {
  unique_ptr<vpoint<T>> val;

  dpoint() = default;

  dpoint(const dpoint &p) {
    if (p.val)
      val = p.val->copy();
  }
  dpoint(dpoint &&p) = default;
  dpoint &operator=(const dpoint &p) {
    if (p.val)
      val = p.val->copy();
    else
      val = nullptr;
    return *this;
  }
  dpoint &operator=(dpoint &&p) = default;

  template <int D>
  dpoint(const point<T, D> &p) : val(make_unique1<wpoint<T, D>>(p)) {}
  dpoint(const vpoint<T> &p) : val(p.copy()) {}
  dpoint(const unique_ptr<vpoint<T>> &val) {
    if (val)
      this->val = val->copy();
  }
  dpoint(unique_ptr<vpoint<T>> &&val) : val(move(val)) {}

  explicit dpoint(int d) : val(vpoint<T>::make(d)) {}
  dpoint(int d, T x) : val(vpoint<T>::make(d, x)) {}
  template <size_t D>
  dpoint(const array<T, D> &p) : val(make_unique1<wpoint<T, D>>(p)) {}
  template <typename U, size_t D>
  explicit dpoint(const array<U, D> &p) : val(make_unique1<wpoint<T, D>>(p)) {}
  dpoint(const vector<T> &p) : val(vpoint<T>::make(p)) {}
  template <typename U>
  explicit dpoint(const vector<U> &p) : val(vpoint<T>::make(p)) {}
  operator vector<T>() const { return vector<T>(*val); }
  template <typename U> explicit operator vector<U>() const {
    return vector<U>(*val);
  }
  template <typename U> dpoint(const dpoint<U> &p) {
    if (p.val)
      val = vpoint<T>::make(*p.val);
  }
  template <int D> operator point<T, D>() const {
    assert(valid());
    assert(rank() == D);
    return dynamic_cast<const RegionCalculus::wpoint<T, D> *>(val.get())->val;
  }

  bool valid() const { return bool(val); }
  void reset() { val.reset(); }
  int rank() const { return val->rank(); }

  // Access and conversion
  T operator[](int d) const { return (*val)[d]; }
  T &operator[](int d) { return (*val)[d]; }
  dpoint subpoint(int dir) const { return dpoint(val->subpoint(dir)); }
  dpoint superpoint(int dir, T x) const { return dpoint(val->subpoint(dir)); }
  dpoint reversed() const { return dpoint(val->reversed()); }

  // Unary operators
  dpoint operator+() const { return dpoint(+*val); }
  dpoint operator-() const { return dpoint(-*val); }
  dpoint operator~() const { return dpoint(~*val); }
  dpoint<bool> operator!() const { return dpoint<bool>(!*val); }

  // Assignment operators
  dpoint &operator+=(const dpoint &p) {
    *val += *p.val;
    return *this;
  }
  dpoint &operator-=(const dpoint &p) {
    *val -= *p.val;
    return *this;
  }
  dpoint &operator*=(const dpoint &p) {
    *val *= *p.val;
    return *this;
  }
  dpoint &operator/=(const dpoint &p) {
    *val /= *p.val;
    return *this;
  }
  dpoint &operator%=(const dpoint &p) {
    *val %= *p.val;
    return *this;
  }
  dpoint &operator&=(const dpoint &p) {
    *val &= *p.val;
    return *this;
  }
  dpoint &operator|=(const dpoint &p) {
    *val |= *p.val;
    return *this;
  }
  dpoint &operator^=(const dpoint &p) {
    *val ^= *p.val;
    return *this;
  }

  // Binary operators
  dpoint operator+(const dpoint &p) const { return dpoint(*val + *p.val); }
  dpoint operator-(const dpoint &p) const { return dpoint(*val - *p.val); }
  dpoint operator*(const dpoint &p) const { return dpoint(*val * *p.val); }
  dpoint operator/(const dpoint &p) const { return dpoint(*val / *p.val); }
  dpoint operator%(const dpoint &p) const { return dpoint(*val % *p.val); }
  dpoint operator&(const dpoint &p) const { return dpoint(*val & *p.val); }
  dpoint operator|(const dpoint &p) const { return dpoint(*val | *p.val); }
  dpoint operator^(const dpoint &p) const { return dpoint(*val ^ *p.val); }
  dpoint<bool> operator&&(const dpoint &p) const {
    return dpoint<bool>(*val && *p.val);
  }
  dpoint<bool> operator||(const dpoint &p) const {
    return dpoint<bool>(*val || *p.val);
  }

  // Unary functions
  dpoint abs() const { return dpoint(val->abs()); }

  // Binary functions
  dpoint min(const dpoint &p) const { return dpoint(val->min(*p.val)); }
  dpoint max(const dpoint &p) const { return dpoint(val->max(*p.val)); }

  // Comparison operators
  dpoint<bool> operator==(const dpoint &p) const {
    return dpoint<bool>(*val == *p.val);
  }
  dpoint<bool> operator!=(const dpoint &p) const {
    return dpoint<bool>(*val != *p.val);
  }
  dpoint<bool> operator<(const dpoint &p) const {
    return dpoint<bool>(*val < *p.val);
  }
  dpoint<bool> operator>(const dpoint &p) const {
    return dpoint<bool>(*val > *p.val);
  }
  dpoint<bool> operator<=(const dpoint &p) const {
    return dpoint<bool>(*val <= *p.val);
  }
  dpoint<bool> operator>=(const dpoint &p) const {
    return dpoint<bool>(*val >= *p.val);
  }

  bool equal_to(const dpoint &p) const { return val->equal_to(*p.val); }
  bool less(const dpoint &p) const { return val->less(*p.val); }
  size_t hash() const { return val->hash(); }

  // Reductions
  bool all() const { return val->all(); }
  bool any() const { return val->any(); }
  T minval() const { return val->minval(); }
  T maxval() const { return val->maxval(); }
  T sum() const { return val->sum(); }
  typedef typename vpoint<T>::prod_t prod_t;
  prod_t prod() const { return val->prod(); }

  // I/O

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  explicit dpoint(const YAML::Node &node) : val(vpoint<T>::make(node)) {}
#endif

  ostream &output(ostream &os) const {
    if (!val)
      return os << "dpoint()";
    return val->output(os);
  }
  friend ostream &operator<<(ostream &os, const dpoint &p) {
    return p.output(os);
  }

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  YAML::Emitter &output(YAML::Emitter &w) const {
    if (!val)
      return w << YAML::LocalTag("sio", "point-1.0.0");
    return val->output(w);
  }
  friend YAML::Emitter &operator<<(YAML::Emitter &w, const dpoint &p) {
    return p.output(w);
  }
#endif
};

// Unary functions
template <typename T> dpoint<T> abs(const dpoint<T> &p) { return p.abs(); }

// Binary functions
template <typename T> dpoint<T> min(const dpoint<T> &p, const dpoint<T> &q) {
  return p.min(q);
}
template <typename T> dpoint<T> max(const dpoint<T> &p, const dpoint<T> &q) {
  return p.max(q);
}

// Reductions
template <typename T> bool all(const dpoint<T> &p) { return p.all(); }
template <typename T> bool any(const dpoint<T> &p) { return p.any(); }
template <typename T> T minval(const dpoint<T> &p) { return p.minval(); }
template <typename T> T maxval(const dpoint<T> &p) { return p.maxval(); }
template <typename T> T sum(const dpoint<T> &p) { return p.sum(); }
template <typename T> typename dpoint<T>::prod_t prod(const dpoint<T> &p) {
  return p.prod();
}
} // namespace RegionCalculus

namespace std {
template <typename T> struct equal_to<RegionCalculus::dpoint<T>> {
  bool operator()(const RegionCalculus::dpoint<T> &p,
                  const RegionCalculus::dpoint<T> &q) const {
    return p.equal_to(q);
  }
};

template <typename T> struct less<RegionCalculus::dpoint<T>> {
  bool operator()(const RegionCalculus::dpoint<T> &p,
                  const RegionCalculus::dpoint<T> &q) const {
    return p.less(q);
  }
};
template <typename T> struct hash<RegionCalculus::dpoint<T>> {
  size_t operator()(const RegionCalculus::dpoint<T> &p) const {
    return p.hash();
  }
};
} // namespace std

namespace RegionCalculus {
template <typename T> struct dbox {
  unique_ptr<vbox<T>> val;

  dbox() = default;

  dbox(const dbox &b) {
    if (b.val)
      val = b.val->copy();
  }
  dbox(dbox &&b) = default;
  dbox &operator=(const dbox &b) {
    if (b.val)
      val = b.val->copy();
    else
      val = nullptr;
    return *this;
  }
  dbox &operator=(dbox &&b) = default;

  template <int D>
  dbox(const box<T, D> &b) : val(make_unique1<wbox<T, D>>(b)) {}
  dbox(const vbox<T> &b) : val(b.copy()) {}
  dbox(const unique_ptr<vbox<T>> &val) {
    if (val)
      this->val = val->copy();
  }
  dbox(unique_ptr<vbox<T>> &&val) : val(move(val)) {}

  explicit dbox(int d) : val(vbox<T>::make(d)) {}
  dbox(const dpoint<T> &lo, const dpoint<T> &hi)
      : val(vbox<T>::make(*lo.val, *hi.val)) {}
  template <typename U> dbox(const dbox<U> &p) {
    if (p.val)
      val = vbox<T>::make(*p.val);
  }
  template <int D> operator box<T, D>() const {
    assert(valid());
    assert(rank() == D);
    return dynamic_cast<const RegionCalculus::wbox<T, D> *>(val.get())->val;
  }

  bool valid() const { return bool(val); }
  void reset() { val.reset(); }
  int rank() const { return val->rank(); }

  // Predicates
  bool empty() const { return val->empty(); }
  dpoint<T> lower() const { return dpoint<T>(val->lower()); }
  dpoint<T> upper() const { return dpoint<T>(val->upper()); }
  dpoint<T> shape() const { return dpoint<T>(val->shape()); }
  typedef typename vbox<T>::prod_t prod_t;
  prod_t size() const { return val->size(); }

  // Shift and scale operators
  dbox &operator>>=(const dpoint<T> &p) {
    *val >>= *p.val;
    return *this;
  }
  dbox &operator<<=(const dpoint<T> &p) {
    *val <<= *p.val;
    return *this;
  }
  dbox &operator*=(const dpoint<T> &p) {
    *val *= *p.val;
    return *this;
  }
  dbox operator>>(const dpoint<T> &p) const { return dbox(*val >> *p.val); }
  dbox operator<<(const dpoint<T> &p) const { return dbox(*val << *p.val); }
  dbox operator*(const dpoint<T> &p) const { return dbox(*val * *p.val); }
  dbox grow(const dpoint<T> &dlo, const dpoint<T> &dup) const {
    return dbox(val->grow(*dlo.val, *dup.val));
  }
  dbox grow(const dpoint<T> &d) const { return dbox(val->grow(*d)); }
  dbox grow(T n) const { return dbox(val->grow(n)); }
  dbox shrink(const dpoint<T> &dlo, const dpoint<T> &dup) const {
    return dbox(val->shrink(*dlo.val, *dup.val));
  }
  dbox shrink(const dpoint<T> &d) const { return dbox(val->shrink(*d)); }
  dbox shrink(T n) const { return dbox(val->shrink(n)); }

  // Comparison operators
  bool operator==(const dbox &b) const { return *val == *b.val; }
  bool operator!=(const dbox &b) const { return !(*this == b); }
  bool equal_to(const dbox &b) const { return val->equal_to(*b.val); }
  bool less(const dbox &b) const { return val->less(*b.val); }
  size_t hash() const { return val->hash(); }

  // Set comparison operators
  bool contains(const dpoint<T> &p) const { return val->contains(*p.val); }
  bool isdisjoint(const dbox &b) const { return val->isdisjoint(*b.val); }
  bool operator<=(const dbox &b) const { return *val <= *b.val; }
  bool operator>=(const dbox &b) const { return b <= *this; }
  bool operator<(const dbox &b) const { return *val < *b.val; }
  bool operator>(const dbox &b) const { return b < *this; }
  bool is_subset_of(const dbox &b) const { return *this <= b; }
  bool is_superset_of(const dbox &b) const { return *this >= b; }
  bool is_strict_subset_of(const dbox &b) const { return *this < b; }
  bool is_strict_superset_of(const dbox &b) const { return *this > b; }

  // Set operations
  dbox bounding_box(const dbox &b) const {
    return dbox(val->bounding_box(*b.val));
  }
  dbox operator&(const dbox &b) const { return dbox(*val & *b.val); }
  // dbox operator-(const dbox &b) const { return dbox(*val - *b.val); }
  // dbox operator|(const dbox &b) const { return dbox(*val | *b.val); }
  // dbox operator^(const dbox &b) const { return dbox(*val ^ *b.val); }
  dbox intersection(const dbox &b) const { return *this & b; }
  // dbox difference(const dbox &b) const { return *this - b; }
  // dbox setunion(const dbox &b) const { return *this | b; }
  // dbox symmetric_difference(const dbox &b) const { return *this ^ b; }

  // I/O

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  explicit dbox(const YAML::Node &node) : val(vbox<T>::make(node)) {}
#endif

  ostream &output(ostream &os) const {
    if (!val)
      return os << "dbox()";
    return val->output(os);
  }
  friend ostream &operator<<(ostream &os, const dbox &b) {
    return b.output(os);
  }

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  YAML::Emitter &output(YAML::Emitter &w) const {
    if (!val)
      return w << YAML::LocalTag("sio", "box-1.0.0");
    return val->output(w);
  }
  friend YAML::Emitter &operator<<(YAML::Emitter &w, const dbox &b) {
    return b.output(w);
  }
#endif
};
} // namespace RegionCalculus

namespace std {
template <typename T> struct equal_to<RegionCalculus::dbox<T>> {
  bool operator()(const RegionCalculus::dbox<T> &p,
                  const RegionCalculus::dbox<T> &q) const {
    return p.equal_to(q);
  }
};
template <typename T> struct less<RegionCalculus::dbox<T>> {
  bool operator()(const RegionCalculus::dbox<T> &p,
                  const RegionCalculus::dbox<T> &q) const {
    return p.less(q);
  }
};
template <typename T> struct hash<RegionCalculus::dbox<T>> {
  size_t operator()(const RegionCalculus::dbox<T> &p) const { return p.hash(); }
};
} // namespace std

namespace RegionCalculus {
template <typename T> struct dregion {
  unique_ptr<vregion<T>> val;

  dregion() = default;

  dregion(const dregion &r) {
    if (r.val)
      val = r.val->copy();
  }
  dregion(dregion &&r) = default;
  dregion &operator=(const dregion &r) {
    if (r.val)
      val = r.val->copy();
    else
      val = nullptr;
    return *this;
  }
  dregion &operator=(dregion &&r) = default;

  template <int D>
  dregion(const region<T, D> &r) : val(make_unique1<wregion<T, D>>(r)) {}
  dregion(const vregion<T> &r) : val(r.copy()) {}
  dregion(const unique_ptr<vregion<T>> &val) {
    if (val)
      this->val = val->copy();
  }
  dregion(unique_ptr<vregion<T>> &&val) : val(move(val)) {}

  explicit dregion(int d) : val(vregion<T>::make(d)) {}
  dregion(const dbox<T> &b) : val(vregion<T>::make(*b.val)) {}
  dregion(const vector<dbox<T>> &bs) {
    vector<unique_ptr<vbox<T>>> rs;
    for (const auto &b : bs)
      rs.push_back(b.val->copy());
    val = vregion<T>::make(move(rs));
  }
  dregion(vector<dbox<T>> &&bs) {
    vector<unique_ptr<vbox<T>>> rs;
    for (auto &b : bs)
      rs.push_back(move(b.val));
    val = vregion<T>::make(move(rs));
  }
  operator vector<dbox<T>>() const {
    vector<unique_ptr<vbox<T>>> bs(*val);
    vector<dbox<T>> rs;
    for (auto &b : bs)
      rs.push_back(dbox<T>(move(b)));
    return rs;
  }
  template <typename U> dregion(const dregion<U> &p) {
    if (p.val)
      val = vregion<T>::make(*p.val);
  }
  template <int D> operator region<T, D>() const {
    assert(valid());
    assert(rank() == D);
    return dynamic_cast<const RegionCalculus::wregion<T, D> *>(val.get())->val;
  }

  bool valid() const { return bool(val); }
  void reset() { val.reset(); }
  int rank() const { return val->rank(); }

  // Predicates
  bool invariant() const { return val->invariant(); }
  bool empty() const { return val->empty(); }
  typedef typename vregion<T>::prod_t prod_t;
  prod_t size() const { return val->size(); }

  // Shift and scale operators
  dregion operator>>(const dpoint<T> &d) const {
    return dregion(*val >> *d.val);
  }
  dregion operator<<(const dpoint<T> &d) const {
    return dregion(*val << *d.val);
  }
  dregion grow(const dpoint<T> &dlo, const dpoint<T> &dup) const {
    return dregion(val->grow(*dlo, *dup));
  }
  dregion grow(const dpoint<T> &d) const { return dregion(val->grow(*d.val)); }
  dregion grow(T n) const { return dregion(val->grow(n)); }
  dregion shrink(const dpoint<T> &dlo, const dpoint<T> &dup) const {
    return dregion(val->shrink(*dlo.val, *dup.val));
  }
  dregion shrink(const dpoint<T> &d) const {
    return dregion(val->shrink(*d.val));
  }
  dregion shrink(T n) const { return dregion(val->shrink(n)); }

  // Set operations
  dbox<T> bounding_box() const { return dbox<T>(val->bounding_box()); }
  dregion operator&(const dbox<T> &b) const { return dregion(*val & *b.val); }
  dregion operator&(const dregion &r) const { return dregion(*val & *r.val); }
  dregion operator-(const dbox<T> &b) const { return dregion(*val - *b.val); }
  dregion operator-(const dregion &r) const { return dregion(*val - *r.val); }
  dregion operator|(const dbox<T> &b) const { return dregion(*val | *b.val); }
  dregion operator|(const dregion &r) const { return dregion(*val | *r.val); }
  dregion operator^(const dbox<T> &b) const { return dregion(*val ^ *b.val); }
  dregion operator^(const dregion &r) const { return dregion(*val ^ *r.val); }

  dregion &operator^=(const dregion &other) { return *this = *this ^ other; }
  dregion &operator&=(const dregion &other) { return *this = *this & other; }
  dregion &operator|=(const dregion &other) { return *this = *this | other; }
  dregion &operator-=(const dregion &other) { return *this = *this - other; }

  dregion intersection(const dbox<T> &b) const { return *this & b; }
  dregion intersection(const dregion &r) const { return *this & r; }
  dregion difference(const dbox<T> &b) const { return *this - b; }
  dregion difference(const dregion &r) const { return *this - r; }
  dregion setunion(const dbox<T> &b) const { return *this | b; }
  dregion setunion(const dregion &r) const { return *this | r; }
  dregion symmetric_difference(const dbox<T> &b) const { return *this ^ b; }
  dregion symmetric_difference(const dregion &r) const { return *this ^ r; }

  // Set comparison operators
  bool contains(const dpoint<T> &p) const { return val->contains(*p.val); }
  bool isdisjoint(const dbox<T> &b) const { return val->isdisjoint(*b.val); }
  bool isdisjoint(const dregion &r) const { return val->isdisjoint(*r.val); }

  // Comparison operators
  bool operator<=(const dregion &r) const { return *val <= *r.val; }
  bool operator>=(const dregion &r) const { return *val >= *r.val; }
  bool operator<(const dregion &r) const { return *val < *r.val; }
  bool operator>(const dregion &r) const { return *val > *r.val; }
  bool is_subset_of(const dregion &r) const { return *this <= r; }
  bool is_superset_of(const dregion &r) const { return *this >= r; }
  bool is_strict_subset_of(const dregion &r) const { return *this < r; }
  bool is_strict_superset_of(const dregion &r) const { return *this > r; }
  bool operator==(const dregion &r) const { return *val == *r.val; }
  bool operator!=(const dregion &r) const { return *val != *r.val; }

  bool equal_to(const dregion &r) const { return val->equal_to(*r.val); }
  bool less(const dregion &r) const { return val->less(*r.val); }
  size_t hash() const { return val->hash(); }

  // I/O

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  explicit dregion(const YAML::Node &node) : val(vregion<T>::make(node)) {}
#endif

  ostream &output(ostream &os) const {
    if (!val)
      return os << "dregion()";
    return val->output(os);
  }
  friend ostream &operator<<(ostream &os, const dregion &r) {
    return r.output(os);
  }

#ifdef SIMULATIONIO_HAVE_ASDF_CXX
  YAML::Emitter &output(YAML::Emitter &w) const {
    if (!val)
      return w << YAML::LocalTag("sio", "region-1.0.0");
    return val->output(w);
  }
  friend YAML::Emitter &operator<<(YAML::Emitter &w, const dregion &r) {
    return r.output(w);
  }
#endif
};

} // namespace RegionCalculus

namespace std {
template <typename T> struct equal_to<RegionCalculus::dregion<T>> {
  bool operator()(const RegionCalculus::dregion<T> &p,
                  const RegionCalculus::dregion<T> &q) const {
    return p.equal_to(q);
  }
};
template <typename T> struct less<RegionCalculus::dregion<T>> {
  bool operator()(const RegionCalculus::dregion<T> &p,
                  const RegionCalculus::dregion<T> &q) const {
    return p.less(q);
  }
};
template <typename T> struct hash<RegionCalculus::dregion<T>> {
  size_t operator()(const RegionCalculus::dregion<T> &p) const {
    return p.hash();
  }
};
} // namespace std

namespace RegionCalculus {
typedef RegionCalculus::dpoint<long long> point_t;
typedef RegionCalculus::dbox<long long> box_t;
typedef RegionCalculus::dregion<long long> region_t;

} // namespace RegionCalculus

#endif // REGIONCALCULUS_HPP
