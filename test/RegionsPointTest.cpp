#include <openPMD/regions/NDPoint.hpp>
#include <openPMD/regions/Point.hpp>

#include <catch2/catch.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <random>
#include <type_traits>

using namespace openPMD::Regions;

template <typename T, typename VT = typename T::value_type>
auto maxabs(const T &xs) {
  if (xs.size() == 0)
    return VT(0);
  using std::abs, std::max_element;
  return max_element(abs(xs));
}
template <typename T, std::enable_if_t<std::is_arithmetic_v<T>> * = nullptr>
auto maxabs(const T &x) {
  using std::abs;
  return abs(x);
}
template <typename T> bool is_approx(const T &xs, const T &ys) {
  using std::max;
  auto m = maxabs(xs - ys);
  if (m == 0)
    return true;
  m /= max(maxabs(xs), maxabs(ys));
  const auto eps = std::numeric_limits<decltype(m)>::epsilon();
  return m <= 100 * eps;
}

template <typename T> bool eq_helper(const T &x1, const T &x2) {
  std::equal_to<T> eq;
  return eq(x1, x2);
}
template <typename T1, typename T2> bool eq_helper(const T1 &, const T2 &) {
  return false;
}

template <typename P> void test_Point_bool(const P &p) {
  const std::size_t D = p.ndims();
  using T = typename P::value_type;
  const std::equal_to<P> eq;
  const std::less<P> lt;

  std::mt19937 gen;
  std::uniform_int_distribution dist(0, 1);
  const auto rand = [&]() { return dist(gen); };
  const auto randp = [&]() { return fmap([&](auto) { return rand(); }, p); };

  for (int iter = 0; iter < 100; ++iter) {

    P n = p;
    CHECK(n.ndims() == D);
    for (std::size_t d = 0; d < D; ++d)
      CHECK(n[d] == 0);
    CHECK(n.size() == D);

    const P x = randp();
    const P y = randp();
    const P z = randp();

    CHECK(eq(n, n));
    CHECK(eq(x, x));
    CHECK(!lt(n, n));
    CHECK(!lt(x, x));
    if (all(x == n)) {
      CHECK(eq(n, x));
      CHECK(!lt(n, x));
    } else {
      CHECK(!eq(n, x));
      CHECK(lt(n, x));
    }

    CHECK(!any(n));
    CHECK(all(!n));

    CHECK(eq(n & x, n));
    CHECK(eq(0 & x, n));
    CHECK(eq(x & n, n));
    CHECK(eq(x & 0, n));

    CHECK(eq((!n & x), x));
    CHECK(eq((T(1) & x), x));
    CHECK(eq((x & !n), x));
    CHECK(eq((x & T(1)), x));

    CHECK(eq((n | x), x));
    CHECK(eq((0 | x), x));
    CHECK(eq((x | n), x));
    CHECK(eq((x | 0), x));

    CHECK(eq((!n | x), !n));
    CHECK(eq((T(1) | x), !n));
    CHECK(eq((x | !n), !n));
    CHECK(eq((x | T(1)), !n));

    CHECK(eq((x & y), (y & x)));
    CHECK(eq((x | y), (y | x)));

    CHECK(eq(((x & y) & z), (x & (y & z))));
    CHECK(eq(((x | y) | z), (x | (y | z))));

    CHECK(eq((x & (y | z)), ((y & x) | (x & z))));
    CHECK(eq((x | (y & z)), ((y | x) & (x | z))));

    CHECK(eq((x & y), !(!x | !y)));
    CHECK(eq((x | y), !(!x & !y)));

    CHECK(eq((n ^ x), x));
    CHECK(eq((0 ^ x), x));
    CHECK(eq((x ^ n), x));
    CHECK(eq((x ^ 0), x));

    CHECK(eq((!n ^ x), !x));
    CHECK(eq((T(1) ^ x), !x));
    CHECK(eq((x ^ !n), !x));
    CHECK(eq((x ^ T(1)), !x));

    CHECK(eq((x ^ x), n));

    CHECK(eq((x ^ y), (y ^ x)));
    CHECK(eq(((x ^ y) ^ z), (x ^ (y ^ z))));

    CHECK(eq(!(!x), x));

    CHECK(eq((n && x), n));
    CHECK(eq((0 && x), n));
    CHECK(eq((x && n), n));
    CHECK(eq((x && 0), n));

    CHECK(eq((!n && x), x));
    CHECK(eq((!T(0) && x), x));
    CHECK(eq((x && !n), x));
    CHECK(eq((x && !T(0)), x));

    CHECK(eq((n || x), x));
    CHECK(eq((0 || x), x));
    CHECK(eq((x || n), x));
    CHECK(eq((x || 0), x));

    CHECK(eq((!n || x), !n));
    CHECK(eq((!T(0) || x), !n));
    CHECK(eq((x || !n), !n));
    CHECK(eq((x || !T(0)), !n));

    CHECK(eq((x && y), (y && x)));
    CHECK(eq((x || y), (y || x)));

    CHECK(eq(((x && y) && z), (x && (y && z))));
    CHECK(eq(((x || y) || z), (x || (y || z))));

    CHECK(eq((x && (y || z)), ((y && x) || (x && z))));
    CHECK(eq((x || (y && z)), ((y || x) && (x || z))));

    CHECK(eq((x && y), !(!x || !y)));
    CHECK(eq((x || y), !(!x && !y)));

    P t;
    t = x;
    t &= y;
    CHECK(eq(t, (x & y)));
    t = x;
    t |= y;
    CHECK(eq(t, (x | y)));
    t = x;
    t ^= y;
    CHECK(eq(t, (x ^ y)));

  } // for iter
}

template <typename P> void test_Point_int(const P &p) {
  const std::size_t D = p.ndims();
  using T = typename P::value_type;
  const std::equal_to<P> eq;

  std::mt19937 gen;
  std::uniform_int_distribution dist(-1000, 1000);
  const auto rand = [&]() { return dist(gen); };
  const auto randp = [&]() { return fmap([&](auto) { return rand(); }, p); };

  for (int iter = 0; iter < 100; ++iter) {

    P n(p);
    CHECK(n.size() == D);
    for (std::size_t d = 0; d < D; ++d)
      CHECK(n[d] == 0);

    const P x = randp();
    const P y = randp();
    const P z = randp();

    const T a = rand();
    const T b = rand();

    CHECK(eq(fmap([](auto i) { return i; }, x), x));
    CHECK(eq(fmap([](auto i) { return i + 1; },
                  fmap([](auto i) { return 2 * i; }, x)),
             fmap([](auto i) { return 2 * i + 1; }, x)));

    CHECK(eq(fmap([](auto i, auto j) { return 2 * i + j; }, x, y), 2 * x + y));
    CHECK(eq(
        fmap([](auto i, auto j, auto k) { return 3 * i + 2 * j + k; }, x, y, z),
        3 * x + 2 * y + z));

    CHECK(fold([](auto i, auto j) { return i + j; }, 0, x) == sum(x));
    CHECK(fold([](auto i, auto j, auto k) { return i + j + k; }, 0, x, y) ==
          sum(x + y));

    CHECK(sum(n) == 0);
    CHECK(sum(n + 1) == std::ptrdiff_t(D));
    CHECK(product(n) == (D == 0 ? 1 : 0));
    CHECK(product(n + 1) == 1);
    CHECK(min_element(n) == (D == 0 ? std::numeric_limits<T>::max() : 0));
    CHECK(max_element(n) == (D == 0 ? std::numeric_limits<T>::min() : 0));
    CHECK(min_element(n + 1) == (D == 0 ? std::numeric_limits<T>::max() : 1));
    CHECK(max_element(n + 1) == (D == 0 ? std::numeric_limits<T>::min() : 1));

    CHECK(eq(+x, x));
    CHECK(eq(n + x, x));
    CHECK(eq(T(0) + x, x));
    CHECK(eq(x + n, x));
    CHECK(eq(x + T(0), x));

    CHECK(eq(x + y, y + x));

    CHECK(eq((x + y) + z, x + (y + z)));

    CHECK(eq(-x, -T(1) * x));
    CHECK(eq(-(-x), x));
    CHECK(eq(x - x, n));

    CHECK(eq(a * n, n));
    CHECK(eq(n * a, n));
    CHECK(eq(T(0) * x, n));
    CHECK(eq(x * T(0), n));
    CHECK(eq(T(1) * x, x));
    CHECK(eq(x * T(1), x));

    CHECK(eq(a * x, x * a));

    CHECK(eq(a * x + b * x, (a + b) * x));
    CHECK(eq(a * (x + y), a * x + a * y));

    CHECK(eq(x * (y + z), x * y + x * z));

    if (all(y != 0)) {
      CHECK(eq(x * y / y, x));
      CHECK(eq(x / y * y + x % y, x));
    }

    CHECK(eq(~(~x), x));

    CHECK(eq((n & x), n));
    CHECK(eq((0 & x), n));
    CHECK(eq((x & n), n));
    CHECK(eq((x & 0), n));

    CHECK(eq((~n & x), x));
    CHECK(eq((~T(0) & x), x));
    CHECK(eq((x & ~n), x));
    CHECK(eq((x & ~T(0)), x));

    CHECK(eq((n | x), x));
    CHECK(eq((0 | x), x));
    CHECK(eq((x | n), x));
    CHECK(eq((x | 0), x));

    CHECK(eq((~n | x), ~n));
    CHECK(eq((~T(0) | x), ~n));
    CHECK(eq((x | ~n), ~n));
    CHECK(eq((x | ~T(0)), ~n));

    CHECK(eq((x & y), (y & x)));
    CHECK(eq((x | y), (y | x)));

    CHECK(eq(((x & y) & z), (x & (y & z))));
    CHECK(eq(((x | y) | z), (x | (y | z))));

    CHECK(eq((x & (y | z)), ((y & x) | (x & z))));
    CHECK(eq((x | (y & z)), ((y | x) & (x | z))));

    CHECK(eq((x & y), ~(~x | ~y)));
    CHECK(eq((x | y), ~(~x & ~y)));

    CHECK(eq((n ^ x), x));
    CHECK(eq((0 ^ x), x));
    CHECK(eq((x ^ n), x));
    CHECK(eq((x ^ 0), x));

    CHECK(eq((~n ^ x), ~x));
    CHECK(eq((~T(0) ^ x), ~x));
    CHECK(eq((x ^ ~n), ~x));
    CHECK(eq((x ^ ~T(0)), ~x));

    CHECK(eq((x ^ x), n));

    CHECK(eq((x ^ y), (y ^ x)));
    CHECK(eq(((x ^ y) ^ z), (x ^ (y ^ z))));

    P t;
    t = x;
    t += y;
    CHECK(eq(t, x + y));
    t = x;
    t -= y;
    CHECK(eq(t, x - y));
    t = x;
    t *= y;
    CHECK(eq(t, x * y));
    if (all(y != 0)) {
      t = x;
      t /= y;
      CHECK(eq(t, x / y));
      t = x;
      t %= y;
      CHECK(eq(t, x % y));
    }
    t = x;
    t &= y;
    CHECK(eq(t, (x & y)));
    t = x;
    t |= y;
    CHECK(eq(t, (x | y)));
    t = x;
    t ^= y;
    CHECK(eq(t, (x ^ y)));

  } // for iter
}

template <typename P> void test_Point_float(const P &p) {
  const std::size_t D = p.ndims();
  using T = typename P::value_type;
  const std::equal_to<P> eq;
  const std::less<P> lt;

  std::mt19937 gen;
  std::uniform_real_distribution dist(-1.0, 1.0);
  const auto rand = [&]() { return dist(gen); };
  const auto randp = [&]() { return fmap([&](auto) { return rand(); }, p); };

  for (int iter = 0; iter < 100; ++iter) {

    P n(p);
    CHECK(n.size() == D);
    for (std::size_t d = 0; d < D; ++d)
      CHECK(n[d] == 0);

    const P x = randp();
    const P y = randp();
    const P z = randp();

    const T a = rand();
    const T b = rand();

    CHECK(eq(x, x));
    CHECK(!lt(x, x));
    if (eq(x, y))
      CHECK(lt(x, y) + lt(y, x) == 0);
    else
      CHECK(lt(x, y) + lt(y, x) == 1);
    if (lt(x, y) && lt(y, z))
      CHECK(lt(x, z));
    if (!lt(y, x) && !lt(z, y))
      CHECK(!lt(z, x));

    // remove-insert is no-op
    if (D > 0) {
      for (std::size_t d = 0; d < D; ++d) {
        const auto a1 = x[d];
        const auto x1 = x.erase(d);
        CHECK(x1.ndims() == D - 1);
        const auto x2 = x1.insert(d, a1);
        CHECK(x2.ndims() == D);
        CHECK(eq_helper(x2, x));
      }
    }
    // insert-remove is no-op
    for (std::size_t d = 0; d <= D; ++d) {
      const auto x1 = x.insert(d, a);
      CHECK(x1.ndims() == D + 1);
      CHECK(x1[d] == a);
      CHECK(eq(x1.erase(d), x));
    }

    CHECK(eq(x.reversed().reversed(), x));

    CHECK(eq(fmap([](auto i) { return i; }, x), x));
    CHECK(eq(fmap([](auto i) { return i + 1; },
                  fmap([](auto i) { return 2 * i; }, x)),
             fmap([](auto i) { return 2 * i + 1; }, x)));

    CHECK(eq(fmap([](auto i, auto j) { return 2 * i + j; }, x, y), 2 * x + y));
    CHECK(eq(
        fmap([](auto i, auto j, auto k) { return 3 * i + 2 * j + k; }, x, y, z),
        3 * x + 2 * y + z));

    CHECK(fold([](auto i, auto j) { return i + j; }, T(0), x) == sum(x));
    CHECK(is_approx(
        fold([](auto i, auto j, auto k) { return i + j + k; }, T(0), x, y),
        sum(x + y)));

    // Temporary tests
    CHECK(0.0 == 0.0);
    CHECK(1.0 == 1.0);
    CHECK(1.0 / 0.0 == 1.0 / 0.0);
    CHECK(-1.0 / 0.0 == -1.0 / 0.0);
    CHECK(1.0 / 0.0 != -1.0 / 0.0);
    CHECK(0.0 / 0.0 != 0.0 / 0.0);
    CHECK(1.0 / 0.0 == std::numeric_limits<double>::infinity());
    CHECK(-1.0 / 0.0 == -std::numeric_limits<double>::infinity());
    CHECK(1.0 / 0.0 == std::numeric_limits<float>::infinity());
    CHECK(-1.0 / 0.0 == -std::numeric_limits<float>::infinity());

    CHECK(sum(n) == 0);
    CHECK(sum(n + 1) == D);
    CHECK(product(n) == (D == 0 ? 1 : 0));
    CHECK(product(n + 1) == 1);
    CHECK(min_element(n) == (D == 0 ? T(1) / 0 : 0));
    CHECK(max_element(n) == (D == 0 ? -T(1) / 0 : 0));
    CHECK(min_element(n + 1) == (D == 0 ? T(1) / 0 : 1));
    CHECK(max_element(n + 1) == (D == 0 ? -T(1) / 0 : 1));

    CHECK(eq(+x, x));
    CHECK(eq(n + x, x));
    CHECK(eq(T(0) + x, x));
    CHECK(eq(x + n, x));
    CHECK(eq(x + T(0), x));

    CHECK(eq(x + y, y + x));

    CHECK(is_approx((x + y) + z, x + (y + z)));

    CHECK(eq(-x, -T(1) * x));
    CHECK(eq(-(-x), x));
    CHECK(eq(x - x, n));

    CHECK(eq(a * n, n));
    CHECK(eq(n * a, n));
    CHECK(eq(T(0) * x, n));
    CHECK(eq(x * T(0), n));
    CHECK(eq(T(1) * x, x));
    CHECK(eq(x * T(1), x));

    CHECK(eq(a * x, x * a));

    if (all(x != 0)) {
      CHECK(eq(x / x, n + 1));
      CHECK(is_approx(1 / (1 / x), x));
      CHECK(is_approx(a / x, a * (1 / x)));
    }
    if (a != 0) {
      CHECK(is_approx(x / a, x * (1 / a)));
    }

    CHECK(is_approx(a * x + b * x, (a + b) * x));
    CHECK(is_approx(a * (x + y), a * x + a * y));

    CHECK(is_approx(x * (y + z), x * y + x * z));

    if (all(y != 0)) {
      CHECK(is_approx(x * y / y, x));
    }

    P t;
    t = x;
    t += y;
    CHECK(eq(t, x + y));
    t = x;
    t -= y;
    CHECK(eq(t, x - y));
    t = x;
    t *= y;
    CHECK(eq(t, x * y));
    t = x;
    t /= y;
    CHECK(eq(t, x / y));

  } // for iter
}

TEST_CASE("Point<bool,0>", "[regions]") { test_Point_bool(Point<bool, 0>()); }
TEST_CASE("Point<bool,1>", "[regions]") { test_Point_bool(Point<bool, 1>()); }
TEST_CASE("Point<bool,2>", "[regions]") { test_Point_bool(Point<bool, 2>()); }
TEST_CASE("Point<bool,3>", "[regions]") { test_Point_bool(Point<bool, 3>()); }

TEST_CASE("Point<std::ptrdiff_t,0>", "[regions]") {
  test_Point_int(Point<std::ptrdiff_t, 0>());
}
TEST_CASE("Point<std::ptrdiff_t,1>", "[regions]") {
  test_Point_int(Point<std::ptrdiff_t, 1>());
}
TEST_CASE("Point<std::ptrdiff_t,2>", "[regions]") {
  test_Point_int(Point<std::ptrdiff_t, 2>());
}
TEST_CASE("Point<std::ptrdiff_t,3>", "[regions]") {
  test_Point_int(Point<std::ptrdiff_t, 3>());
}

TEST_CASE("Point<double,0>", "[regions]") {
  test_Point_float(Point<double, 0>());
}
TEST_CASE("Point<double,1>", "[regions]") {
  test_Point_float(Point<double, 1>());
}
TEST_CASE("Point<double,2>", "[regions]") {
  test_Point_float(Point<double, 2>());
}
TEST_CASE("Point<double,3>", "[regions]") {
  test_Point_float(Point<double, 3>());
}

TEST_CASE("NDPoint<bool>(0)", "[regions]") {
  test_Point_bool(NDPoint<bool>(0));
}
TEST_CASE("NDPoint<bool>(1)", "[regions]") {
  test_Point_bool(NDPoint<bool>(1));
}
TEST_CASE("NDPoint<bool>(2)", "[regions]") {
  test_Point_bool(NDPoint<bool>(2));
}
TEST_CASE("NDPoint<bool>(3)", "[regions]") {
  test_Point_bool(NDPoint<bool>(3));
}

TEST_CASE("NDPoint<std::ptrdiff_t>(0)", "[regions]") {
  test_Point_int(NDPoint<std::ptrdiff_t>(0));
}
TEST_CASE("NDPoint<std::ptrdiff_t>(1)", "[regions]") {
  test_Point_int(NDPoint<std::ptrdiff_t>(1));
}
TEST_CASE("NDPoint<std::ptrdiff_t>(2)", "[regions]") {
  test_Point_int(NDPoint<std::ptrdiff_t>(2));
}
TEST_CASE("NDPoint<std::ptrdiff_t>(3)", "[regions]") {
  test_Point_int(NDPoint<std::ptrdiff_t>(3));
}

TEST_CASE("NDPoint<double>(0)", "[regions]") {
  test_Point_float(NDPoint<double>(0));
}
TEST_CASE("NDPoint<double>(1)", "[regions]") {
  test_Point_float(NDPoint<double>(1));
}
TEST_CASE("NDPoint<double>(2)", "[regions]") {
  test_Point_float(NDPoint<double>(2));
}
TEST_CASE("NDPoint<double>(3)", "[regions]") {
  test_Point_float(NDPoint<double>(3));
}
