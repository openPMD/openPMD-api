#include <openPMD/regions/Box.hpp>
#include <openPMD/regions/Point.hpp>
#include <openPMD/regions/Region.hpp>

#include <catch2/catch.hpp>

#include <cstddef>
#include <functional>
#include <limits>
#include <random>
#include <type_traits>

using namespace openPMD::Regions;

template <typename R> void test_Region(const R &r) {
  const std::size_t D = r.ndims();
  const auto b = bounding_box(r);
  const auto p = b.lower();
  using B = std::decay_t<decltype(b)>;
  using P = std::decay_t<decltype(p)>;
  const std::equal_to<R> eqr;
  const std::less<R> ltr;
  CHECK(r.empty());
  CHECK(b.empty());

  std::mt19937 gen;
  std::uniform_int_distribution dist0(0, 9);
  std::uniform_int_distribution dist(-1000, 1000);
  const auto rand = [&]() { return dist(gen); };
  const auto randp = [&]() { return fmap([&](auto) { return rand(); }, p); };
  const auto randb = [&]() {
    if (D == 0) {
      if (dist0(gen) < 5)
        return B();
      else
        return B(p);
    }
    if (dist0(gen) == 0)
      return b;
    while (1) {
      auto lo = randp();
      auto hi = randp();
      auto nb = B(lo, hi);
      if (!nb.empty())
        return nb;
    }
  };
  const auto randr = [&]() {
    if (D == 0) {
      if (dist0(gen) < 5)
        return R();
      else
        return R(B(p));
    }
    const int nboxes = dist0(gen) / 2;
    R nr;
    for (int n = 0; n < nboxes; ++n)
      nr |= randb();
    return nr;
  };

  for (int iter = 0; iter < 100; ++iter) {

    R N = r;
    CHECK(N.empty());
    R X = randr();
    R Y = randr();
    R Z = randr();

    P n = p;
    P x = randp();
    P y = randp();

    CHECK(eqr(X, X));
    CHECK(!ltr(X, X));
    if (eqr(X, Y))
      CHECK(ltr(X, Y) + ltr(Y, X) == 0);
    else
      CHECK(ltr(X, Y) + ltr(Y, X) == 1);
    if (ltr(X, Y) && ltr(Y, Z))
      CHECK(ltr(X, Z));
    if (!ltr(Y, X) && !ltr(Z, Y))
      CHECK(!ltr(Z, X));

    CHECK(N == N);
    CHECK(X == X);
    CHECK((N != X) != X.empty());

    CHECK((X >> n) == X);
    CHECK((X >> x) == (X << -x));
    CHECK(((X >> x) << x) == X);
    CHECK(((X >> x) >> y) == (X >> (x + y)));

    CHECK((X * abs(x)) * abs(y) == X * (abs(x) * abs(y)));
    CHECK((X >> x) * y == (X * y) >> (x * y));

    if (X.empty()) {
      CHECK(X.grown(abs(x)).empty());
      CHECK(X.shrunk(abs(x)).empty());
    } else if (all(abs(x) == n)) {
      CHECK(X.grown(abs(x)) == X);
      CHECK(X.shrunk(abs(x)) == X);
    } else {
      CHECK(X.grown(abs(x)) > X);
      CHECK(X.shrunk(abs(x)) < X);
    }

    CHECK(X.grown(abs(x)).shrunk(abs(x)) >= X);
    CHECK(X.shrunk(abs(x)).grown(abs(x)) <= X);

    CHECK((X.grown(x) >> y) == (X >> y).grown(x));

    CHECK((X * abs(y)).grown(abs(x) * abs(y)) == X.grown(abs(x)) * abs(y));

    const B E = bounding_box(bounding_box(bounding_box(X), bounding_box(Y)),
                             bounding_box(Z))
                    .grown(10);

    CHECK((N & X) == N);
    CHECK((X & N) == N);
    CHECK((E & X) == X);
    CHECK((X & E) == X);
    CHECK((X & Y) == (Y & X));
    CHECK(((X & Y) & Z) == (X & (Y & Z)));

    CHECK((N | X) == X);
    CHECK((E | X) == E);
    CHECK((X | E) == E);
    CHECK((X | Y) == (Y | X));
    CHECK(((X | Y) | Z) == (X | (Y | Z)));

    CHECK(E - (X & Y) == ((E - X) | (E - Y)));
    CHECK(E - (X | Y) == ((E - X) & (E - Y)));

    CHECK((N ^ X) == X);
    CHECK((X ^ N) == X);
    CHECK((X ^ X) == N);
    CHECK((X ^ Y) == (Y ^ X));
    CHECK(((X ^ Y) ^ Z) == (X ^ (Y ^ Z)));

    const R IXY = X & Y;
    CHECK((IXY <= X && IXY <= Y) == true);
    CHECK((IXY.grown(1) <= X && IXY.grown(1) <= Y) == (D == 0 || IXY.empty()));

    const R UXY = X | Y;
    CHECK((X <= UXY && Y <= UXY) == true);

    const R DXY = X - Y;
    CHECK((DXY <= X || !isdisjoint(DXY, Y)) == true);

    const R SXY = X ^ Y;
    CHECK((SXY <= UXY && isdisjoint(SXY, IXY)) == true);

    CHECK(IXY <= UXY);
    CHECK((IXY | SXY) == UXY);

  } // for iter
}

TEST_CASE("Region<std::ptrdiff_t,0>", "[regions]") {
  test_Region(Region<std::ptrdiff_t, 0>());
}
TEST_CASE("Region<std::ptrdiff_t,1>", "[regions]") {
  test_Region(Region<std::ptrdiff_t, 1>());
}
TEST_CASE("Region<std::ptrdiff_t,2>", "[regions]") {
  test_Region(Region<std::ptrdiff_t, 2>());
}
TEST_CASE("Region<std::ptrdiff_t,3>", "[regions]") {
  test_Region(Region<std::ptrdiff_t, 3>());
}

TEST_CASE("Region<double,0>", "[regions]") { test_Region(Region<double, 0>()); }
TEST_CASE("Region<double,1>", "[regions]") { test_Region(Region<double, 1>()); }
TEST_CASE("Region<double,2>", "[regions]") { test_Region(Region<double, 2>()); }
TEST_CASE("Region<double,3>", "[regions]") { test_Region(Region<double, 3>()); }

// TODO
#if 0
TEST_CASE("NDRegion<std::ptrdiff_t>(0)", "[regions]") {
  test_Region(NDRegion<std::ptrdiff_t>(0));
}
TEST_CASE("NDRegion<std::ptrdiff_t>(1)", "[regions]") {
  test_Region(NDRegion<std::ptrdiff_t>(1));
}
TEST_CASE("NDRegion<std::ptrdiff_t>(2)", "[regions]") {
  test_Region(NDRegion<std::ptrdiff_t>(2));
}
TEST_CASE("NDRegion<std::ptrdiff_t>(3)", "[regions]") {
  test_Region(NDRegion<std::ptrdiff_t>(3));
}

TEST_CASE("NDRegion<double>(0)", "[regions]") { test_Region(NDRegion<double>(0)); }
TEST_CASE("NDRegion<double>(1)", "[regions]") { test_Region(NDRegion<double>(1)); }
TEST_CASE("NDRegion<double>(2)", "[regions]") { test_Region(NDRegion<double>(2)); }
TEST_CASE("NDRegion<double>(3)", "[regions]") { test_Region(NDRegion<double>(3)); }
#endif
