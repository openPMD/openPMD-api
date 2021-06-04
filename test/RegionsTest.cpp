#include <openPMD/regions/Regions.hpp>

#include <catch2/catch.hpp>

#include <cstddef>
#include <limits>
#include <random>
#include <type_traits>

using namespace openPMD::Regions;

template <typename T> auto maxabs(const T &xs) {
  using std::abs, std::max_element;
  return max_element(abs(xs));
}
template <typename T> bool is_approx(const T &xs, const T &ys) {
  return maxabs(xs - ys) <=
         10 * std::numeric_limits<typename T::value_type>::epsilon();
}

template <typename P> void test_Point_bool(const P &p) {
  const std::size_t D = p.size();
  using T = typename P::value_type;

  std::mt19937 gen;
  std::uniform_int_distribution dist(0, 1);
  const auto rand = [&]() { return dist(gen); };

  P n = p;
  REQUIRE(n.size() == D);
  for (std::size_t d = 0; d < D; ++d)
    REQUIRE(n[d] == 0);

  const P x = fmap([&](auto) { return rand(); }, p);
  const P y = fmap([&](auto) { return rand(); }, p);
  const P z = fmap([&](auto) { return rand(); }, p);

  const T a = rand();
  const T b = rand();

  REQUIRE(!any(n));
  REQUIRE(all(!n));

  REQUIRE((n & x) == n);
  REQUIRE((0 & x) == n);
  REQUIRE((x & n) == n);
  REQUIRE((x & 0) == n);

  REQUIRE((!n & x) == x);
  REQUIRE((T(1) & x) == x);
  REQUIRE((x & !n) == x);
  REQUIRE((x & T(1)) == x);

  REQUIRE((n | x) == x);
  REQUIRE((0 | x) == x);
  REQUIRE((x | n) == x);
  REQUIRE((x | 0) == x);

  REQUIRE((!n | x) == !n);
  REQUIRE((T(1) | x) == !n);
  REQUIRE((x | !n) == !n);
  REQUIRE((x | T(1)) == !n);

  REQUIRE((x & y) == (y & x));
  REQUIRE((x | y) == (y | x));

  REQUIRE(((x & y) & z) == (x & (y & z)));
  REQUIRE(((x | y) | z) == (x | (y | z)));

  REQUIRE((x & (y | z)) == ((y & x) | (x & z)));
  REQUIRE((x | (y & z)) == ((y | x) & (x | z)));

  REQUIRE((x & y) == !(!x | !y));
  REQUIRE((x | y) == !(!x & !y));

  REQUIRE((n ^ x) == x);
  REQUIRE((0 ^ x) == x);
  REQUIRE((x ^ n) == x);
  REQUIRE((x ^ 0) == x);

  REQUIRE((!n ^ x) == !x);
  REQUIRE((T(1) ^ x) == !x);
  REQUIRE((x ^ !n) == !x);
  REQUIRE((x ^ T(1)) == !x);

  REQUIRE((x ^ x) == n);

  REQUIRE((x ^ y) == (y ^ x));
  REQUIRE(((x ^ y) ^ z) == (x ^ (y ^ z)));

  REQUIRE(!(!x) == x);

  REQUIRE((n && x) == n);
  REQUIRE((0 && x) == n);
  REQUIRE((x && n) == n);
  REQUIRE((x && 0) == n);

  REQUIRE((!n && x) == x);
  REQUIRE((!T(0) && x) == x);
  REQUIRE((x && !n) == x);
  REQUIRE((x && !T(0)) == x);

  REQUIRE((n || x) == x);
  REQUIRE((0 || x) == x);
  REQUIRE((x || n) == x);
  REQUIRE((x || 0) == x);

  REQUIRE((!n || x) == !n);
  REQUIRE((!T(0) || x) == !n);
  REQUIRE((x || !n) == !n);
  REQUIRE((x || !T(0)) == !n);

  REQUIRE((x && y) == (y && x));
  REQUIRE((x || y) == (y || x));

  REQUIRE(((x && y) && z) == (x && (y && z)));
  REQUIRE(((x || y) || z) == (x || (y || z)));

  REQUIRE((x && (y || z)) == ((y && x) || (x && z)));
  REQUIRE((x || (y && z)) == ((y || x) && (x || z)));

  REQUIRE((x && y) == !(!x || !y));
  REQUIRE((x || y) == !(!x && !y));

  P t;
  t = x;
  t &= y;
  REQUIRE(t == (x & y));
  t = x;
  t |= y;
  REQUIRE(t == (x | y));
  t = x;
  t ^= y;
  REQUIRE(t == (x ^ y));
}

template <typename P> void test_Point_int(const P &p) {
  const std::size_t D = p.size();
  using T = typename P::value_type;

  std::mt19937 gen;
  std::uniform_int_distribution dist(-1000, 1000);
  const auto rand = [&]() { return dist(gen); };

  P n(p);
  REQUIRE(n.size() == D);
  for (std::size_t d = 0; d < D; ++d)
    REQUIRE(n[d] == 0);

  const P x = fmap([&](auto) { return rand(); }, p);
  const P y = fmap([&](auto) { return rand(); }, p);
  const P z = fmap([&](auto) { return rand(); }, p);

  const T a = rand();
  const T b = rand();

  REQUIRE(fmap([](auto i) { return i; }, x) == x);
  REQUIRE(fmap([](auto i) { return i + 1; },
               fmap([](auto i) { return 2 * i; }, x)) ==
          fmap([](auto i) { return 2 * i + 1; }, x));

  REQUIRE(fmap([](auto i, auto j) { return 2 * i + j; }, x, y) == 2 * x + y);
  REQUIRE(fmap([](auto i, auto j, auto k) { return 3 * i + 2 * j + k; }, x, y,
               z) == 3 * x + 2 * y + z);

  REQUIRE(fold([](auto i, auto j) { return i + j; }, 0, x) == sum(x));
  REQUIRE(fold([](auto i, auto j, auto k) { return i + j + k; }, 0, x, y) ==
          sum(x + y));

  REQUIRE(sum(n) == 0);
  REQUIRE(sum(n + 1) == D);
  REQUIRE(product(n) == (D == 0 ? 1 : 0));
  REQUIRE(product(n + 1) == 1);
  REQUIRE(min_element(n) == (D == 0 ? std::numeric_limits<T>::max() : 0));
  REQUIRE(max_element(n) == (D == 0 ? std::numeric_limits<T>::min() : 0));
  REQUIRE(min_element(n + 1) == (D == 0 ? std::numeric_limits<T>::max() : 1));
  REQUIRE(max_element(n + 1) == (D == 0 ? std::numeric_limits<T>::min() : 1));

  REQUIRE(+x == x);
  REQUIRE(n + x == x);
  REQUIRE(T(0) + x == x);
  REQUIRE(x + n == x);
  REQUIRE(x + T(0) == x);

  REQUIRE(x + y == y + x);

  REQUIRE((x + y) + z == x + (y + z));

  REQUIRE(-x == -T(1) * x);
  REQUIRE(-(-x) == x);
  REQUIRE(x - x == n);

  REQUIRE(a * n == n);
  REQUIRE(n * a == n);
  REQUIRE(T(0) * x == n);
  REQUIRE(x * T(0) == n);
  REQUIRE(T(1) * x == x);
  REQUIRE(x * T(1) == x);

  REQUIRE(a * x == x * a);

  REQUIRE(a * x + b * x == (a + b) * x);
  REQUIRE(a * (x + y) == a * x + a * y);

  REQUIRE(x * (y + z) == x * y + x * z);

  if (min_element(abs(y)) != 0) {
    REQUIRE(x * y / y == x);
    REQUIRE(x / y * y + x % y == x);
  }

  REQUIRE(~(~x) == x);

  REQUIRE((n & x) == n);
  REQUIRE((0 & x) == n);
  REQUIRE((x & n) == n);
  REQUIRE((x & 0) == n);

  REQUIRE((~n & x) == x);
  REQUIRE((~T(0) & x) == x);
  REQUIRE((x & ~n) == x);
  REQUIRE((x & ~T(0)) == x);

  REQUIRE((n | x) == x);
  REQUIRE((0 | x) == x);
  REQUIRE((x | n) == x);
  REQUIRE((x | 0) == x);

  REQUIRE((~n | x) == ~n);
  REQUIRE((~T(0) | x) == ~n);
  REQUIRE((x | ~n) == ~n);
  REQUIRE((x | ~T(0)) == ~n);

  REQUIRE((x & y) == (y & x));
  REQUIRE((x | y) == (y | x));

  REQUIRE(((x & y) & z) == (x & (y & z)));
  REQUIRE(((x | y) | z) == (x | (y | z)));

  REQUIRE((x & (y | z)) == ((y & x) | (x & z)));
  REQUIRE((x | (y & z)) == ((y | x) & (x | z)));

  REQUIRE((x & y) == ~(~x | ~y));
  REQUIRE((x | y) == ~(~x & ~y));

  REQUIRE((n ^ x) == x);
  REQUIRE((0 ^ x) == x);
  REQUIRE((x ^ n) == x);
  REQUIRE((x ^ 0) == x);

  REQUIRE((~n ^ x) == ~x);
  REQUIRE((~T(0) ^ x) == ~x);
  REQUIRE((x ^ ~n) == ~x);
  REQUIRE((x ^ ~T(0)) == ~x);

  REQUIRE((x ^ x) == n);

  REQUIRE((x ^ y) == (y ^ x));
  REQUIRE(((x ^ y) ^ z) == (x ^ (y ^ z)));

  P t;
  t = x;
  t += y;
  REQUIRE(t == x + y);
  t = x;
  t -= y;
  REQUIRE(t == x - y);
  t = x;
  t *= y;
  REQUIRE(t == x * y);
  t = x;
  t /= y;
  REQUIRE(t == x / y);
  t = x;
  t %= y;
  REQUIRE(t == x % y);
  t = x;
  t &= y;
  REQUIRE(t == (x & y));
  t = x;
  t |= y;
  REQUIRE(t == (x | y));
  t = x;
  t ^= y;
  REQUIRE(t == (x ^ y));
}

template <typename P> void test_Point_float(const P &p) {
  const std::size_t D = p.size();
  using T = typename P::value_type;

  std::mt19937 gen;
  std::uniform_real_distribution dist(-1.0, 1.0);
  const auto rand = [&]() { return dist(gen); };

  P n(p);
  REQUIRE(n.size() == D);
  for (std::size_t d = 0; d < D; ++d)
    REQUIRE(n[d] == 0);

  const P x = fmap([&](auto) { return rand(); }, p);
  const P y = fmap([&](auto) { return rand(); }, p);
  const P z = fmap([&](auto) { return rand(); }, p);

  const T a = rand();
  const T b = rand();

  REQUIRE(fmap([](auto i) { return i; }, x) == x);
  REQUIRE(fmap([](auto i) { return i + 1; },
               fmap([](auto i) { return 2 * i; }, x)) ==
          fmap([](auto i) { return 2 * i + 1; }, x));

  REQUIRE(fmap([](auto i, auto j) { return 2 * i + j; }, x, y) == 2 * x + y);
  REQUIRE(fmap([](auto i, auto j, auto k) { return 3 * i + 2 * j + k; }, x, y,
               z) == 3 * x + 2 * y + z);

  REQUIRE(fold([](auto i, auto j) { return i + j; }, T(0), x) == sum(x));
  REQUIRE(fold([](auto i, auto j, auto k) { return i + j + k; }, T(0), x, y) ==
          sum(x + y));

  REQUIRE(sum(n) == 0);
  REQUIRE(sum(n + 1) == D);
  REQUIRE(product(n) == (D == 0 ? 1 : 0));
  REQUIRE(product(n + 1) == 1);
  REQUIRE(min_element(n) == (D == 0 ? T(1) / 0 : 0));
  REQUIRE(max_element(n) == (D == 0 ? -T(1) / 0 : 0));
  REQUIRE(min_element(n + 1) == (D == 0 ? T(1) / 0 : 1));
  REQUIRE(max_element(n + 1) == (D == 0 ? -T(1) / 0 : 1));

  REQUIRE(+x == x);
  REQUIRE(n + x == x);
  REQUIRE(T(0) + x == x);
  REQUIRE(x + n == x);
  REQUIRE(x + T(0) == x);

  REQUIRE(x + y == y + x);

  REQUIRE(is_approx((x + y) + z, x + (y + z)));

  REQUIRE(-x == -T(1) * x);
  REQUIRE(-(-x) == x);
  REQUIRE(x - x == n);

  REQUIRE(a * n == n);
  REQUIRE(n * a == n);
  REQUIRE(T(0) * x == n);
  REQUIRE(x * T(0) == n);
  REQUIRE(T(1) * x == x);
  REQUIRE(x * T(1) == x);

  REQUIRE(a * x == x * a);

  if (min_element(abs(y)) != 0) {
    REQUIRE(x / x == n + 1);
    REQUIRE(1 / (1 / x) == x);
    REQUIRE(a / x == a * (1 / x));
  }
  if (a != 0) {
    REQUIRE(is_approx(x / a, x * (1 / a)));
  }

  REQUIRE(is_approx(a * x + b * x, (a + b) * x));
  REQUIRE(is_approx(a * (x + y), a * x + a * y));

  REQUIRE(is_approx(x * (y + z), x * y + x * z));

  if (min_element(abs(y)) != 0) {
    REQUIRE(is_approx(x * y / y, x));
  }

  P t;
  t = x;
  t += y;
  REQUIRE(t == x + y);
  t = x;
  t -= y;
  REQUIRE(t == x - y);
  t = x;
  t *= y;
  REQUIRE(t == x * y);
  t = x;
  t /= y;
  REQUIRE(t == x / y);
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

TEST_CASE("NDPoint<std::ptrdiff_t(0)", "[regions]") {
  test_Point_int(NDPoint<std::ptrdiff_t>(0));
}
TEST_CASE("NDPoint<std::ptrdiff_t(1)", "[regions]") {
  test_Point_int(NDPoint<std::ptrdiff_t>(1));
}
TEST_CASE("NDPoint<std::ptrdiff_t(2)", "[regions]") {
  test_Point_int(NDPoint<std::ptrdiff_t>(2));
}
TEST_CASE("NDPoint<std::ptrdiff_t(3)", "[regions]") {
  test_Point_int(NDPoint<std::ptrdiff_t>(3));
}

TEST_CASE("NDPoint<double(0)", "[regions]") {
  test_Point_float(NDPoint<double>(0));
}
TEST_CASE("NDPoint<double(1)", "[regions]") {
  test_Point_float(NDPoint<double>(1));
}
TEST_CASE("NDPoint<double(2)", "[regions]") {
  test_Point_float(NDPoint<double>(2));
}
TEST_CASE("NDPoint<double(3)", "[regions]") {
  test_Point_float(NDPoint<double>(3));
}
