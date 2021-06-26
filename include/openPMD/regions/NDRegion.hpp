#ifndef REGIONS_NDREGION_HPP
#define REGIONS_NDREGION_HPP

#include "Helpers.hpp"
#include "NDBox.hpp"
#include "NDPoint.hpp"
#include "Region.hpp"

namespace openPMD {
namespace Regions {

template <typename T> class NDRegion;

namespace detail {

// Abstract base helper class

template <typename T> class VRegion {
public:
  typedef T value_type;
  typedef typename VPoint<T>::size_type size_type;

  virtual std::unique_ptr<VRegion> copy() const = 0;

  virtual operator std::vector<NDBox<T>>() const = 0;

  virtual ~VRegion() {}

  virtual size_type ndims() const = 0;
  virtual bool empty() const = 0;

  virtual size_type size() const = 0;

  virtual size_type nboxes() const = 0;

  virtual std::unique_ptr<VRegion> operator>>(const NDPoint<T> &d) const = 0;
  virtual std::unique_ptr<VRegion> operator<<(const NDPoint<T> &d) const = 0;
  virtual VRegion &operator>>=(const NDPoint<T> &d) = 0;
  virtual VRegion &operator<<=(const NDPoint<T> &d) = 0;
  virtual std::unique_ptr<VRegion> operator*(const NDPoint<T> &d) const = 0;
  virtual VRegion &operator*=(const NDPoint<T> &d) = 0;
  virtual std::unique_ptr<VRegion> grown(const NDPoint<T> &dlo,
                                         const NDPoint<T> &dup) const = 0;
  virtual std::unique_ptr<VRegion> grown(const NDPoint<T> &d) const = 0;
  virtual std::unique_ptr<VRegion> grown(const T &n) const = 0;
  virtual std::unique_ptr<VRegion> shrunk(const NDPoint<T> &dlo,
                                          const NDPoint<T> &dup) const = 0;
  virtual std::unique_ptr<VRegion> shrunk(const NDPoint<T> &d) const = 0;
  virtual std::unique_ptr<VRegion> shrunk(const T &n) const = 0;

  virtual bool operator==(const VRegion &region2) const = 0;
  virtual bool operator!=(const VRegion &region2) const = 0;
  virtual bool operator==(const NDBox<T> &box2) const = 0;
  virtual bool operator!=(const NDBox<T> &box2) const = 0;

  virtual NDBox<T> bounding_box1() const = 0;

  virtual std::unique_ptr<VRegion> operator&(const VRegion &region2) const = 0;
  virtual std::unique_ptr<VRegion> operator|(const VRegion &region2) const = 0;
  virtual std::unique_ptr<VRegion> operator^(const VRegion &region2) const = 0;
  virtual std::unique_ptr<VRegion> operator-(const VRegion &region2) const = 0;

  virtual VRegion &operator&=(const VRegion &region) = 0;
  virtual VRegion &operator|=(const VRegion &region) = 0;
  virtual VRegion &operator^=(const VRegion &region) = 0;
  virtual VRegion &operator-=(const VRegion &region) = 0;

  virtual std::unique_ptr<VRegion>
  intersection1(const VRegion &region2) const = 0;
  virtual std::unique_ptr<VRegion> setunion1(const VRegion &region2) const = 0;
  virtual std::unique_ptr<VRegion>
  symmetric_difference1(const VRegion &region2) const = 0;
  virtual std::unique_ptr<VRegion>
  difference1(const VRegion &region2) const = 0;

  virtual bool contains(const NDPoint<T> &p) const = 0;
  virtual bool isdisjoint1(const VRegion &region2) const = 0;

  virtual bool operator<=(const VRegion &region2) const = 0;
  virtual bool operator>=(const VRegion &region2) const = 0;
  virtual bool operator<(const VRegion &region2) const = 0;
  virtual bool operator>(const VRegion &region2) const = 0;

  virtual bool is_subset_of(const VRegion &region) const = 0;
  virtual bool is_superset_of(const VRegion &region) const = 0;
  virtual bool is_strict_subset_of(const VRegion &region) const = 0;
  virtual bool is_strict_superset_of(const VRegion &region) const = 0;

  virtual bool equal_to1(const VRegion &x) const = 0;
  virtual bool less1(const VRegion &x) const = 0;

  virtual std::ostream &output(std::ostream &os) const = 0;
};

// Helper class wrapping Region<T,D>

template <typename T, std::size_t D> class WRegion final : public VRegion<T> {
  Region<T, D> r;

public:
  using typename VRegion<T>::value_type;
  using typename VRegion<T>::size_type;

  WRegion() : r{} {}

  WRegion(const WRegion &x) = default;
  WRegion(WRegion &&) = default;
  WRegion &operator=(const WRegion &) = default;
  WRegion &operator=(WRegion &&) = default;

  WRegion(const Region<T, D> &r_) : r(r_) {}
  WRegion(Region<T, D> &&r_) : r(std::move(r_)) {}
  operator Region<T, D>() const { return r; }

  WRegion(const Point<T, D> &p) : r(p) {}
  WRegion(const Box<T, D> &b) : r(b) {}
  WRegion(const std::vector<Box<T, D>> &boxes) : r(boxes) {}

  WRegion(const NDPoint<T> &p) : r(Point<T, D>(p)) {}
  WRegion(const NDBox<T> &b) : r(Box<T, D>(b)) {}
  WRegion(const std::vector<NDBox<T>> &boxes)
      : r(helpers::convert_vector<Box<T, D>>(boxes)) {}

  operator std::vector<Box<T, D>>() const { return std::vector<Box<T, D>>(r); }
  operator std::vector<NDBox<T>>() const override {
    return helpers::convert_vector<NDBox<T>>(std::vector<Box<T, D>>(r));
  }

  std::unique_ptr<VRegion<T>> copy() const override {
    return std::make_unique<WRegion>(*this);
  }

  ~WRegion() override {}

  size_type ndims() const override { return r.ndims(); }
  bool empty() const override { return r.empty(); }

  size_type size() const override { return r.size(); }

  size_type nboxes() const override { return r.nboxes(); }

  std::unique_ptr<VRegion<T>> operator>>(const NDPoint<T> &d) const override {
    return std::make_unique<WRegion>(r >> Point<T, D>(d));
  }
  std::unique_ptr<VRegion<T>> operator<<(const NDPoint<T> &d) const override {
    return std::make_unique<WRegion>(r << Point<T, D>(d));
  }
  VRegion<T> &operator>>=(const NDPoint<T> &d) override {
    r >>= Point<T, D>(d);
    return *this;
  }
  VRegion<T> &operator<<=(const NDPoint<T> &d) override {
    r <<= Point<T, D>(d);
    return *this;
  }
  std::unique_ptr<VRegion<T>> operator*(const NDPoint<T> &d) const override {
    return std::make_unique<WRegion>(r * Point<T, D>(d));
  }
  VRegion<T> &operator*=(const NDPoint<T> &d) override {
    r *= Point<T, D>(d);
    return *this;
  }
  std::unique_ptr<VRegion<T>> grown(const NDPoint<T> &dlo,
                                    const NDPoint<T> &dup) const override {
    return std::make_unique<WRegion>(
        r.grown(Point<T, D>(dlo), Point<T, D>(dup)));
  }
  std::unique_ptr<VRegion<T>> grown(const NDPoint<T> &d) const override {
    return std::make_unique<WRegion>(r.grown(Point<T, D>(d)));
  }
  std::unique_ptr<VRegion<T>> grown(const T &n) const override {
    return std::make_unique<WRegion>(r.grown(n));
  }
  std::unique_ptr<VRegion<T>> shrunk(const NDPoint<T> &dlo,
                                     const NDPoint<T> &dup) const override {
    return std::make_unique<WRegion>(
        r.shrunk(Point<T, D>(dlo), Point<T, D>(dup)));
  }
  std::unique_ptr<VRegion<T>> shrunk(const NDPoint<T> &d) const override {
    return std::make_unique<WRegion>(r.shrunk(Point<T, D>(d)));
  }
  std::unique_ptr<VRegion<T>> shrunk(const T &n) const override {
    return std::make_unique<WRegion>(r.shrunk(n));
  }

  bool operator==(const VRegion<T> &region2) const override {
    return r == dynamic_cast<const WRegion &>(region2);
  }
  bool operator!=(const VRegion<T> &region2) const override {
    return r != dynamic_cast<const WRegion &>(region2);
  }
  bool operator==(const NDBox<T> &box2) const override {
    return r == Box<T, D>(box2);
  }
  bool operator!=(const NDBox<T> &box2) const override {
    return r != Box<T, D>(box2);
  }

  NDBox<T> bounding_box1() const override { return NDBox<T>(bounding_box(r)); }

  std::unique_ptr<VRegion<T>>
  operator&(const VRegion<T> &region2) const override {
    return std::make_unique<WRegion>(r &
                                     dynamic_cast<const WRegion &>(region2));
  }
  std::unique_ptr<VRegion<T>>
  operator|(const VRegion<T> &region2) const override {
    return std::make_unique<WRegion>(r |
                                     dynamic_cast<const WRegion &>(region2));
  }
  std::unique_ptr<VRegion<T>>
  operator^(const VRegion<T> &region2) const override {
    return std::make_unique<WRegion>(r ^
                                     dynamic_cast<const WRegion &>(region2));
  }
  std::unique_ptr<VRegion<T>>
  operator-(const VRegion<T> &region2) const override {
    return std::make_unique<WRegion>(r -
                                     dynamic_cast<const WRegion &>(region2));
  }

  VRegion<T> &operator&=(const VRegion<T> &region) override {
    r &= dynamic_cast<const WRegion &>(region);
    return *this;
  }
  VRegion<T> &operator|=(const VRegion<T> &region) override {
    r |= dynamic_cast<const WRegion &>(region);
    return *this;
  }
  VRegion<T> &operator^=(const VRegion<T> &region) override {
    r ^= dynamic_cast<const WRegion &>(region);
    return *this;
  }
  VRegion<T> &operator-=(const VRegion<T> &region) override {
    r -= dynamic_cast<const WRegion &>(region);
    return *this;
  }

  std::unique_ptr<VRegion<T>>
  intersection1(const VRegion<T> &region2) const override {
    return std::make_unique<WRegion>(
        intersection(r, dynamic_cast<const WRegion &>(region2)));
  }
  std::unique_ptr<VRegion<T>>
  setunion1(const VRegion<T> &region2) const override {
    return std::make_unique<WRegion>(
        setunion(r, dynamic_cast<const WRegion &>(region2)));
  }
  std::unique_ptr<VRegion<T>>
  symmetric_difference1(const VRegion<T> &region2) const override {
    return std::make_unique<WRegion>(
        symmetric_difference(r, dynamic_cast<const WRegion &>(region2)));
  }
  std::unique_ptr<VRegion<T>>
  difference1(const VRegion<T> &region2) const override {
    return std::make_unique<WRegion>(
        difference(r, dynamic_cast<const WRegion &>(region2)));
  }

  bool contains(const NDPoint<T> &p) const override {
    return r.contains(Point<T, D>(p));
  }
  bool isdisjoint1(const VRegion<T> &region2) const override {
    return isdisjoint(r, dynamic_cast<const WRegion &>(region2));
  }

  bool operator<=(const VRegion<T> &region2) const override {
    return r <= dynamic_cast<const WRegion &>(region2);
  }
  bool operator>=(const VRegion<T> &region2) const override {
    return r >= dynamic_cast<const WRegion &>(region2);
  }
  bool operator<(const VRegion<T> &region2) const override {
    return r < dynamic_cast<const WRegion &>(region2);
  }
  bool operator>(const VRegion<T> &region2) const override {
    return r > dynamic_cast<const WRegion &>(region2);
  }

  bool is_subset_of(const VRegion<T> &region) const override {
    return r.is_subset_of(dynamic_cast<const WRegion &>(region));
  }
  bool is_superset_of(const VRegion<T> &region) const override {
    return r.is_superset_of(dynamic_cast<const WRegion &>(region));
  }
  bool is_strict_subset_of(const VRegion<T> &region) const override {
    return r.is_strict_subset_of(dynamic_cast<const WRegion &>(region));
  }
  bool is_strict_superset_of(const VRegion<T> &region) const override {
    return r.is_strict_superset_of(dynamic_cast<const WRegion &>(region));
  }

  bool equal_to1(const VRegion<T> &x) const override {
    return std::equal_to<Region<T, D>>()(r, dynamic_cast<const WRegion &>(x).r);
  }
  bool less1(const VRegion<T> &x) const override {
    return std::less<Region<T, D>>()(r, dynamic_cast<const WRegion &>(x).r);
  }

  std::ostream &output(std::ostream &os) const override { return os << r; }
};

template <typename T, typename... Args>
std::unique_ptr<VRegion<T>> make_VRegion(const std::size_t D,
                                         const Args &...args) {
  switch (D) {
  case 0:
    return std::make_unique<WRegion<T, 0>>(args...);
  case 1:
    return std::make_unique<WRegion<T, 1>>(args...);
  case 2:
    return std::make_unique<WRegion<T, 2>>(args...);
  case 3:
    return std::make_unique<WRegion<T, 3>>(args...);
  case 4:
    return std::make_unique<WRegion<T, 4>>(args...);
  case 5:
    return std::make_unique<WRegion<T, 5>>(args...);
  default:
    abort();
  }
}

} // namespace detail

/** A Region
 *
 * The dimension (number of component) of the Region is only known at
 * run-time. @see Region
 *
 * A region is described by two points, its lower bound (inclusive) and
 * upper bound (exclusive). If the lower bound not less than the upper
 * bound, the region is empty. Empty regiones are fine (similar to an empty
 * array).
 */
template <typename T> class NDRegion {
  template <typename U> using VRegion = detail::VRegion<U>;

  template <typename U> friend class NDRegion;
  friend struct std::equal_to<NDRegion>;
  friend struct std::less<NDRegion>;

  std::unique_ptr<VRegion<T>> r;

  NDRegion(std::unique_ptr<VRegion<T>> r_) : r(std::move(r_)) {}

public:
  /** Component type
   */
  typedef typename VRegion<T>::value_type value_type;
  /** Return type of Region::size()
   */
  typedef typename VRegion<T>::size_type size_type;

  /** Create an invalid Region
   */
  NDRegion() : r() {}
  /** Create a value-initialized Region with D components
   */
  NDRegion(const size_type D) : r(detail::make_VRegion<T>(D)) {}

  NDRegion(const NDRegion &x) : r(x.r ? x.r->copy() : nullptr) {}
  NDRegion(NDRegion &&) = default;
  NDRegion &operator=(const NDRegion &x) {
    r = x.r ? x.r->copy() : nullptr;
    return *this;
  }
  NDRegion &operator=(NDRegion &&) = default;

  template <std::size_t D>
  NDRegion(const Region<T, D> &r_)
      : r(std::make_unique<detail::WRegion<T, D>>(r_)) {}
  template <std::size_t D>
  NDRegion(Region<T, D> &&r_)
      : r(std::make_unique<detail::WRegion<T, D>>(std::move(r_))) {}
  template <std::size_t D> operator Region<T, D>() const {
    return Region<T, D>(dynamic_cast<const detail::WRegion<T, D> &>(*r));
  }

  template <std::size_t D>
  NDRegion(const Point<T, D> &p)
      : r(std::make_unique<VRegion<T>>(detail::WRegion<T, D>(p))) {}
  template <std::size_t D>
  NDRegion(const Box<T, D> &b)
      : r(std::make_unique<VRegion<T>>(detail::WRegion<T, D>(b))) {}
  template <std::size_t D>
  NDRegion(const std::vector<Box<T, D>> &boxes)
      : r(std::make_unique<VRegion<T>>(detail::WRegion<T, D>(boxes))) {}
  template <std::size_t D> operator std::vector<Box<T, D>>() const {
    return std::vector<Box<T, D>>(
        dynamic_cast<const detail::WRegion<T, D> &>(*r));
  }

  NDRegion(const NDPoint<T> &p) : r(detail::make_VRegion<T>(p.ndims(), p)) {}
  NDRegion(const NDBox<T> &b) : r(detail::make_VRegion<T>(b.ndims(), b)) {}
  NDRegion(const size_type D, const std::vector<NDBox<T>> &boxes)
      : r(detail::make_VRegion<T>(D, boxes)) {}
  operator std::vector<NDBox<T>>() const { return std::vector<NDBox<T>>(*r); }

  /** Check whether a Region is valid
   *
   * A valid Region knows its number of dimensions, and its components
   * are initialized. An invalid Region does not know its number of
   * dimensions and holds no data, similar to a null pointer.
   *
   * Most other member functions must not be called for invalid
   * Regions.
   */
  bool has_value() const { return bool(r); }

  size_type ndims() const { return r->ndims(); }
  bool empty() const { return r->empty(); }

  size_type size() const { return r->size(); }

  size_type nboxes() const { return r->nboxes(); }

  NDRegion operator>>(const NDPoint<T> &d) const { return NDRegion(*r >> d); }
  NDRegion operator<<(const NDPoint<T> &d) const { return NDRegion(*r << d); }
  NDRegion &operator>>=(const NDPoint<T> &d) {
    *r >>= d;
    return *this;
  }
  NDRegion &operator<<=(const NDPoint<T> &d) {
    *r <<= d;
    return *this;
  }
  NDRegion operator*(const NDPoint<T> &d) const { return NDRegion(*r * d); }
  NDRegion &operator*=(const NDPoint<T> &d) {
    *r *= d;
    return *this;
  }

  NDRegion grown(const NDPoint<T> &dlo, const NDPoint<T> &dup) const {
    return NDRegion(r->grown(dlo, dup));
  }
  NDRegion grown(const NDPoint<T> &d) const { return NDRegion(r->grown(d)); }
  NDRegion grown(const T &n) const { return NDRegion(r->grown(n)); }
  NDRegion shrunk(const NDPoint<T> &dlo, const NDPoint<T> &dup) const {
    return NDRegion(r->shrunk(dlo, dup));
  }
  NDRegion shrunk(const NDPoint<T> &d) const { return NDRegion(r->shrunk(d)); }
  NDRegion shrunk(const T &n) const { return NDRegion(r->shrunk(n)); }

  friend bool operator==(const NDRegion &region1, const NDRegion &region2) {
    return *region1.r == *region2.r;
  }
  friend bool operator!=(const NDRegion &region1, const NDRegion &region2) {
    return *region1.r != *region2.r;
  }
  friend bool operator==(const NDRegion &region1, const NDBox<T> &box2) {
    return *region1.r == box2;
  }
  friend bool operator!=(const NDRegion &region1, const NDBox<T> &box2) {
    return *region1.r != box2;
  }
  friend bool operator==(const NDBox<T> &box1, const NDRegion &region2) {
    return *region2.r == box1;
  }
  friend bool operator!=(const NDBox<T> &box1, const NDRegion &region2) {
    return *region2.r != box1;
  }

  friend NDBox<T> bounding_box(const NDRegion &region) {
    return region.r->bounding_box1();
  }

  friend NDRegion operator&(const NDRegion &region1, const NDRegion &region2) {
    return NDRegion(*region1.r & *region2.r);
  }
  friend NDRegion operator|(const NDRegion &region1, const NDRegion &region2) {
    return NDRegion(*region1.r | *region2.r);
  }
  friend NDRegion operator^(const NDRegion &region1, const NDRegion &region2) {
    return NDRegion(*region1.r ^ *region2.r);
  }
  friend NDRegion operator-(const NDRegion &region1, const NDRegion &region2) {
    return NDRegion(*region1.r - *region2.r);
  }

  NDRegion &operator&=(const NDRegion &region) {
    *r &= *region.r;
    return *this;
  }
  NDRegion &operator|=(const NDRegion &region) {
    *r |= *region.r;
    return *this;
  }
  NDRegion &operator^=(const NDRegion &region) {
    *r ^= *region.r;
    return *this;
  }
  NDRegion &operator-=(const NDRegion &region) {
    *r -= *region.r;
    return *this;
  }

  friend NDRegion intersection(const NDRegion &region1,
                               const NDRegion &region2) {
    return NDRegion(intersection(*region1.r, *region2.r));
  }
  friend NDRegion setunion(const NDRegion &region1, const NDRegion &region2) {
    return NDRegion(setunion(*region1.r, *region2.r));
  }
  friend NDRegion symmetric_difference(const NDRegion &region1,
                                       const NDRegion &region2) {
    return NDRegion(symmetric_difference(*region1.r, *region2.r));
  }
  friend NDRegion difference(const NDRegion &region1, const NDRegion &region2) {
    return NDRegion(difference(*region1.r, *region2.r));
  }

  bool contains(const NDPoint<T> &p) const { return r->contains(p); }
  friend bool isdisjoint(const NDRegion &region1, const NDRegion &region2) {
    return region1.r->isdisjoint1(*region2.r);
  }

  friend bool operator<=(const NDRegion &region1, const NDRegion &region2) {
    return *region1.r <= *region2.r;
  }
  friend bool operator>=(const NDRegion &region1, const NDRegion &region2) {
    return *region1.r >= *region2.r;
  }
  friend bool operator<(const NDRegion &region1, const NDRegion &region2) {
    return *region1.r < *region2.r;
  }
  friend bool operator>(const NDRegion &region1, const NDRegion &region2) {
    return *region1.r > *region2.r;
  }

  bool is_subset_of(const NDRegion &region) const {
    return r->is_subset_of(*region.r);
  }
  bool is_superset_of(const NDRegion &region) const {
    return r->is_superset_of(*region.r);
  }
  bool is_strict_subset_of(const NDRegion &region) const {
    return r->is_strict_subset_of(*region.r);
  }
  bool is_strict_superset_of(const NDRegion &region) const {
    return r->is_strict_superset_of(*region.r);
  }

  /** Output a region
   */
  friend std::ostream &operator<<(std::ostream &os, const NDRegion &x) {
    if (x.r)
      x.r->output(os);
    else
      os << "{INVALID}";
    return os;
  }
};

} // namespace Regions
} // namespace openPMD

namespace std {

template <typename T> struct equal_to<openPMD::Regions::NDRegion<T>> {
  constexpr bool operator()(const openPMD::Regions::NDRegion<T> &x,
                            const openPMD::Regions::NDRegion<T> &y) const {
    if (!x.has_value() && !y.has_value())
      return true;
    if (!x.has_value() || !y.has_value())
      return false;
    return x.r->equal_to1(*y.r);
  }
};

template <typename T> struct less<openPMD::Regions::NDRegion<T>> {
  constexpr bool operator()(const openPMD::Regions::NDRegion<T> &x,
                            const openPMD::Regions::NDRegion<T> &y) const {
    if (!x.has_value())
      return true;
    if (!y.has_value())
      return false;
    return x.r->less1(*y.r);
  }
};

} // namespace std

#endif // #ifndef REGIONS_NDREGION_HPP
