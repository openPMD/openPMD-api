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
    REQUIRE(n.ndims() == D);
    for (std::size_t d = 0; d < D; ++d)
      REQUIRE(n[d] == 0);
    REQUIRE(n.size() == D);

    const P x = randp();
    const P y = randp();
    const P z = randp();

    REQUIRE(eq(n, n));
    REQUIRE(eq(x, x));
    REQUIRE(!lt(n, n));
    REQUIRE(!lt(x, x));
    if (all(x == n)) {
      REQUIRE(eq(n, x));
      REQUIRE(!lt(n, x));
    } else {
      REQUIRE(!eq(n, x));
      REQUIRE(lt(n, x));
    }

    REQUIRE(!any(n));
    REQUIRE(all(!n));

    REQUIRE(eq(n & x, n));
    REQUIRE(eq(0 & x, n));
    REQUIRE(eq(x & n, n));
    REQUIRE(eq(x & 0, n));

    REQUIRE(eq((!n & x), x));
    REQUIRE(eq((T(1) & x), x));
    REQUIRE(eq((x & !n), x));
    REQUIRE(eq((x & T(1)), x));

    REQUIRE(eq((n | x), x));
    REQUIRE(eq((0 | x), x));
    REQUIRE(eq((x | n), x));
    REQUIRE(eq((x | 0), x));

    REQUIRE(eq((!n | x), !n));
    REQUIRE(eq((T(1) | x), !n));
    REQUIRE(eq((x | !n), !n));
    REQUIRE(eq((x | T(1)), !n));

    REQUIRE(eq((x & y), (y & x)));
    REQUIRE(eq((x | y), (y | x)));

    REQUIRE(eq(((x & y) & z), (x & (y & z))));
    REQUIRE(eq(((x | y) | z), (x | (y | z))));

    REQUIRE(eq((x & (y | z)), ((y & x) | (x & z))));
    REQUIRE(eq((x | (y & z)), ((y | x) & (x | z))));

    REQUIRE(eq((x & y), !(!x | !y)));
    REQUIRE(eq((x | y), !(!x & !y)));

    REQUIRE(eq((n ^ x), x));
    REQUIRE(eq((0 ^ x), x));
    REQUIRE(eq((x ^ n), x));
    REQUIRE(eq((x ^ 0), x));

    REQUIRE(eq((!n ^ x), !x));
    REQUIRE(eq((T(1) ^ x), !x));
    REQUIRE(eq((x ^ !n), !x));
    REQUIRE(eq((x ^ T(1)), !x));

    REQUIRE(eq((x ^ x), n));

    REQUIRE(eq((x ^ y), (y ^ x)));
    REQUIRE(eq(((x ^ y) ^ z), (x ^ (y ^ z))));

    REQUIRE(eq(!(!x), x));

    REQUIRE(eq((n && x), n));
    REQUIRE(eq((0 && x), n));
    REQUIRE(eq((x && n), n));
    REQUIRE(eq((x && 0), n));

    REQUIRE(eq((!n && x), x));
    REQUIRE(eq((!T(0) && x), x));
    REQUIRE(eq((x && !n), x));
    REQUIRE(eq((x && !T(0)), x));

    REQUIRE(eq((n || x), x));
    REQUIRE(eq((0 || x), x));
    REQUIRE(eq((x || n), x));
    REQUIRE(eq((x || 0), x));

    REQUIRE(eq((!n || x), !n));
    REQUIRE(eq((!T(0) || x), !n));
    REQUIRE(eq((x || !n), !n));
    REQUIRE(eq((x || !T(0)), !n));

    REQUIRE(eq((x && y), (y && x)));
    REQUIRE(eq((x || y), (y || x)));

    REQUIRE(eq(((x && y) && z), (x && (y && z))));
    REQUIRE(eq(((x || y) || z), (x || (y || z))));

    REQUIRE(eq((x && (y || z)), ((y && x) || (x && z))));
    REQUIRE(eq((x || (y && z)), ((y || x) && (x || z))));

    REQUIRE(eq((x && y), !(!x || !y)));
    REQUIRE(eq((x || y), !(!x && !y)));

    P t;
    t = x;
    t &= y;
    REQUIRE(eq(t, (x & y)));
    t = x;
    t |= y;
    REQUIRE(eq(t, (x | y)));
    t = x;
    t ^= y;
    REQUIRE(eq(t, (x ^ y)));

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
    REQUIRE(n.size() == D);
    for (std::size_t d = 0; d < D; ++d)
      REQUIRE(n[d] == 0);

    const P x = randp();
    const P y = randp();
    const P z = randp();

    const T a = rand();
    const T b = rand();

    REQUIRE(eq(fmap([](auto i) { return i; }, x), x));
    REQUIRE(eq(fmap([](auto i) { return i + 1; },
                    fmap([](auto i) { return 2 * i; }, x)),
               fmap([](auto i) { return 2 * i + 1; }, x)));

    REQUIRE(
        eq(fmap([](auto i, auto j) { return 2 * i + j; }, x, y), 2 * x + y));
    REQUIRE(eq(
        fmap([](auto i, auto j, auto k) { return 3 * i + 2 * j + k; }, x, y, z),
        3 * x + 2 * y + z));

    REQUIRE(fold([](auto i, auto j) { return i + j; }, 0, x) == sum(x));
    REQUIRE(fold([](auto i, auto j, auto k) { return i + j + k; }, 0, x, y) ==
            sum(x + y));

    REQUIRE(sum(n) == 0);
    REQUIRE(sum(n + 1) == std::ptrdiff_t(D));
    REQUIRE(product(n) == (D == 0 ? 1 : 0));
    REQUIRE(product(n + 1) == 1);
    REQUIRE(min_element(n) == (D == 0 ? std::numeric_limits<T>::max() : 0));
    REQUIRE(max_element(n) == (D == 0 ? std::numeric_limits<T>::min() : 0));
    REQUIRE(min_element(n + 1) == (D == 0 ? std::numeric_limits<T>::max() : 1));
    REQUIRE(max_element(n + 1) == (D == 0 ? std::numeric_limits<T>::min() : 1));

    REQUIRE(eq(+x, x));
    REQUIRE(eq(n + x, x));
    REQUIRE(eq(T(0) + x, x));
    REQUIRE(eq(x + n, x));
    REQUIRE(eq(x + T(0), x));

    REQUIRE(eq(x + y, y + x));

    REQUIRE(eq((x + y) + z, x + (y + z)));

    REQUIRE(eq(-x, -T(1) * x));
    REQUIRE(eq(-(-x), x));
    REQUIRE(eq(x - x, n));

    REQUIRE(eq(a * n, n));
    REQUIRE(eq(n * a, n));
    REQUIRE(eq(T(0) * x, n));
    REQUIRE(eq(x * T(0), n));
    REQUIRE(eq(T(1) * x, x));
    REQUIRE(eq(x * T(1), x));

    REQUIRE(eq(a * x, x * a));

    REQUIRE(eq(a * x + b * x, (a + b) * x));
    REQUIRE(eq(a * (x + y), a * x + a * y));

    REQUIRE(eq(x * (y + z), x * y + x * z));

    if (all(y != 0)) {
      REQUIRE(eq(x * y / y, x));
      REQUIRE(eq(x / y * y + x % y, x));
    }

    REQUIRE(eq(~(~x), x));

    REQUIRE(eq((n & x), n));
    REQUIRE(eq((0 & x), n));
    REQUIRE(eq((x & n), n));
    REQUIRE(eq((x & 0), n));

    REQUIRE(eq((~n & x), x));
    REQUIRE(eq((~T(0) & x), x));
    REQUIRE(eq((x & ~n), x));
    REQUIRE(eq((x & ~T(0)), x));

    REQUIRE(eq((n | x), x));
    REQUIRE(eq((0 | x), x));
    REQUIRE(eq((x | n), x));
    REQUIRE(eq((x | 0), x));

    REQUIRE(eq((~n | x), ~n));
    REQUIRE(eq((~T(0) | x), ~n));
    REQUIRE(eq((x | ~n), ~n));
    REQUIRE(eq((x | ~T(0)), ~n));

    REQUIRE(eq((x & y), (y & x)));
    REQUIRE(eq((x | y), (y | x)));

    REQUIRE(eq(((x & y) & z), (x & (y & z))));
    REQUIRE(eq(((x | y) | z), (x | (y | z))));

    REQUIRE(eq((x & (y | z)), ((y & x) | (x & z))));
    REQUIRE(eq((x | (y & z)), ((y | x) & (x | z))));

    REQUIRE(eq((x & y), ~(~x | ~y)));
    REQUIRE(eq((x | y), ~(~x & ~y)));

    REQUIRE(eq((n ^ x), x));
    REQUIRE(eq((0 ^ x), x));
    REQUIRE(eq((x ^ n), x));
    REQUIRE(eq((x ^ 0), x));

    REQUIRE(eq((~n ^ x), ~x));
    REQUIRE(eq((~T(0) ^ x), ~x));
    REQUIRE(eq((x ^ ~n), ~x));
    REQUIRE(eq((x ^ ~T(0)), ~x));

    REQUIRE(eq((x ^ x), n));

    REQUIRE(eq((x ^ y), (y ^ x)));
    REQUIRE(eq(((x ^ y) ^ z), (x ^ (y ^ z))));

    P t;
    t = x;
    t += y;
    REQUIRE(eq(t, x + y));
    t = x;
    t -= y;
    REQUIRE(eq(t, x - y));
    t = x;
    t *= y;
    REQUIRE(eq(t, x * y));
    if (all(y != 0)) {
      t = x;
      t /= y;
      REQUIRE(eq(t, x / y));
      t = x;
      t %= y;
      REQUIRE(eq(t, x % y));
    }
    t = x;
    t &= y;
    REQUIRE(eq(t, (x & y)));
    t = x;
    t |= y;
    REQUIRE(eq(t, (x | y)));
    t = x;
    t ^= y;
    REQUIRE(eq(t, (x ^ y)));

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
    REQUIRE(n.size() == D);
    for (std::size_t d = 0; d < D; ++d)
      REQUIRE(n[d] == 0);

    const P x = randp();
    const P y = randp();
    const P z = randp();

    const T a = rand();
    const T b = rand();

    REQUIRE(eq(x, x));
    REQUIRE(!lt(x, x));
    if (eq(x, y))
      REQUIRE(lt(x, y) + lt(y, x) == 0);
    else
      REQUIRE(lt(x, y) + lt(y, x) == 1);
    if (lt(x, y) && lt(y, z))
      REQUIRE(lt(x, z));
    if (!lt(y, x) && !lt(z, y))
      REQUIRE(!lt(z, x));

    // remove-insert is no-op
    if (D > 0) {
      for (std::size_t d = 0; d < D; ++d) {
        const auto a1 = x[d];
        const auto x1 = x.erase(d);
        REQUIRE(x1.ndims() == D - 1);
        const auto x2 = x1.insert(d, a1);
        REQUIRE(x2.ndims() == D);
        REQUIRE(eq_helper(x2, x));
      }
    }
    // insert-remove is no-op
    for (std::size_t d = 0; d <= D; ++d) {
      const auto x1 = x.insert(d, a);
      REQUIRE(x1.ndims() == D + 1);
      REQUIRE(x1[d] == a);
      REQUIRE(eq(x1.erase(d), x));
    }

    REQUIRE(eq(x.reversed().reversed(), x));

    REQUIRE(eq(fmap([](auto i) { return i; }, x), x));
    REQUIRE(eq(fmap([](auto i) { return i + 1; },
                    fmap([](auto i) { return 2 * i; }, x)),
               fmap([](auto i) { return 2 * i + 1; }, x)));

    REQUIRE(
        eq(fmap([](auto i, auto j) { return 2 * i + j; }, x, y), 2 * x + y));
    REQUIRE(eq(
        fmap([](auto i, auto j, auto k) { return 3 * i + 2 * j + k; }, x, y, z),
        3 * x + 2 * y + z));

    REQUIRE(fold([](auto i, auto j) { return i + j; }, T(0), x) == sum(x));
    REQUIRE(is_approx(
        fold([](auto i, auto j, auto k) { return i + j + k; }, T(0), x, y),
        sum(x + y)));

    REQUIRE(sum(n) == 0);
    REQUIRE(sum(n + 1) == D);
    REQUIRE(product(n) == (D == 0 ? 1 : 0));
    REQUIRE(product(n + 1) == 1);
    // We need to allow `inf == inf`, and REQUIRE's standard
    // comparison wouldn't, so we compare `== true` instead
    REQUIRE((min_element(n) == (D == 0 ? T(1) / 0 : 0)) == true);
    REQUIRE((max_element(n) == (D == 0 ? -T(1) / 0 : 0)) == true);
    REQUIRE((min_element(n + 1) == (D == 0 ? T(1) / 0 : 1)) == true);
    REQUIRE((max_element(n + 1) == (D == 0 ? -T(1) / 0 : 1)) == true);

    REQUIRE(eq(+x, x));
    REQUIRE(eq(n + x, x));
    REQUIRE(eq(T(0) + x, x));
    REQUIRE(eq(x + n, x));
    REQUIRE(eq(x + T(0), x));

    REQUIRE(eq(x + y, y + x));

    REQUIRE(is_approx((x + y) + z, x + (y + z)));

    REQUIRE(eq(-x, -T(1) * x));
    REQUIRE(eq(-(-x), x));
    REQUIRE(eq(x - x, n));

    REQUIRE(eq(a * n, n));
    REQUIRE(eq(n * a, n));
    REQUIRE(eq(T(0) * x, n));
    REQUIRE(eq(x * T(0), n));
    REQUIRE(eq(T(1) * x, x));
    REQUIRE(eq(x * T(1), x));

    REQUIRE(eq(a * x, x * a));

    if (all(x != 0)) {
      REQUIRE(eq(x / x, n + 1));
      REQUIRE(is_approx(1 / (1 / x), x));
      REQUIRE(is_approx(a / x, a * (1 / x)));
    }
    if (a != 0) {
      REQUIRE(is_approx(x / a, x * (1 / a)));
    }

    REQUIRE(is_approx(a * x + b * x, (a + b) * x));
    REQUIRE(is_approx(a * (x + y), a * x + a * y));

    REQUIRE(is_approx(x * (y + z), x * y + x * z));

    if (all(y != 0)) {
      REQUIRE(is_approx(x * y / y, x));
    }

    P t;
    t = x;
    t += y;
    REQUIRE(eq(t, x + y));
    t = x;
    t -= y;
    REQUIRE(eq(t, x - y));
    t = x;
    t *= y;
    REQUIRE(eq(t, x * y));
    t = x;
    t /= y;
    REQUIRE(eq(t, x / y));

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
