#ifndef REGIONS_NDBOX_HPP
#define REGIONS_NDBOX_HPP

#include "Box.hpp"
#include "Helpers.hpp"
#include "NDPoint.hpp"

namespace openPMD {
namespace Regions {

template <typename T> class NDBox;

namespace detail {

// Abstract base helper class

template <typename T> class VBox {
public:
  typedef T value_type;
  typedef typename VPoint<T>::size_type size_type;

  virtual std::unique_ptr<VBox> copy() const = 0;

  virtual ~VBox() {}

  virtual size_type ndims() const = 0;
  virtual bool empty() const = 0;
  virtual NDPoint<T> lower() const = 0;
  virtual NDPoint<T> upper() const = 0;
  virtual NDPoint<T> shape() const = 0;
  virtual size_type size() const = 0;

  virtual VBox &operator>>=(const NDPoint<T> &p) = 0;
  virtual VBox &operator<<=(const NDPoint<T> &p) = 0;
  virtual VBox &operator*=(const NDPoint<T> &p) = 0;
  virtual std::unique_ptr<VBox> operator>>(const NDPoint<T> &p) const = 0;
  virtual std::unique_ptr<VBox> operator<<(const NDPoint<T> &p) const = 0;
  virtual std::unique_ptr<VBox> operator*(const NDPoint<T> &p) const = 0;
  virtual std::unique_ptr<VBox> grown(const NDPoint<T> &dlo,
                                      const NDPoint<T> &dup) const = 0;
  virtual std::unique_ptr<VBox> grown(const NDPoint<T> &d) const = 0;
  virtual std::unique_ptr<VBox> grown(const T &d) const = 0;
  virtual std::unique_ptr<VBox> shrunk(const NDPoint<T> &dlo,
                                       const NDPoint<T> &dup) const = 0;
  virtual std::unique_ptr<VBox> shrunk(const NDPoint<T> &d) const = 0;
  virtual std::unique_ptr<VBox> shrunk(const T &d) const = 0;

  virtual bool operator==(const VBox &b2) const = 0;
  virtual bool operator!=(const VBox &b2) const = 0;

  virtual bool contains(const NDPoint<T> &p) const = 0;
  virtual bool isdisjoint1(const VBox &b2) const = 0;
  virtual bool operator<=(const VBox &b2) const = 0;
  virtual bool operator>=(const VBox &b2) const = 0;
  virtual bool operator<(const VBox &b2) const = 0;
  virtual bool operator>(const VBox &b2) const = 0;
  virtual bool is_subset_of(const VBox &b) const = 0;
  virtual bool is_superset_of(const VBox &b) const = 0;
  virtual bool is_strict_subset_of(const VBox &b) const = 0;
  virtual bool is_strict_superset_of(const VBox &b) const = 0;

  virtual std::unique_ptr<VBox> bounding_box1(const VBox &b2) const = 0;

  virtual std::unique_ptr<VBox> operator&(const VBox &b2) const = 0;
  virtual VBox &operator&=(const VBox &b) = 0;
  virtual std::unique_ptr<VBox> intersection1(const VBox &b2) const = 0;

  virtual bool operator==(const std::vector<NDBox<T>> &bs) const = 0;
  virtual bool operator!=(const std::vector<NDBox<T>> &bs) const = 0;

  virtual std::vector<NDBox<T>> operator-(const VBox &b2) const = 0;
  virtual std::vector<NDBox<T>> difference1(const VBox &b2) const = 0;

  virtual std::vector<NDBox<T>> operator|(const VBox &b2) const = 0;
  virtual std::vector<NDBox<T>> setunion1(const VBox &b2) const = 0;

  virtual std::vector<NDBox<T>> operator^(const VBox &b2) const = 0;
  virtual std::vector<NDBox<T>> symmetric_difference1(const VBox &b2) const = 0;

  virtual bool equal_to1(const VBox &x) const = 0;
  virtual std::size_t hash1() const = 0;
  virtual bool less1(const VBox &x) const = 0;

  virtual std::ostream &output(std::ostream &os) const = 0;
};

// Helper class wrapping Box<T,D>

template <typename T, std::size_t D> class WBox final : public VBox<T> {
  Box<T, D> b;

public:
  using typename VBox<T>::value_type;
  using typename VBox<T>::size_type;

  WBox() : b{} {}

  WBox(const WBox &x) = default;
  WBox(WBox &&) = default;
  WBox &operator=(const WBox &) = default;
  WBox &operator=(WBox &&) = default;

  WBox(const Box<T, D> &b_) : b(b_) {}
  WBox(Box<T, D> &&b_) : b(std::move(b_)) {}
  operator Box<T, D>() const { return b; }

  WBox(const Point<T, D> &lo_, const Point<T, D> &hi_) : b(lo_, hi_) {}
  explicit WBox(const Point<T, D> &p) : b(p) {}

  std::unique_ptr<VBox<T>> copy() const override {
    return std::make_unique<WBox>(*this);
  }

  ~WBox() override {}

  size_type ndims() const override { return b.ndims(); }
  bool empty() const override { return b.empty(); }
  NDPoint<T> lower() const override { return b.lower(); }
  NDPoint<T> upper() const override { return b.upper(); }
  NDPoint<T> shape() const override { return b.shape(); }
  size_type size() const override { return b.size(); }

  VBox<T> &operator>>=(const NDPoint<T> &p) override {
    b >>= Point<T, D>(p);
    return *this;
  }
  VBox<T> &operator<<=(const NDPoint<T> &p) override {
    b <<= Point<T, D>(p);
    return *this;
  }
  VBox<T> &operator*=(const NDPoint<T> &p) override {
    b *= Point<T, D>(p);
    return *this;
  }
  std::unique_ptr<VBox<T>> operator>>(const NDPoint<T> &p) const override {
    return std::make_unique<WBox>(b >> Point<T, D>(p));
  }
  std::unique_ptr<VBox<T>> operator<<(const NDPoint<T> &p) const override {
    return std::make_unique<WBox>(b << Point<T, D>(p));
  }
  std::unique_ptr<VBox<T>> operator*(const NDPoint<T> &p) const override {
    return std::make_unique<WBox>(b * Point<T, D>(p));
  }
  std::unique_ptr<VBox<T>> grown(const NDPoint<T> &dlo,
                                 const NDPoint<T> &dup) const override {
    return std::make_unique<WBox>(b.grown(Point<T, D>(dlo), Point<T, D>(dup)));
  }
  std::unique_ptr<VBox<T>> grown(const NDPoint<T> &d) const override {
    return std::make_unique<WBox>(b.grown(Point<T, D>(d)));
  }
  std::unique_ptr<VBox<T>> grown(const T &d) const override {
    return std::make_unique<WBox>(b.grown(d));
  }
  std::unique_ptr<VBox<T>> shrunk(const NDPoint<T> &dlo,
                                  const NDPoint<T> &dup) const override {
    return std::make_unique<WBox>(b.shrunk(Point<T, D>(dlo), Point<T, D>(dup)));
  }
  std::unique_ptr<VBox<T>> shrunk(const NDPoint<T> &d) const override {
    return std::make_unique<WBox>(b.shrunk(Point<T, D>(d)));
  }
  std::unique_ptr<VBox<T>> shrunk(const T &d) const override {
    return std::make_unique<WBox>(b.shrunk(d));
  }

  bool operator==(const VBox<T> &b2) const override {
    return b == dynamic_cast<const WBox &>(b2).b;
  }
  bool operator!=(const VBox<T> &b2) const override {
    return b != dynamic_cast<const WBox &>(b2).b;
  }

  bool contains(const NDPoint<T> &p) const override {
    return b.contains(Point<T, D>(p));
  }
  bool isdisjoint1(const VBox<T> &b2) const override {
    return isdisjoint(b, dynamic_cast<const WBox &>(b2).b);
  }
  bool operator<=(const VBox<T> &b2) const override {
    return b <= dynamic_cast<const WBox &>(b2).b;
  }
  bool operator>=(const VBox<T> &b2) const override {
    return b >= dynamic_cast<const WBox &>(b2).b;
  }
  bool operator<(const VBox<T> &b2) const override {
    return b < dynamic_cast<const WBox &>(b2).b;
  }
  bool operator>(const VBox<T> &b2) const override {
    return b > dynamic_cast<const WBox &>(b2).b;
  }
  bool is_subset_of(const VBox<T> &b2) const override {
    return b.is_subset_of(dynamic_cast<const WBox &>(b2).b);
  }
  bool is_superset_of(const VBox<T> &b2) const override {
    return b.is_superset_of(dynamic_cast<const WBox &>(b2).b);
  }
  bool is_strict_subset_of(const VBox<T> &b2) const override {
    return b.is_strict_subset_of(dynamic_cast<const WBox &>(b2).b);
  }
  bool is_strict_superset_of(const VBox<T> &b2) const override {
    return b.is_strict_superset_of(dynamic_cast<const WBox &>(b2).b);
  }

  std::unique_ptr<VBox<T>> bounding_box1(const VBox<T> &b2) const override {
    return std::make_unique<WBox>(
        bounding_box(b, dynamic_cast<const WBox &>(b2).b));
  }

  std::unique_ptr<VBox<T>> operator&(const VBox<T> &b2) const override {
    return std::make_unique<WBox>(b & dynamic_cast<const WBox &>(b2).b);
  }
  VBox<T> &operator&=(const VBox<T> &b2) override {
    b &= dynamic_cast<const WBox &>(b2).b;
    return *this;
  }
  std::unique_ptr<VBox<T>> intersection1(const VBox<T> &b2) const override {
    return std::make_unique<WBox>(
        intersection(b, dynamic_cast<const WBox &>(b2).b));
  }

  bool operator==(const std::vector<NDBox<T>> &bs) const override {
    return b == helpers::convert_vector<Box<T, D>>(bs);
  }
  bool operator!=(const std::vector<NDBox<T>> &bs) const override {
    return b != helpers::convert_vector<Box<T, D>>(bs);
  }

  std::vector<NDBox<T>> operator-(const VBox<T> &b2) const override {
    return helpers::convert_vector<NDBox<T>>(b -
                                             dynamic_cast<const WBox &>(b2).b);
  }
  std::vector<NDBox<T>> difference1(const VBox<T> &b2) const override {
    return helpers::convert_vector<NDBox<T>>(
        difference(b, dynamic_cast<const WBox &>(b2).b));
  }

  std::vector<NDBox<T>> operator|(const VBox<T> &b2) const override {
    return helpers::convert_vector<NDBox<T>>(b |
                                             dynamic_cast<const WBox &>(b2).b);
  }
  std::vector<NDBox<T>> setunion1(const VBox<T> &b2) const override {
    return helpers::convert_vector<NDBox<T>>(
        setunion(b, dynamic_cast<const WBox &>(b2).b));
  }

  std::vector<NDBox<T>> operator^(const VBox<T> &b2) const override {
    return helpers::convert_vector<NDBox<T>>(b ^
                                             dynamic_cast<const WBox &>(b2).b);
  }
  std::vector<NDBox<T>>
  symmetric_difference1(const VBox<T> &b2) const override {
    return helpers::convert_vector<NDBox<T>>(
        symmetric_difference(b, dynamic_cast<const WBox &>(b2).b));
  }

  bool equal_to1(const VBox<T> &x) const override {
    return std::equal_to<Box<T, D>>()(b, dynamic_cast<const WBox &>(x).b);
  }
  std::size_t hash1() const override { return std::hash<Box<T, D>>()(b); }
  bool less1(const VBox<T> &x) const override {
    return std::less<Box<T, D>>()(b, dynamic_cast<const WBox &>(x).b);
  }

  std::ostream &output(std::ostream &os) const override { return os << b; }
};

template <typename T, typename... Args>
std::unique_ptr<VBox<T>> make_VBox(const std::size_t D, const Args &...args) {
  switch (D) {
  case 0:
    return std::make_unique<WBox<T, 0>>(args...);
  case 1:
    return std::make_unique<WBox<T, 1>>(args...);
  case 2:
    return std::make_unique<WBox<T, 2>>(args...);
  case 3:
    return std::make_unique<WBox<T, 3>>(args...);
  case 4:
    return std::make_unique<WBox<T, 4>>(args...);
  case 5:
    return std::make_unique<WBox<T, 5>>(args...);
  default:
    abort();
  }
}

} // namespace detail

/** A Box
 *
 * The dimension (number of component) of the Box is only known at
 * run-time. @see Box
 *
 * A box is described by two points, its lower bound (inclusive) and
 * upper bound (exclusive). If the lower bound not less than the upper
 * bound, the box is empty. Empty boxes are fine (similar to an empty
 * array).
 */
template <typename T> class NDBox {
  template <typename U> using VBox = detail::VBox<U>;

  template <typename U> friend class NDBox;
  friend struct std::equal_to<NDBox>;
  friend struct std::hash<NDBox>;
  friend struct std::less<NDBox>;

  std::unique_ptr<VBox<T>> b;

  NDBox(std::unique_ptr<VBox<T>> b_) : b(std::move(b_)) {}

public:
  /** Component type
   */
  typedef typename VBox<T>::value_type value_type;
  /** Return type of Box::size()
   */
  typedef typename VBox<T>::size_type size_type;

  /** Create an invalid Box
   */
  NDBox() : b() {}
  /** Create a value-initialized Box with D components
   */
  NDBox(const size_type D) : b(detail::make_VBox<T>(D)) {}

  NDBox(const NDBox &x) : b(x.b ? x.b->copy() : nullptr) {}
  NDBox(NDBox &&) = default;
  NDBox &operator=(const NDBox &x) {
    b = x.b ? x.b->copy() : nullptr;
    return *this;
  }
  NDBox &operator=(NDBox &&) = default;

  template <std::size_t D>
  NDBox(const Box<T, D> &b_) : b(std::make_unique<detail::WBox<T, D>>(b_)) {}
  template <std::size_t D>
  NDBox(Box<T, D> &&b_)
      : b(std::make_unique<detail::WBox<T, D>>(std::move(b_))) {}
  template <std::size_t D> operator Box<T, D>() const {
    return Box<T, D>(dynamic_cast<const detail::WBox<T, D> &>(*b));
  }

  NDBox(const NDPoint<T> &lo_, const NDPoint<T> &hi_)
      : b(detail::make_VBox<T>(lo_.ndims(), lo_, hi_)) {}
  explicit NDBox(const NDPoint<T> &p) : b(detail::make_VBox<T>(p.ndims(), p)) {}

  /** Check whether a box is valid
   *
   * A valid box knows its number of dimensions, and its components
   * are initialized. An invalid box does not know its number of
   * dimensions and holds no data, similar to a null pointer.
   *
   * Most other member functions must not be called for invalid
   * boxes.
   */
  bool has_value() const { return bool(b); }

  size_type ndims() const { return b->ndims(); }
  bool empty() const { return b->empty(); }
  NDPoint<T> lower() const { return b->lower(); }
  NDPoint<T> upper() const { return b->upper(); }
  NDPoint<T> shape() const { return b->shape(); }
  size_type size() const { return b->size(); }

  NDBox &operator>>=(const NDPoint<T> &p) {
    *b >>= p->p;
    return *this;
  }
  NDBox &operator<<=(const NDPoint<T> &p) {
    *b <<= p->p;
    return *this;
  }
  NDBox &operator*=(const NDPoint<T> &p) {
    *b *= p->p;
    return *this;
  }
  NDBox operator>>(const NDPoint<T> &p) const { return NDBox(*b >> p); }
  NDBox operator<<(const NDPoint<T> &p) const { return NDBox(*b << p); }
  NDBox operator*(const NDPoint<T> &p) const { return NDBox(*b * p); }
  NDBox grown(const NDPoint<T> &dlo, const NDPoint<T> &dup) const {
    return NDBox(b->grown(dlo, dup));
  }
  NDBox grown(const NDPoint<T> &d) const { return NDBox(b->grown(d)); }
  NDBox grown(const T &d) const { return NDBox(b->grown(d)); }
  NDBox shrunk(const NDPoint<T> &dlo, const NDPoint<T> &dup) const {
    return NDBox(b->shrunk(dlo, dup));
  }
  NDBox shrunk(const NDPoint<T> &d) const { return NDBox(b->shrunk(d)); }
  NDBox shrunk(const T &d) const { return NDBox(b->shrunk(d)); }

  friend bool operator==(const NDBox &b1, const NDBox &b2) {
    return *b1.b == *b2.b;
  }
  friend bool operator!=(const NDBox &b1, const NDBox &b2) {
    return *b1.b != *b2.b;
  }

  bool contains(const NDPoint<T> &p) const { return b->contains(p); }
  friend bool isdisjoint(const NDBox &b1, const NDBox &b2) {
    return b1.b->isdisjoint1(*b2.b);
  }
  friend bool operator<=(const NDBox &b1, const NDBox &b2) {
    return *b1.b <= *b2.b;
  }
  friend bool operator>=(const NDBox &b1, const NDBox &b2) {
    return *b1.b >= *b2.b;
  }
  friend bool operator<(const NDBox &b1, const NDBox &b2) {
    return *b1.b < *b2.b;
  }
  friend bool operator>(const NDBox &b1, const NDBox &b2) {
    return *b1.b > *b2.b;
  }
  bool is_subset_of(const NDBox &b2) const { return b->is_subset_of(*b2.b); }
  bool is_superset_of(const NDBox &b2) const {
    return b->is_superset_of(*b2.b);
  }
  bool is_strict_subset_of(const NDBox &b2) const {
    return b->is_strict_subset_of(*b2.b);
  }
  bool is_strict_superset_of(const NDBox &b2) const {
    return b->is_strict_superset_of(*b2.b);
  }

  friend NDBox bounding_box(const NDBox &b1, const NDBox &b2) {
    return b1.b->bounding_box1(*b2.b);
  }

  friend NDBox operator&(const NDBox &b1, const NDBox &b2) {
    return *b1.b & *b2.b;
  }
  NDBox &operator&=(const NDBox &b2) {
    *b &= *b2.b;
    return *this;
  }
  friend NDBox intersection(const NDBox &b1, const NDBox &b2) {
    return intersection(*b1.b, *b2.b);
  }

  friend bool operator==(const NDBox &b1, const std::vector<NDBox> &bs) {
    return *b1.b == bs;
  }
  friend bool operator==(const std::vector<NDBox> &bs, const NDBox &b2) {
    return *b2.b == bs;
  }
  friend bool operator!=(const NDBox &b1, const std::vector<NDBox> &bs) {
    return *b1.b != bs;
  }
  friend bool operator!=(const std::vector<NDBox> &bs, const NDBox &b2) {
    return *b2.b != bs;
  }

  friend std::vector<NDBox> operator-(const NDBox &b1, const NDBox &b2) {

    return *b1.b - *b2.b;
  }
  friend std::vector<NDBox> difference(const NDBox &b1, const NDBox &b2) {
    return difference(*b1.b, *b2.b);
  }

  friend std::vector<NDBox> operator|(const NDBox &b1, const NDBox &b2) {
    return *b1.b | *b2.b;
  }
  friend std::vector<NDBox> setunion(const NDBox &b1, const NDBox &b2) {
    return setunion(*b1.b, *b2.b);
  }

  friend std::vector<NDBox> operator^(const NDBox &b1, const NDBox &b2) {
    return *b1.b ^ *b2.b;
  }
  friend std::vector<NDBox> symmetric_difference(const NDBox &b1,
                                                 const NDBox &b2) {
    return symmetric_difference(*b1.b, *b2.b);
  }

  /** Output a box
   */
  friend std::ostream &operator<<(std::ostream &os, const NDBox &x) {
    if (x.b)
      x.b->output(os);
    else
      os << "(INVALID)";
    return os;
  }
};

} // namespace Regions
} // namespace openPMD

namespace std {

template <typename T> struct equal_to<openPMD::Regions::NDBox<T>> {
  constexpr bool operator()(const openPMD::Regions::NDBox<T> &x,
                            const openPMD::Regions::NDBox<T> &y) const {
    if (!x.has_value() && !y.has_value())
      return true;
    if (!x.has_value() || !y.has_value())
      return false;
    return x.b->equal_to1(*y.b);
  }
};

template <typename T> struct hash<openPMD::Regions::NDBox<T>> {
  constexpr size_t operator()(const openPMD::Regions::NDBox<T> &x) const {
    if (!x.has_value())
      return size_t(0x7e21b87749864bbcULL);
    return x.b->hash1();
  }
};

template <typename T> struct less<openPMD::Regions::NDBox<T>> {
  constexpr bool operator()(const openPMD::Regions::NDBox<T> &x,
                            const openPMD::Regions::NDBox<T> &y) const {
    if (!x.has_value())
      return true;
    if (!y.has_value())
      return false;
    return x.b->less1(*y.b);
  }
};

} // namespace std

#endif // #ifndef REGIONS_NDBOX_HPP
