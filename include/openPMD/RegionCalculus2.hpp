#ifndef REGIONCALCULUS2_HPP
#define REGIONCALCULUS2_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

namespace RegionCalculus2 {

// A D-dimensional point

template <typename T, std::size_t D> class point {
  std::array<T, D> elts;

public:
  typedef T value_type;
  typedef std::size_t size_type;

  constexpr point() : elts{} {} // value initialization

  point(const point &) = default;
  point(point &&) = default;
  point &operator=(const point &) = default;
  point &operator=(point &&) = default;

  constexpr size_type size() const { return D; }

  constexpr const T &operator[](const size_type d) const { return elts[d]; }
  constexpr T &operator[](const size_type d) { return elts[d]; }

  friend constexpr point operator+(const point &x, const point &y) {
    point r;
    for (size_type d = 0; d < D; ++d)
      r.elts[d] = x.elts[d] + y.elts[d];
    return r;
  }
  constexpr point &operator+=(const point &x) { return *this = *this + x; }
};

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

  virtual vpoint &operator+=(const std::unique_ptr<vpoint> &x) = 0;
  virtual std::unique_ptr<vpoint>
  operator+(const std::unique_ptr<vpoint> &x) const = 0;
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

  std::unique_ptr<vpoint<T>> copy() const override {
    return std::make_unique<wpoint>(*this);
  }

  ~wpoint() override {}

  constexpr size_type size() const override { return p.size(); }

  const T &operator[](const size_type d) const override { return p[d]; }
  T &operator[](const size_type d) override { return p[d]; }

  vpoint<T> &operator+=(const std::unique_ptr<vpoint<T>> &x) override {
    p += dynamic_cast<const wpoint &>(*x).p;
    return *this;
  }
  std::unique_ptr<vpoint<T>>
  operator+(const std::unique_ptr<vpoint<T>> &x) const override {
    auto r = std::make_unique<wpoint>(*this);
    *r += x;
    return r;
  }
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

// A point

template <typename T> class ndpoint {
  template <typename U> using vpoint = detail::vpoint<U>;

  std::unique_ptr<vpoint<T>> p;

  ndpoint(std::unique_ptr<vpoint<T>> p) : p(std::move(p)) {}

public:
  typedef typename vpoint<T>::value_type value_type;
  typedef typename vpoint<T>::size_type size_type;

  ndpoint() : p() {}
  ndpoint(const size_type D) : p(detail::make_vpoint<T>(D)) {}

  ndpoint(const ndpoint &x) : p(x.p ? x.p->copy() : nullptr) {}
  ndpoint(ndpoint &&) = default;
  ndpoint &operator=(const ndpoint &) = default;
  ndpoint &operator=(ndpoint &&) = default;

  operator bool() const { return bool(p); }

  size_type size() const { return p->size(); }

  const T &operator[](const size_type d) const { return (*p)[d]; }
  T &operator[](const size_type d) { return (*p)[d]; }

  ndpoint &operator+=(const ndpoint &x) {
    *p += x.p;
    return *this;
  }
  friend ndpoint operator+(const ndpoint &x, const ndpoint &y) {
    return ndpoint(x) += y;
  }
};

} // namespace RegionCalculus2

#endif // #ifndef REGIONCALCULUS2_HPP
