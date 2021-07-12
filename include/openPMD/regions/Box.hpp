#pragma once

#include "Helpers.hpp"
#include "Point.hpp"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <tuple>
#include <vector>

namespace openPMD {
namespace Regions {

/** A D-dimensional box
 *
 * A box is described by two points, its lower bound (inclusive) and
 * upper bound (exclusive). If the lower bound not less than the upper
 * bound, the box is empty. Empty boxes are fine (similar to an empty
 * array).
 *
 * The dimension D needs to be known at compile time. @see NDBox
 */
template <typename T, std::size_t D> class Box;

template <typename T> class Box<T, 0> {
  bool is_full;

  explicit constexpr Box(bool is_full_) : is_full(is_full_) {}

public:
  constexpr static std::size_t D = 0;

  typedef typename Point<T, D>::value_type value_type;
  typedef typename Point<T, D>::size_type size_type;

  /** Create empty box
   */
  constexpr Box() : is_full(false) {}

  Box(const Box &) = default;
  Box(Box &&) = default;
  Box &operator=(const Box &) = default;
  Box &operator=(Box &&) = default;

  template <typename U> Box(const Box<U, D> &b) : is_full(b.is_full) {}

  /** Create box from lower (inclusive) and upper (exclusive) bound
   */
  Box(const Point<T, D> &, const Point<T, D> &) : is_full(false) {}
  /** Create box holding a single point
   */
  explicit Box(const Point<T, D> &) : is_full(true) {}

  // Predicates
  size_type ndims() const { return D; }
  bool empty() const { return !is_full; }
  Point<T, D> lower() const { return Point<T, D>(); }
  Point<T, D> upper() const { return Point<T, D>(); }
  Point<T, D> shape() const { return Point<T, D>(); }
  size_type size() const { return is_full; }

  // Shift and scale operators
  Box &operator>>=(const Point<T, D> &) { return *this; }
  Box &operator<<=(const Point<T, D> &) { return *this; }
  Box &operator*=(const Point<T, D> &) { return *this; }
  Box operator>>(const Point<T, D> &) const { return Box(*this); }
  Box operator<<(const Point<T, D> &) const { return Box(*this); }
  Box operator*(const Point<T, D> &) const { return Box(*this); }
  Box grown(const Point<T, D> &, const Point<T, D> &) const { return *this; }
  Box grown(const Point<T, D> &) const { return *this; }
  Box grown(const T &) const { return *this; }
  Box shrunk(const Point<T, D> &, const Point<T, D> &) const { return *this; }
  Box shrunk(const Point<T, D> &) const { return *this; }
  Box shrunk(const T &) const { return *this; }

  // Comparison operators
  friend bool operator==(const Box &b1, const Box &b2) {
    return b1.empty() == b2.empty();
  }
  friend bool operator!=(const Box &b1, const Box &b2) { return !(b1 == b2); }

  // Set comparison operators
  bool contains(const Point<T, D> &) const { return !empty(); }
  friend bool isdisjoint(const Box &b1, const Box &b2) {
    return b1.empty() || b2.empty();
  }
  friend bool operator<=(const Box &b1, const Box &b2) {
    return !b1.empty() <= !b2.empty();
  }
  friend bool operator>=(const Box &b1, const Box &b2) { return b2 <= b1; }
  friend bool operator<(const Box &b1, const Box &b2) {
    return b1 <= b2 && b1 != b2;
  }
  friend bool operator>(const Box &b1, const Box &b2) { return b2 < b1; }
  bool is_subset_of(const Box &b) const { return *this <= b; }
  bool is_superset_of(const Box &b) const { return *this >= b; }
  bool is_strict_subset_of(const Box &b) const { return *this < b; }
  bool is_strict_superset_of(const Box &b) const { return *this > b; }

  // Set operations
  friend Box bounding_box(const Box &b1, const Box &b2) {
    return Box(!b1.empty() || !b2.empty());
  }

  friend Box operator&(const Box &b1, const Box &b2) {
    return Box(!b1.empty() & !b2.empty());
  }
  Box &operator&=(const Box &b) { return *this = *this & b; }
  friend Box intersection(const Box &b1, const Box &b2) { return b1 & b2; }

  friend bool operator==(const Box &b, const std::vector<Box> &bs) {
    // This assumes that the elements of bs are disjoint
    return b.empty() == bs.empty();
  }
  friend bool operator==(const std::vector<Box> &bs, const Box &b) {
    return b == bs;
  }
  friend bool operator!=(const Box &b, const std::vector<Box> &bs) {
    return !(b == bs);
  }
  friend bool operator!=(const std::vector<Box> &bs, const Box &b) {
    return !(bs == b);
  }

  friend std::vector<Box> operator-(const Box &b1, const Box &b2) {
    if (!b1.empty() > b2.empty())
      return std::vector<Box>{Box(Point<T, D>())};
    return std::vector<Box>{};
  }
  friend std::vector<Box> difference(const Box &b1, const Box &b2) {
    return b1 - b2;
  }

  friend std::vector<Box> operator|(const Box &b1, const Box &b2) {
    if (!b1.empty() | !b2.empty())
      return std::vector<Box>{Box(Point<T, D>())};
    return std::vector<Box>{};
  }
  friend std::vector<Box> setunion(const Box &b1, const Box &b2) {
    return b1 | b2;
  }

  friend std::vector<Box> operator^(const Box &b1, const Box &b2) {
    if (!b1.empty() ^ !b2.empty())
      return std::vector<Box>{Box(Point<T, D>())};
    return std::vector<Box>{};
  }
  friend std::vector<Box> symmetric_difference(const Box &b1, const Box &b2) {
    return b1 ^ b2;
  }

  /** Output a box
   */
  friend std::ostream &operator<<(std::ostream &os, const Box &b) {
    return os << "(" << b.is_full << ")";
  }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, std::size_t D> class Box {
  Point<T, D> lo, hi;

public:
  typedef typename Point<T, D>::value_type value_type;
  typedef typename Point<T, D>::size_type size_type;

  /** Create empty box
   */
  Box() = default;

  Box(const Box &) = default;
  Box(Box &&) = default;
  Box &operator=(const Box &) = default;
  Box &operator=(Box &&) = default;

  template <typename U> Box(const Box<U, D> &b) : lo(b.lo), hi(b.hi) {}

  /** Create box from lower (inclusive) and upper (exclusive) bound
   */
  Box(const Point<T, D> &lo_, const Point<T, D> &hi_) : lo(lo_), hi(hi_) {}
  /** Create box holding a single point
   */
  explicit Box(const Point<T, D> &p) : lo(p), hi(p + T(1)) {}

  // Predicates
  /** Number of dimensions
   */
  size_type ndims() const { return D; }
  /** Whether the Box is empty
   */
  bool empty() const { return any(hi <= lo); }
  /** Lower bound (inclusive)
   */
  Point<T, D> lower() const { return lo; }
  /** Upper bound (exclusive)
   */
  Point<T, D> upper() const { return hi; }
  /** Shape, i.e. the "size" in each direction
   */
  Point<T, D> shape() const { return max(hi - lo, T(0)); }
  /** Size, the total number of contained points
   */
  size_type size() const { return product(shape()); }

  // Shift and scale operators
  /** Shift a Box to the right (upwards). The shift can be negative, which
   * shifts left.
   */
  Box &operator>>=(const Point<T, D> &p) {
    lo += p;
    hi += p;
    return *this;
  }
  /** Shift a Box to the left (downwards). The shift can be negative, which
   * shifts right.
   */
  Box &operator<<=(const Point<T, D> &p) {
    lo -= p;
    hi -= p;
    return *this;
  }
  /** Scale a Box
   */
  Box &operator*=(const Point<T, D> &p) {
    lo *= p;
    hi *= p;
    return *this;
  }
  Box operator>>(const Point<T, D> &p) const { return Box(*this) >>= p; }
  Box operator<<(const Point<T, D> &p) const { return Box(*this) <<= p; }
  Box operator*(const Point<T, D> &p) const { return Box(*this) *= p; }
  /** Grow a Box by given amounts in each direction.
   *
   * The growth can be negative, which shrinks the box. If a Box is
   * shrunk too much it becomes empty. Growing an empty box always
   * results in an empty box.
   */
  Box grown(const Point<T, D> &dlo, const Point<T, D> &dup) const {
    if (empty())
      return Box();
    Box nb(*this);
    nb.lo -= dlo;
    nb.hi += dup;
    return nb;
  }
  Box grown(const Point<T, D> &d) const { return grown(d, d); }
  Box grown(const T &d) const { return grown(Point<T, D>::pure(d)); }
  /** Shrink a Box by given amounts in each direction.
   *
   * The shrinkage can be negative, which grows the box. If a Box is
   * shrunk too much it becomes empty. Growing an empty box always
   * results in an empty box.
   */
  Box shrunk(const Point<T, D> &dlo, const Point<T, D> &dup) const {
    return grown(-dlo, -dup);
  }
  Box shrunk(const Point<T, D> &d) const { return shrunk(d, d); }
  Box shrunk(const T &d) const { return shrunk(Point<T, D>::pure(d)); }

  // Comparison operators
  /** Compare two boxes
   *
   * (All empty boxes are equal.)
   */
  friend bool operator==(const Box &b1, const Box &b2) {
    if (b1.empty() && b2.empty())
      return true;
    if (b1.empty() || b2.empty())
      return false;
    return all(b1.lo == b2.lo && b1.hi == b2.hi);
  }
  friend bool operator!=(const Box &b1, const Box &b2) { return !(b1 == b2); }

  // Set comparison operators
  /** Check whether a Box contains a given point
   */
  bool contains(const Point<T, D> &p) const {
    if (empty())
      return false;
    return all(p >= lo && p < hi);
  }
  /** Check whether two boxes are disjoint, i.e. whether they have no point in
   * common
   */
  friend bool isdisjoint(const Box &b1, const Box &b2) {
    return (b1 & b2).empty();
  }
  /** Check whether Box 1 is (completely) contained in Box 2
   */
  friend bool operator<=(const Box &b1, const Box &b2) {
    if (b1.empty())
      return true;
    if (b2.empty())
      return false;
    return all(b1.lo >= b2.lo && b1.hi <= b2.hi);
  }
  /** Check whether Box 1 (completely) contains Box 2
   */
  friend bool operator>=(const Box &b1, const Box &b2) { return b2 <= b1; }
  /** Check whether Box 1 is (completely) contained in Box 2, and Box
   * 2 is larger than Box 1
   */
  friend bool operator<(const Box &b1, const Box &b2) {
    return b1 <= b2 && b1 != b2;
  }
  /** Check whether Box 1 (completely) contains in Box 2, and Box
   * 1 is larger than Box 2
   */
  friend bool operator>(const Box &b1, const Box &b2) { return b2 < b1; }
  /** Check whether a Box is a subset of another Box. This is equivalen to `<=`.
   */
  bool is_subset_of(const Box &b) const { return *this <= b; }
  /** Check whether a Box is a superset of another Box. This is equivalen to
   * `>=`.
   */
  bool is_superset_of(const Box &b) const { return *this >= b; }
  /** Check whether a Box is a strict subset of another Box. This is equivalent
   * to `<`.
   */
  bool is_strict_subset_of(const Box &b) const { return *this < b; }
  /** Check whether a Box is a strict superset of another Box. This is
   * equivalent to `>`.
   */
  bool is_strict_superset_of(const Box &b) const { return *this > b; }

  // Set operations
  /** Calculate the bounding box of two Boxes. This is the smallest Box that
   * contains both Boxes.
   */
  friend Box bounding_box(const Box &b1, const Box &b2) {
    if (b1.empty())
      return b2;
    if (b2.empty())
      return b1;
    auto r = Box(min(b1.lo, b2.lo), max(b1.hi, b2.hi));
// Postcondition
#if REGIONS_DEBUG
    assert(b1 <= r && b2 <= r);
#endif
    return r;
  }

  friend Box operator&(const Box &b1, const Box &b2) {
    auto nlo = max(b1.lo, b2.lo);
    auto nhi = min(b1.hi, b2.hi);
    auto r = Box(nlo, nhi);
// Postcondition
#if REGIONS_DEBUG
    assert(r <= b1 && r <= b2);
#endif
    return r;
  }
  /** Calculate the intersection between two Boxes
   *
   * Other set operations (union, difference, symmetric difference) are not
   * supported for Boxes; use Regions instead.
   */
  Box &operator&=(const Box &b) { return *this = *this & b; }
  /** Calculate the intersection between two Boxes
   */
  friend Box intersection(const Box &b1, const Box &b2) { return b1 & b2; }

  /** Output a Box
   */
  friend std::ostream &operator<<(std::ostream &os, const Box &b) {
    return os << "(" << b.lo << ":" << b.hi << ")";
  }
};

} // namespace Regions
} // namespace openPMD

namespace std {

template <typename T, std::size_t D>
struct equal_to<openPMD::Regions::Box<T, D>>;

template <typename T> struct equal_to<openPMD::Regions::Box<T, 0>> {
  constexpr bool operator()(const openPMD::Regions::Box<T, 0> &x,
                            const openPMD::Regions::Box<T, 0> &y) const {
    return x.empty() == y.empty();
  }
};

template <typename T, std::size_t D>
struct equal_to<openPMD::Regions::Box<T, D>> {
  constexpr bool operator()(const openPMD::Regions::Box<T, D> &x,
                            const openPMD::Regions::Box<T, D> &y) const {
    if (x.empty() && y.empty())
      return true;
    if (x.empty() || y.empty())
      return false;
    return openPMD::Regions::helpers::tuple_eq(
        make_tuple(x.lower(), x.upper()), make_tuple(y.lower(), y.upper()));
  }
};

template <typename T, std::size_t D> struct hash<openPMD::Regions::Box<T, D>> {
  constexpr size_t operator()(const openPMD::Regions::Box<T, D> &x) const {
    if (x.empty())
      return size_t(0xc9df21a36550a048ULL);
    hash<openPMD::Regions::Point<T, D>> h;
    return openPMD::Regions::helpers::hash_combine(h(x.lower()), h(x.upper()));
  }
};

template <typename T, std::size_t D> struct less<openPMD::Regions::Box<T, D>>;

template <typename T> struct less<openPMD::Regions::Box<T, 0>> {
  constexpr bool operator()(const openPMD::Regions::Box<T, 0> &x,
                            const openPMD::Regions::Box<T, 0> &y) const {
    return !x.empty() < !y.empty();
  }
};

template <typename T, std::size_t D> struct less<openPMD::Regions::Box<T, D>> {
  constexpr bool operator()(const openPMD::Regions::Box<T, D> &x,
                            const openPMD::Regions::Box<T, D> &y) const {
    if (x.empty() && y.empty())
      return false;
    if (x.empty())
      return true;
    if (y.empty())
      return false;
    return openPMD::Regions::helpers::tuple_lt(
        make_tuple(x.lower(), x.upper()), make_tuple(y.lower(), y.upper()));
  }
};

} // namespace std
