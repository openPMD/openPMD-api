#ifndef REGIONS_REGION_HPP
#define REGIONS_REGION_HPP

#include "Box.hpp"
#include "Helpers.hpp"
#include "Point.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <map>
#include <utility>
#include <vector>

namespace openPMD {
namespace Regions {

/** A D-dimensional region
 *
 * A region is an arbitrarily shaped set of points. The internal
 * representation is likely based on boxes, and is thus most efficient
 * if the region has many axis-aligned boundaries.
 *
 * The dimension D needs to be known at compile time. @see NDRegion
 */
template <typename T, std::size_t D> class Region;

////////////////////////////////////////////////////////////////////////////////

template <typename T> class Region<T, 0> {
public:
  constexpr static std::size_t D = 0;

private:
  bool is_full;

  friend class std::equal_to<Region<T, D>>;
  friend class std::less<Region<T, D>>;
  friend class Region<T, D + 1>;

  // This should be private, but GCC 9 seems to ignore the friend
  // declaration above
public:
  explicit constexpr Region(bool is_full_) : is_full(is_full_) {}

private:
public:
  typedef typename Point<T, D>::value_type value_type;
  typedef typename Point<T, D>::size_type size_type;

  /** Invariant
   */
  constexpr bool invariant() const { return true; }

  void check_invariant() const {
#if REGIONS_DEBUG
    assert(invariant());
#endif
  }

  /** Create empty region
   */
  constexpr Region() : is_full(false) {}

  Region(const Region &) = default;
  Region(Region &&) = default;
  Region &operator=(const Region &) = default;
  Region &operator=(Region &&) = default;

  Region(const Point<T, D> &) : is_full(true) {}
  Region(const Box<T, D> &b) : is_full(!b.empty()) {}

  template <typename U>
  Region(const Region<U, D> &region) : is_full(region.is_full) {}

  Region(const std::vector<Box<T, D>> &bs) {
    is_full = false;
    for (const auto &b : bs)
      is_full |= !b.empty();
  }
  operator std::vector<Box<T, D>>() const {
    if (empty())
      return std::vector<Box<T, D>>{};
    return std::vector<Box<T, D>>{Box<T, D>(Point<T, D>())};
  }

  // Predicates
  size_type ndims() const { return D; }
  bool empty() const { return !is_full; }
  size_type size() const { return is_full; }
  size_type nboxes() const { return is_full; }

  // Comparison operators
  friend bool operator==(const Region &region1, const Region &region2) {
    return region1.empty() == region2.empty();
  }
  friend bool operator!=(const Region &region1, const Region &region2) {
    return !(region1 == region2);
  }
  friend bool operator==(const Region &region1, const Box<T, D> &box2) {
    return region1 == Region(box2);
  }
  friend bool operator!=(const Region &region1, const Box<T, D> &box2) {
    return region1 != Region(box2);
  }
  friend bool operator==(const Box<T, D> &box1, const Region &region2) {
    return Region(box1) == region2;
  }
  friend bool operator!=(const Box<T, D> &box1, const Region &region2) {
    return Region(box1) != region2;
  }

  // Shift and scale operators
  Region operator>>(const Point<T, D> &) const { return *this; }
  Region operator<<(const Point<T, D> &) const { return *this; }
  Region &operator>>=(const Point<T, D> &d) { return *this = *this >> d; }
  Region &operator<<=(const Point<T, D> &d) { return *this = *this << d; }
  Region operator*(const Point<T, D> &) const { return *this; }
  Region &operator*=(const Point<T, D> &s) { return *this = *this * s; }
  Region grown(const Point<T, D> &, const Point<T, D> &) const { return *this; }
  Region grown(const Point<T, D> &d) const { return grown(d, d); }
  Region grown(T n) const { return grown(Point<T, D>::pure(n)); }
  Region shrunk(const Point<T, D> &, const Point<T, D> &) const {
    return *this;
  }
  Region shrunk(const Point<T, D> &d) const { return shrunk(d, d); }
  Region shrunk(T n) const { return shrunk(Point<T, D>::pure(n)); }

  // Set operations
  friend Box<T, D> bounding_box(const Region &region) {
    if (region.empty())
      return Box<T, D>();
    return Box<T, D>(Point<T, D>());
  }

  friend Region operator&(const Region &region1, const Region &region2) {
    return Region(!region1.empty() & !region2.empty());
  }
  friend Region operator|(const Region &region1, const Region &region2) {
    return Region(!region1.empty() | !region2.empty());
  }
  friend Region operator^(const Region &region1, const Region &region2) {
    return Region(!region1.empty() ^ !region2.empty());
  }
  friend Region operator-(const Region &region1, const Region &region2) {
    return Region(!region1.empty() & region2.empty());
  }

  Region &operator&=(const Region &region) { return *this = *this & region; }
  Region &operator|=(const Region &region) { return *this = *this | region; }
  Region &operator^=(const Region &region) { return *this = *this ^ region; }
  Region &operator-=(const Region &region) { return *this = *this - region; }

  friend Region intersection(const Region &region1, const Region &region2) {
    return region1 & region2;
  }
  friend Region setunion(const Region &region1, const Region &region2) {
    return region1 | region2;
  }
  friend Region symmetric_difference(const Region &region1,
                                     const Region &region2) {
    return region1 ^ region2;
  }
  friend Region difference(const Region &region1, const Region &region2) {
    return region1 - region2;
  }

  // Set comparison operators
  bool contains(const Point<T, D> &p) const { return !isdisjoint(Region(p)); }
  friend bool isdisjoint(const Region &region1, const Region &region2) {
    return (region1 & region2).empty();
  }

  // Comparison operators
  friend bool operator<=(const Region &region1, const Region &region2) {
    return (region1 - region2).empty();
  }
  friend bool operator>=(const Region &region1, const Region &region2) {
    return region2 <= region1;
  }
  friend bool operator<(const Region &region1, const Region &region2) {
    return region1 != region2 && region1 <= region2;
  }
  friend bool operator>(const Region &region1, const Region &region2) {
    return region2 < region1;
  }

  bool issubset(const Region &region) const { return *this <= region; }
  bool issuperset(const Region &region) const { return *this >= region; }
  bool is_strict_subset(const Region &region) const { return *this < region; }
  bool is_strict_superset(const Region &region) const { return *this > region; }

  friend std::ostream &operator<<(std::ostream &os, const Region &region) {
    os << "{";
    if (!region.empty())
      os << "(1)";
    os << "}";
    return os;
  }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T> class Region<T, 1> {
public:
  constexpr static std::size_t D = 1;

private:
  typedef Region<T, D - 1> Subregion; // This is essentially a bool
  typedef std::vector<T> Subregions;
  Subregions subregions;

  friend class std::equal_to<Region<T, D>>;
  friend class std::less<Region<T, D>>;

public:
  typedef typename Point<T, D>::value_type value_type;
  typedef typename Point<T, D>::size_type size_type;

  /** Invariant
   */
  bool invariant() const {
    const size_type nboxes = subregions.size();
    for (size_type i = 1; i < nboxes; ++i)
      // The subregions must be ordered
      if (!(subregions[i - 1] < subregions[i])) {
        assert(false);
        return false;
      }
    // There must be an even number of subregions
    if (subregions.size() % 2 != 0) {
      assert(false);
      return false;
    }
    return true;
  }

  void check_invariant() const {
#if REGIONS_DEBUG
    assert(invariant());
#endif
  }

  /** Create empty region
   */
  Region() = default;

  Region(const Region &) = default;
  Region(Region &&) = default;
  Region &operator=(const Region &) = default;
  Region &operator=(Region &&) = default;

  Region(const Point<T, D> &p) : Region(Box<T, D>(p)) {}
  Region(const Box<T, D> &b) {
    if (b.empty())
      return;
    subregions = {b.lower()[0], b.upper()[0]};
    check_invariant();
  }

  template <typename U>
  Region(const Region<U, D> &region) : subregions(region.subregions.size()) {
    std::transform(region.subregions.begin(), region.subregions.end(),
                   subregions.begin(),
                   [](const auto &subregion) { return Subregion(subregion); });
  }

private:
  static std::vector<T> subregions_from_bounds(std::vector<T> lbnds,
                                               std::vector<T> ubnds) {
    const std::size_t nboxes = lbnds.size();
    assert(ubnds.size() == nboxes);
    std::vector<T> subregions;
    if (nboxes == 0)
      return subregions;
    std::sort(lbnds.begin(), lbnds.end());
    std::sort(ubnds.begin(), ubnds.end());
    std::size_t nactive = 0;
    std::size_t lpos = 0, upos = 0;
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
#if REGIONS_DEBUG
    // TODO: turn this into a test case
    {
      Region reg;
      for (std::size_t i = 0; i < nboxes; ++i)
        reg |= Region(Box<T, D>(Point<T, 1>{lbnds[i]}, Point<T, 1>{ubnds[i]}));
      assert(subregions == reg.subregions);
    }
#endif
    return subregions;
  }

public:
  Region(const std::vector<Box<T, D>> &boxes) {
    std::vector<T> lbnds, ubnds;
    lbnds.reserve(boxes.size());
    ubnds.reserve(boxes.size());
    for (const auto &box : boxes) {
      lbnds.push_back(box.lower()[0]);
      ubnds.push_back(box.upper()[0]);
    }
    subregions = subregions_from_bounds(std::move(lbnds), std::move(ubnds));
    check_invariant();
  }
  operator std::vector<Box<T, D>>() const {
    std::vector<Box<T, D>> res;
    res.reserve(subregions.size() / 2);
    auto iter = subregions.begin();
    const auto end = subregions.end();
    while (iter != end) {
      const auto pos1 = *iter++;
      const auto pos2 = *iter++;
      res.emplace_back(Box<T, D>(Point<T, D>{pos1}, Point<T, D>{pos2}));
    }
#if REGIONS_DEBUG
    // TODO: turn this into a test case
    assert(std::is_sorted(res.begin(), res.end()));
    {
      Region reg;
      for (const auto &b : res) {
        assert(isdisjoint(Region(b), reg));
        reg |= b;
      }
      if (!(reg == *this)) {
        std::cout << "subregions=[";
        for (const auto &sr : subregions)
          std::cout << sr << ",";
        std::cout << "]\n";
        std::cout << "reg=[";
        for (const auto &r : reg.subregions)
          std::cout << r << ",";
        std::cout << "]\n";
      }
      assert(reg == *this);
    }
#endif
    return res;
  }

private:
  template <typename F>
  friend void traverse_subregions(const F &f, const Region &region) {
    Subregion decoded_subregion;
    for (const auto &pos : region.subregions) {
      decoded_subregion ^= Subregion(Point<T, D - 1>());
      f(pos, decoded_subregion);
    }
    assert(decoded_subregion.empty());
  }

  template <typename F>
  friend void traverse_subregions(const F &f, const Region &region1,
                                  const Region &region2) {
    Subregion decoded_subregion1, decoded_subregion2;

    auto iter1 = region1.subregions.begin();
    auto iter2 = region2.subregions.begin();
    const auto end1 = region1.subregions.end();
    const auto end2 = region2.subregions.end();
    while (iter1 != end1 && iter2 != end2) {
      const T next_pos1 = *iter1;
      const T next_pos2 = *iter2;
      using std::min;
      const T pos = min(next_pos1, next_pos2);
      const bool active1 = next_pos1 == pos;
      const bool active2 = next_pos2 == pos;
      decoded_subregion1 ^= Subregion(active1);
      decoded_subregion2 ^= Subregion(active2);
      f(pos, decoded_subregion1, decoded_subregion2);
      if (active1)
        ++iter1;
      if (active2)
        ++iter2;
    }
    for (; iter1 != end1; ++iter1) {
      const T pos = *iter1;
      decoded_subregion1 ^= Subregion(Point<T, D - 1>());
      f(pos, decoded_subregion1, Subregion());
    }
    for (; iter2 != end2; ++iter2) {
      const T pos = *iter2;
      decoded_subregion2 ^= Subregion(Point<T, D - 1>());
      f(pos, Subregion(), decoded_subregion2);
    }
    assert(decoded_subregion1.empty());
    assert(decoded_subregion2.empty());
  }

  template <typename F>
  friend Region unary_operator(const F &op, const Region &region1) {
    Region res;
    Subregion old_decoded_subregion;
    traverse_subregions(
        [&](const T pos, const Subregion &decoded_subregion1) {
          auto decoded_subregion_res = op(decoded_subregion1);
          auto subregion = decoded_subregion_res ^ old_decoded_subregion;
          if (!subregion.empty())
            res.subregions.push_back(pos);
          old_decoded_subregion = decoded_subregion_res;
        },
        region1);
    assert(old_decoded_subregion.empty());
    assert(res.invariant());
    return res;
  }

  template <typename F>
  friend Region binary_operator(const F &op, const Region &region1,
                                const Region &region2) {
    Region res;
    Subregion old_decoded_subregion;
    traverse_subregions(
        [&](const T pos, const Subregion &decoded_subregion1,
            const Subregion &decoded_subregion2) {
          auto decoded_subregion_res =
              op(decoded_subregion1, decoded_subregion2);
          auto subregion = decoded_subregion_res ^ old_decoded_subregion;
          if (!subregion.empty())
            res.subregions.push_back(pos);
          old_decoded_subregion = std::move(decoded_subregion_res);
        },
        region1, region2);
    assert(old_decoded_subregion.empty());
    assert(res.invariant());
    return res;
  }

public:
  // Predicates
  size_type ndims() const { return D; }
  bool empty() const { return subregions.empty(); }

  size_type size() const {
    size_type total_size = 0;
    auto iter = subregions.begin();
    const auto end = subregions.end();
    while (iter != end) {
      const auto pos0 = *iter++;
      const auto pos1 = *iter++;
      total_size += pos1 - pos0;
    }
    return total_size;
  }

  size_type nboxes() const { return subregions.size(); }

  // Comparison operators
  friend bool operator==(const Region &region1, const Region &region2) {
    return region1.subregions == region2.subregions;
  }
  friend bool operator!=(const Region &region1, const Region &region2) {
    return !(region1 == region2);
  }
  friend bool operator==(const Region &region1, const Box<T, D> &box2) {
    return region1 == Region(box2);
  }
  friend bool operator!=(const Region &region1, const Box<T, D> &box2) {
    return region1 != Region(box2);
  }
  friend bool operator==(const Box<T, D> &box1, const Region &region2) {
    return Region(box1) == region2;
  }
  friend bool operator!=(const Box<T, D> &box1, const Region &region2) {
    return Region(box1) != region2;
  }

  // Shift and scale operators
  Region operator>>(const Point<T, D> &d) const {
    Region nr;
    nr.subregions.reserve(subregions.size());
    for (const auto &pos : subregions)
      nr.subregions.push_back(pos + d[0]);
    check_invariant();
    return nr;
  }
  Region operator<<(const Point<T, D> &d) const { return *this >> -d; }
  Region &operator>>=(const Point<T, D> &d) { return *this = *this >> d; }
  Region &operator<<=(const Point<T, D> &d) { return *this = *this << d; }
  Region operator*(const Point<T, D> &s) const {
    if (s[0] == 0)
      return empty() ? Region() : Region(Point<T, D>());
    Region nr;
    nr.subregions.reserve(subregions.size());
    for (const auto &pos : subregions)
      nr.subregions.push_back(pos * s[0]);
    if (s[0] < 0)
      std::reverse(nr.subregions.begin(), nr.subregions.end());
    check_invariant();
    return nr;
  }
  Region &operator*=(const Point<T, D> &s) { return *this = *this * s; }

private:
  Region grown_(const Point<T, D> &dlo, const Point<T, D> &dup) const {
    // Cannot shrink
    assert(all(dlo + dup >= Point<T, D>()));
    return helpers::mapreduce(
        [&](const Box<T, D> &b) { return Region(b.grown(dlo, dup)); },
        [](const Region &x, const Region &y) { return x | y; },
        std::vector<Box<T, D>>(*this));
  }
  Region shrunk_(const Point<T, D> &dlo, const Point<T, D> &dup) const {
    // Cannot grow
    assert(all(dlo + dup >= Point<T, D>()));
    auto world = bounding_box(*this).grown(1);
    return Region(world.grown(dup, dlo)) -
           (Region(world) - *this).grown(dup, dlo);
  }

public:
  Region grown(const Point<T, D> &dlo, const Point<T, D> &dup) const {
    const Region &region0 = *this;
    const bool need_shrink = any(dlo + dup < Point<T, D>());
    Region shrunk_region;
    if (need_shrink) {
      // Shrink in some directions
      const Point<T, D> ndlo = fmap(
          [](auto lo, auto up) { return lo + up < 0 ? lo : T(0); }, dlo, dup);
      const Point<T, D> ndup = fmap(
          [](auto lo, auto up) { return lo + up < 0 ? up : T(0); }, dlo, dup);
      shrunk_region = region0.shrunk_(-ndlo, -ndup);
    }
    const Region &region1 = need_shrink ? shrunk_region : *this;
    const bool need_grow = any(dlo + dup > Point<T, D>());
    Region grown_region;
    if (need_grow) {
      // Grow in some direction
      const Point<T, D> ndlo = fmap(
          [](auto lo, auto up) { return lo + up > 0 ? lo : T(0); }, dlo, dup);
      const Point<T, D> ndup = fmap(
          [](auto lo, auto up) { return lo + up > 0 ? up : T(0); }, dlo, dup);
      grown_region = region1.grown_(ndlo, ndup);
    }
    const Region &region2 = need_grow ? grown_region : region1;
    return region2;
  }
  Region grown(const Point<T, D> &d) const { return grown(d, d); }
  Region grown(const T &n) const { return grown(Point<T, D>::pure(n)); }
  Region shrunk(const Point<T, D> &dlo, const Point<T, D> &dup) const {
    return grown(-dlo, -dup);
  }
  Region shrunk(const Point<T, D> &d) const { return shrunk(d, d); }
  Region shrunk(T n) const { return shrunk(Point<T, D>(n)); }

  // Set operations
  friend Box<T, D> bounding_box(const Region &region) {
    if (region.empty())
      return Box<T, D>();
    return Box<T, D>(Point<T, D>{*region.subregions.begin()},
                     Point<T, D>{*(region.subregions.end() - 1)});
  }

  friend Region operator&(const Region &region1, const Region &region2) {
    return binary_operator([](const Subregion &set1,
                              const Subregion &set2) { return set1 & set2; },
                           region1, region2);
  }
  friend Region operator|(const Region &region1, const Region &region2) {
    return binary_operator([](const Subregion &set1,
                              const Subregion &set2) { return set1 | set2; },
                           region1, region2);
  }
  friend Region operator^(const Region &region1, const Region &region2) {
    return binary_operator([](const Subregion &set1,
                              const Subregion &set2) { return set1 ^ set2; },
                           region1, region2);
  }
  friend Region operator-(const Region &region1, const Region &region2) {
    return binary_operator([](const Subregion &set1,
                              const Subregion &set2) { return set1 - set2; },
                           region1, region2);
  }

  Region &operator&=(const Region &region) { return *this = *this & region; }
  Region &operator|=(const Region &region) { return *this = *this | region; }
  Region &operator^=(const Region &region) { return *this = *this ^ region; }
  Region &operator-=(const Region &region) { return *this = *this - region; }

  friend Region intersection(const Region &region1, const Region &region2) {
    return region1 & region2;
  }
  friend Region setunion(const Region &region1, const Region &region2) {
    return region1 | region2;
  }
  friend Region symmetric_difference(const Region &region1,
                                     const Region &region2) {
    return region1 ^ region2;
  }
  friend Region difference(const Region &region1, const Region &region2) {
    return region1 - region2;
  }

  // Set comparison operators
  bool contains(const Point<T, D> &p) const { return !isdisjoint(Region(p)); }
  friend bool isdisjoint(const Region &region1, const Region &region2) {
    return (region1 & region2).empty();
  }

  // Comparison operators
  friend bool operator<=(const Region &region1, const Region &region2) {
    return (region1 - region2).empty();
  }
  friend bool operator>=(const Region &region1, const Region &region2) {
    return region2 <= region1;
  }
  friend bool operator<(const Region &region1, const Region &region2) {
    return region1 != region2 && region1 <= region2;
  }
  friend bool operator>(const Region &region1, const Region &region2) {
    return region2 < region1;
  }

  bool issubset(const Region &region) const { return *this <= region; }
  bool issuperset(const Region &region) const { return *this >= region; }
  bool is_strict_subset(const Region &region) const { return *this < region; }
  bool is_strict_superset(const Region &region) const { return *this > region; }

  friend std::ostream &operator<<(std::ostream &os, const Region &region) {
    os << "{";
    const std::vector<Box<T, D>> boxes(region);
    for (std::size_t i = 0; i < boxes.size(); ++i) {
      if (i > 0)
        os << ",";
      os << boxes[i];
    }
    os << "}";
    return os;
  }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, std::size_t D> class Region {
  typedef Region<T, D - 1> Subregion;
  typedef std::vector<std::pair<T, Subregion>> Subregions;
  Subregions subregions;

  friend class std::equal_to<Region<T, D>>;
  friend class std::less<Region<T, D>>;

public:
  typedef typename Point<T, D>::value_type value_type;
  typedef typename Point<T, D>::size_type size_type;

  /** Invariant
   */
  bool invariant() const {
    for (const auto &pos_subregion : subregions) {
      const auto &subregion = pos_subregion.second;
      // The subregions must not be empty, and their invariants must hold
      if (subregion.empty() || !subregion.invariant()) {
        assert(false);
        return false;
      }
    }
    return true;
  }

  void check_invariant() const {
#if REGIONS_DEBUG
    assert(invariant());
#endif
  }

  /** Create empty region
   */
  Region() = default;

  Region(const Region &) = default;
  Region(Region &&) = default;
  Region &operator=(const Region &) = default;
  Region &operator=(Region &&) = default;

  Region(const Point<T, D> &p) : Region(Box<T, D>(p)) {}
  Region(const Box<T, D> &b) {
    if (b.empty())
      return;
    Box<T, D - 1> subbox(b.lower().erase(D - 1), b.upper().erase(D - 1));
    subregions = {{b.lower()[D - 1], Subregion(subbox)},
                  {b.upper()[D - 1], Subregion(subbox)}};
    check_invariant();
  }

  template <typename U>
  Region(const Region<U, D> &region) : subregions(region.subregions.size()) {
    std::transform(region.subregions.begin(), region.subregions.end(),
                   subregions.begin(), [&](const auto &pos_subregion) {
                     T pos(pos_subregion.first);
                     Subregion subregion(pos_subregion.second);
                     subregions.emplace_back(
                         std::make_pair(pos, std::move(subregion)));
                   });
  }

private:
  static Region region_from_boxes(
      const typename std::vector<Box<T, D>>::const_iterator &begin,
      const typename std::vector<Box<T, D>>::const_iterator &end) {
    auto sz = end - begin;
    if (sz == 0)
      return Region();
    if (sz == 1)
      return Region(*begin);
    const auto mid = begin + sz / 2;
    return region_from_boxes(begin, mid) | region_from_boxes(mid, end);
  }

public:
  Region(const std::vector<Box<T, D>> &boxes) {
    *this = region_from_boxes(boxes.begin(), boxes.end());
#if REGIONS_DEBUG
    // TODO: turn this into a test case
    {
      Region reg;
      for (const auto &box : boxes)
        reg |= region(box);
      assert(*this == reg);
    }
#endif
    check_invariant();
  }

  operator std::vector<Box<T, D>>() const {
    std::vector<Box<T, D>> res;
    std::map<Box<T, D - 1>, T> old_subboxes;
    traverse_subregions(
        [&](const T pos, const Subregion &subregion) {
          // Convert subregion to boxes
          const std::vector<Box<T, D - 1>> subboxes1(subregion);

          auto iter0 = old_subboxes.begin();
          auto iter1 = subboxes1.begin();
          const auto end0 = old_subboxes.end();
          const auto end1 = subboxes1.end();
#if REGIONS_DEBUG
          assert(is_sorted(iter1, end1));
#endif
          std::map<Box<T, D - 1>, T> subboxes;
          while (iter0 != end0 || iter1 != end1) {
            bool active0 = iter0 != end0;
            bool active1 = iter1 != end1;
            Box<T, D - 1> dummy;
            const Box<T, D - 1> &subbox0 = active0 ? iter0->first : dummy;
            const Box<T, D - 1> &subbox1 = active1 ? *iter1 : dummy;
            // When both subboxes are active, keep only the first (as determined
            // by less<>)
            std::equal_to<Subregion> eq;
            std::less<Subregion> lt;
            if (active0 && active1) {
              active0 = !lt(subbox0, subbox1);
              active1 = !lt(subbox1, subbox0);
            }

            if (active0 && active1 && eq(subbox0, subbox1)) {
              // The current bbox continues unchanged -- keep it
              const T old_pos = iter0->second;
              subboxes[subbox1] = old_pos;
            } else {
              if (active0) {
                // The current box changed; finalize it
                const T old_pos = iter0->second;
                res.push_back(Box<T, D>(subbox0.lower().insert(D - 1, old_pos),
                                        subbox0.upper().insert(D - 1, pos)));
              }
              if (active1)
                // There is a new box; add it
                subboxes[subbox1] = pos;
            }

            if (active0)
              ++iter0;
            if (active1)
              ++iter1;
          }
          old_subboxes = std::move(subboxes);
        },
        *this);
    assert(old_subboxes.empty());
#if REGIONS_DEBUG
    // TODO: turn this into a test case
    assert(std::is_sorted(res.begin(), res.end()));
    {
      Region reg;
      for (const auto &b : res) {
        assert(isdisjoint(Region(b), reg));
        reg |= b;
      }
      assert(reg == *this);
    }
#endif
    return res;
  }

private:
  template <typename F>
  friend void traverse_subregions(const F &f, const Region &region) {
    Subregion decoded_subregion;
    for (const auto &pos_subregion : region.subregions) {
      const T pos = pos_subregion.first;
      const auto &subregion = pos_subregion.second;
      decoded_subregion ^= subregion;
      f(pos, decoded_subregion);
    }
    assert(decoded_subregion.empty());
  }

  template <typename F>
  friend void traverse_subregions(const F &f, const Region &region1,
                                  const Region &region2) {
    Subregion decoded_subregion1, decoded_subregion2;

    auto iter1 = region1.subregions.begin();
    auto iter2 = region2.subregions.begin();
    const auto end1 = region1.subregions.end();
    const auto end2 = region2.subregions.end();
    while (iter1 != end1 || iter2 != end2) {
      const T next_pos1 =
          iter1 != end1 ? iter1->first : std::numeric_limits<T>::max();
      const T next_pos2 =
          iter2 != end2 ? iter2->first : std::numeric_limits<T>::max();
      using std::min;
      const T pos = min(next_pos1, next_pos2);
      const bool active1 = next_pos1 == pos;
      const bool active2 = next_pos2 == pos;
      Subregion dummy;
      const Subregion &subregion1 = active1 ? iter1->second : dummy;
      const Subregion &subregion2 = active2 ? iter2->second : dummy;
      if (active1)
        decoded_subregion1 ^= subregion1;
      if (active2)
        decoded_subregion2 ^= subregion2;

      f(pos, decoded_subregion1, decoded_subregion2);

      if (active1)
        ++iter1;
      if (active2)
        ++iter2;
    }
    assert(decoded_subregion1.empty());
    assert(decoded_subregion2.empty());
  }

  template <typename F>
  friend Region unary_operator(const F &op, const Region &region1) {
    Region res;
    Subregion old_decoded_subregion;
    traverse_subregions(
        [&](const T pos, const Subregion &decoded_subregion1) {
          auto decoded_subregion_res = op(decoded_subregion1);
          auto subregion = decoded_subregion_res ^ old_decoded_subregion;
          if (!subregion.empty())
            res.subregions.emplace_back(
                std::make_pair(pos, std::move(subregion)));
          old_decoded_subregion = std::move(decoded_subregion_res);
        },
        region1);
    assert(old_decoded_subregion.empty());
    res.check_invariant();
    return res;
  }

  template <typename F>
  friend Region binary_operator(const F &op, const Region &region1,
                                const Region &region2) {
    Region res;
    Subregion old_decoded_subregion;
    traverse_subregions(
        [&](const T pos, const Subregion &decoded_subregion1,
            const Subregion &decoded_subregion2) {
          auto decoded_subregion_res =
              op(decoded_subregion1, decoded_subregion2);
          auto subregion = decoded_subregion_res ^ old_decoded_subregion;
          if (!subregion.empty())
            res.subregions.emplace_back(
                std::make_pair(pos, std::move(subregion)));
          old_decoded_subregion = std::move(decoded_subregion_res);
        },
        region1, region2);
    assert(old_decoded_subregion.empty());
    res.check_invariant();
    return res;
  }

public:
  // Predicates
  size_type ndims() const { return D; }
  bool empty() const { return subregions.empty(); }

  size_type size() const {
    size_type total_size = 0;
    T old_pos = std::numeric_limits<T>::min(); // location of last subregion
    size_type old_subregion_size = 0; // number of points in the last subregion
    traverse_subregions([&](const T pos, const Subregion &subregion) {
      const size_type subregion_size = subregion.size();
      total_size +=
          old_subregion_size == 0 ? 0 : (pos - old_pos) * old_subregion_size;
      old_pos = pos;
      old_subregion_size = subregion_size;
    });
    assert(old_subregion_size == 0);
    return total_size;
  }

  size_type nboxes() const {
    size_type sz = 0;
    for (const auto &pos_subregion : subregions) {
      const auto &subregion = pos_subregion.second;
      sz += subregion.nboxes();
    }
    return sz;
  }

  // Shift and scale operators
  Region operator>>(const Point<T, D> &d) const {
    Region nr;
    nr.subregions.reserve(subregions.size());
    const T dx = d[D - 1];
    auto subd = d.erase(D - 1);
    for (const auto &pos_subregion : subregions) {
      const T pos = pos_subregion.first;
      const auto &subregion = pos_subregion.second;
      nr.subregions.emplace_back(std::make_pair(pos + dx, subregion >> subd));
    }
    nr.check_invariant();
    return nr;
  }
  Region operator<<(const Point<T, D> &d) const { return *this >> -d; }
  Region &operator>>=(const Point<T, D> &d) { return *this = *this >> d; }
  Region &operator<<=(const Point<T, D> &d) { return *this = *this << d; }
  Region operator*(const Point<T, D> &s) const {
    if (s[D - 1] == 0)
      return empty() ? Region() : Region(Point<T, D>());
    Region nr;
    nr.subregions.reserve(subregions.size());
    const T ds = s[D - 1];
    auto subs = s.erase(D - 1);
    for (const auto &pos_subregion : subregions) {
      const T pos = pos_subregion.first;
      const auto &subregion = pos_subregion.second;
      nr.subregions.emplace_back(std::make_pair(pos * ds, subregion * subs));
    }
    if (ds < 0) {
      std::reverse(nr.subregions.begin(), nr.subregions.end());
      std::transform(nr.subregions.begin(), nr.subregions.end(),
                     nr.subregions.begin(), [](const auto &pos_subregion) {
                       const auto &pos = pos_subregion.first;
                       const auto &subregion = pos_subregion.second;
                       return std::make_pair(pos + 1, subregion);
                     });
    }
    nr.check_invariant();
    return nr;
  }
  Region &operator*=(const Point<T, D> &s) { return *this = *this * s; }

private:
  Region grown_(const Point<T, D> &dlo, const Point<T, D> &dup) const {
    // Cannot shrink
    assert(all(dlo + dup >= Point<T, D>()));
    return helpers::mapreduce(
        [&](const Box<T, D> &b) { return Region(b.grown(dlo, dup)); },
        [](const Region &x, const Region &y) { return x | y; },
        std::vector<Box<T, D>>(*this));
  }
  Region shrunk_(const Point<T, D> &dlo, const Point<T, D> &dup) const {
    // Cannot grow
    assert(all(dlo + dup >= Point<T, D>()));
    auto world = bounding_box(*this).grown(1);
    return Region(world.grown(dup, dlo)) -
           (Region(world) - *this).grown(dup, dlo);
  }

public:
  Region grown(const Point<T, D> &dlo, const Point<T, D> &dup) const {
    const Region &region0 = *this;
    const bool need_shrink = any(dlo + dup < Point<T, D>());
    Region shrunk_region;
    if (need_shrink) {
      // Shrink in some directions
      const Point<T, D> ndlo = fmap(
          [](auto lo, auto up) { return lo + up < 0 ? lo : T(0); }, dlo, dup);
      const Point<T, D> ndup = fmap(
          [](auto lo, auto up) { return lo + up < 0 ? up : T(0); }, dlo, dup);
      shrunk_region = region0.shrunk_(-ndlo, -ndup);
    }
    const Region &region1 = need_shrink ? shrunk_region : *this;
    const bool need_grow = any(dlo + dup > Point<T, D>());
    Region grown_region;
    if (need_grow) {
      // Grow in some direction
      const Point<T, D> ndlo = fmap(
          [](auto lo, auto up) { return lo + up > 0 ? lo : T(0); }, dlo, dup);
      const Point<T, D> ndup = fmap(
          [](auto lo, auto up) { return lo + up > 0 ? up : T(0); }, dlo, dup);
      grown_region = region1.grown_(ndlo, ndup);
    }
    const Region &region2 = need_grow ? grown_region : region1;
    return region2;
  }
  Region grown(const Point<T, D> &d) const { return grown(d, d); }
  Region grown(const T &n) const { return grown(Point<T, D>::pure(n)); }
  Region shrunk(const Point<T, D> &dlo, const Point<T, D> &dup) const {
    return grown(-dlo, -dup);
  }
  Region shrunk(const Point<T, D> &d) const { return shrunk(d, d); }
  Region shrunk(const T &n) const { return shrunk(Point<T, D>::pure(n)); }

  // Comparison operators
  friend bool operator==(const Region &region1, const Region &region2) {
    return region1.subregions == region2.subregions;
  }
  friend bool operator!=(const Region &region1, const Region &region2) {
    return !(region1 == region2);
  }
  friend bool operator==(const Region &region1, const Box<T, D> &box2) {
    return region1 == Region(box2);
  }
  friend bool operator!=(const Region &region1, const Box<T, D> &box2) {
    return region1 != Region(box2);
  }
  friend bool operator==(const Box<T, D> &box1, const Region &region2) {
    return Region(box1) == region2;
  }
  friend bool operator!=(const Box<T, D> &box1, const Region &region2) {
    return Region(box1) != region2;
  }

  // Set operations
  friend Box<T, D> bounding_box(const Region &region) {
    if (region.empty())
      return Box<T, D>();
    auto pmin = Point<T, D - 1>::pure(std::numeric_limits<T>::max());
    auto pmax = Point<T, D - 1>::pure(std::numeric_limits<T>::min());
    for (const auto &pos_subregion : region.subregions) {
      const auto &subregion = pos_subregion.second;
      auto subbox = bounding_box(subregion);
      using std::max, std::min;
      pmin = min(pmin, subbox.lower());
      pmax = max(pmax, subbox.upper());
    }
    const T xmin = region.subregions.begin()->first;
    const T xmax = (region.subregions.end() - 1)->first;
    return Box<T, D>(pmin.insert(D - 1, xmin), pmax.insert(D - 1, xmax));
  }

  friend Region operator&(const Region &region1, const Region &region2) {
    return binary_operator([](const Subregion &set1,
                              const Subregion &set2) { return set1 & set2; },
                           region1, region2);
  }
  friend Region operator|(const Region &region1, const Region &region2) {
    return binary_operator([](const Subregion &set1,
                              const Subregion &set2) { return set1 | set2; },
                           region1, region2);
  }
  friend Region operator^(const Region &region1, const Region &region2) {
    // TODO: If region2 is much smaller than region1, direct insertion may be
    // faster
    return binary_operator([](const Subregion &set1,
                              const Subregion &set2) { return set1 ^ set2; },
                           region1, region2);
  }
  friend Region operator-(const Region &region1, const Region &region2) {
    return binary_operator([](const Subregion &set1,
                              const Subregion &set2) { return set1 - set2; },
                           region1, region2);
  }

  Region &operator&=(const Region &region) { return *this = *this & region; }
  Region &operator|=(const Region &region) { return *this = *this | region; }
  Region &operator^=(const Region &region) { return *this = *this ^ region; }
  Region &operator-=(const Region &region) { return *this = *this - region; }

  friend Region intersection(const Region &region1, const Region &region2) {
    return region1 & region2;
  }
  friend Region setunion(const Region &region1, const Region &region2) {
    return region1 | region2;
  }
  friend Region symmetric_difference(const Region &region1,
                                     const Region &region2) {
    return region1 ^ region2;
  }
  friend Region difference(const Region &region1, const Region &region2) {
    return region1 - region2;
  }

  // Set comparison operators
  bool contains(const Point<T, D> &p) const { return !isdisjoint(Region(p)); }
  friend bool isdisjoint(const Region &region1, const Region &region2) {
    return (region1 & region2).empty();
  }

  // Comparison operators
  friend bool operator<=(const Region &region1, const Region &region2) {
    return (region1 - region2).empty();
  }
  friend bool operator>=(const Region &region1, const Region &region2) {
    return region2 <= region1;
  }
  friend bool operator<(const Region &region1, const Region &region2) {
    return region1 != region2 && region1 <= region2;
  }
  friend bool operator>(const Region &region1, const Region &region2) {
    return region2 < region1;
  }

  bool issubset(const Region &region) const { return *this <= region; }
  bool issuperset(const Region &region) const { return *this >= region; }
  bool is_strict_subset(const Region &region) const { return *this < region; }
  bool is_strict_superset(const Region &region) const { return *this > region; }

  friend std::ostream &operator<<(std::ostream &os, const Region &region) {
    os << "{";
    const std::vector<Box<T, D>> boxes(region);
    for (std::size_t i = 0; i < boxes.size(); ++i) {
      if (i > 0)
        os << ",";
      os << boxes[i];
    }
    os << "}";
    return os;
  }
};

} // namespace Regions
} // namespace openPMD

namespace std {

template <typename T, std::size_t D>
struct equal_to<openPMD::Regions::Region<T, D>>;

template <typename T> struct equal_to<openPMD::Regions::Region<T, 0>> {
  constexpr bool operator()(const openPMD::Regions::Region<T, 0> &x,
                            const openPMD::Regions::Region<T, 0> &y) const {
    return x.is_full == y.is_full;
  }
};

template <typename T, std::size_t D>
struct equal_to<openPMD::Regions::Region<T, D>> {
  constexpr bool operator()(const openPMD::Regions::Region<T, D> &x,
                            const openPMD::Regions::Region<T, D> &y) const {
    return openPMD::Regions::helpers::vector_eq(x.subregions, y.subregions);
  }
};

template <typename T, std::size_t D>
struct less<openPMD::Regions::Region<T, D>>;

template <typename T> struct less<openPMD::Regions::Region<T, 0>> {
  constexpr bool operator()(const openPMD::Regions::Region<T, 0> &x,
                            const openPMD::Regions::Region<T, 0> &y) const {
    return x.is_full < y.is_full;
  }
};

template <typename T, std::size_t D>
struct less<openPMD::Regions::Region<T, D>> {
  constexpr bool operator()(const openPMD::Regions::Region<T, D> &x,
                            const openPMD::Regions::Region<T, D> &y) const {
    return openPMD::Regions::helpers::vector_lt(x.subregions, y.subregions);
  }
};

} // namespace std

#endif // #ifndef REGIONS_REGION_HPP
