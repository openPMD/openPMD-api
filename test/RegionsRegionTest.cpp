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
  using T = typename R::value_type;
  const auto b = bounding_box(r);
  const auto p = b.lower();
  typedef std::decay_t<decltype(b)> B;
  typedef std::decay_t<decltype(p)> P;
  const std::equal_to<P> eqp;
  const std::equal_to<B> eqb;
  const std::equal_to<R> eqr;
  const std::less<P> ltp;
  const std::less<B> ltb;
  const std::less<R> ltr;
  REQUIRE(r.empty());
  REQUIRE(b.empty());

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
    R r;
    for (int n = 0; n < nboxes; ++n)
      r |= randb();
    return r;
  };

  for (int iter = 0; iter < 100; ++iter) {

    R N = r;
    REQUIRE(N.empty());
    R X = randr();
    R Y = randr();
    R Z = randr();

    const B E = bounding_box(bounding_box(bounding_box(X), bounding_box(Y)),
                             bounding_box(Z))
                    .grown(10);

    REQUIRE((N & X) == N);
    REQUIRE((X & N) == N);
    REQUIRE((E & X) == X);
    REQUIRE((X & E) == X);
    REQUIRE((X & Y) == (Y & X));
    REQUIRE(((X & Y) & Z) == (X & (Y & Z)));

    REQUIRE((N | X) == X);
    REQUIRE((E | X) == E);
    REQUIRE((X | E) == E);
    REQUIRE((X | Y) == (Y | X));
    REQUIRE(((X | Y) | Z) == (X | (Y | Z)));

    REQUIRE(E - (X & Y) == ((E - X) | (E - Y)));
    REQUIRE(E - (X | Y) == ((E - X) & (E - Y)));

    const R IXY = X & Y;
    REQUIRE((IXY <= X && IXY <= Y) == true);
    REQUIRE((IXY.grown(1) <= X && IXY.grown(1) <= Y) ==
            (D == 0 || IXY.empty()));

    const R UXY = X | Y;
    REQUIRE((X <= UXY && Y <= UXY) == true);

    const R DXY = X - Y;
    REQUIRE((DXY <= X || !isdisjoint(DXY, Y)) == true);

    const R SXY = X ^ Y;
    REQUIRE((SXY <= UXY && isdisjoint(SXY, IXY)) == true);

    REQUIRE(IXY <= UXY);
    REQUIRE((IXY | SXY) == UXY);

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

#warning "TODO"
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
