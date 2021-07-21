#include <openPMD/regions/Box.hpp>
#include <openPMD/regions/NDBox.hpp>

#include <catch2/catch.hpp>

#include <cstddef>
#include <functional>
#include <limits>
#include <random>
#include <type_traits>

using namespace openPMD::Regions;

template <typename B> void test_Box(const B &box) {
  const std::size_t D = box.ndims();
  using T = typename B::value_type;
  const auto p = box.lower();
  typedef std::decay_t<decltype(p)> P;
  const std::equal_to<P> eqp;
  const std::equal_to<B> eqb;
  const std::less<B> ltb;
  CHECK(box.empty());

  std::mt19937 gen;
  std::uniform_int_distribution dist0(0, 9);
  std::uniform_int_distribution dist(-1000, 1000);
  const auto rand = [&]() { return dist(gen); };
  const auto randp = [&]() { return fmap([&](auto) { return rand(); }, p); };
  const auto randb = [&]() {
    if (D == 0) {
      if (dist0(gen) < 5)
        return B(p, p); // empty box
      else
        return B(p);
    }
    if (dist0(gen) == 0)
      return box;
    while (1) {
      auto lo = randp();
      auto hi = randp();
      auto nb = B(lo, hi);
      if (!nb.empty())
        return nb;
    }
  };

  for (int iter = 0; iter < 100; ++iter) {

    B N(box);
    CHECK(N.ndims() == std::ptrdiff_t(D));
    CHECK(N.empty());
    for (std::size_t d = 0; d < D; ++d)
      CHECK(N.lower()[d] >= N.upper()[d]);

    const B X = randb();
    const B Y = randb();
    const B Z = randb();

    const P n = fmap([](auto) { return T(0); }, p);
    CHECK(eqp(n + n, n));
    const P x = randp();
    const P y = randp();

    const T a = rand();

    CHECK(N.empty());
    if (D > 0) {
      CHECK(X.empty() == all(X.shape() == 0));
      CHECK(Y.empty() == all(Y.shape() == 0));
      CHECK(Z.empty() == all(Z.shape() == 0));
    }

    CHECK(X.empty() == (X.size() == 0));
    CHECK(Y.empty() == (Y.size() == 0));
    CHECK(Z.empty() == (Z.size() == 0));

    if (D > 0) {
      CHECK(X.empty() == all(fmap([](auto lo, auto up) { return lo >= up; },
                                  X.lower(), X.upper())));
      CHECK(Y.empty() == all(fmap([](auto lo, auto up) { return lo >= up; },
                                  Y.lower(), Y.upper())));
      CHECK(Z.empty() == all(fmap([](auto lo, auto up) { return lo >= up; },
                                  Z.lower(), Z.upper())));
    }

    CHECK(eqb(N, N));
    CHECK(eqb(X, X));
    CHECK(!ltb(N, N));
    CHECK(!ltb(X, X));
    if (X.empty()) {
      CHECK(eqb(N, X));
      CHECK(!ltb(N, X));
    } else {
      CHECK(!eqb(N, X));
      CHECK(ltb(N, X));
    }
    if (eqb(X, Y))
      CHECK(ltb(X, Y) + ltb(Y, X) == 0);
    else
      CHECK(ltb(X, Y) + ltb(Y, X) == 1);
    if (ltb(X, Y) && ltb(Y, Z))
      CHECK(ltb(X, Z));
    if (!ltb(Y, X) && !ltb(Z, Y))
      CHECK(!ltb(Z, X));

    CHECK(((X >> x) << x) == X);
    CHECK((X >> x) == (X << -x));
    CHECK((X >> x) == (X << -x));
    CHECK((X >> (x + y)) == ((X >> x) >> y));

    CHECK((X * x) * y == X * (x * y));
    CHECK((X >> x) * y == (X * y) >> (x * y));

    CHECK((X.grown(1) == X) == (D == 0 || X.empty()));
    if (all(x >= 0 && y >= 0))
      CHECK(X.grown(x).grown(y) == X.grown(x + y));
    else
      CHECK((X.grown(x).grown(y).empty() ||
             X.grown(x).grown(y) == X.grown(x + y)) == true);
    if (all(x >= 0))
      CHECK(X.grown(x).grown(-x) == X);
    else
      CHECK((X.grown(x).grown(-x).empty() || X.grown(x).grown(-x) == X) ==
            true);
    CHECK(X.grown(x) == X.grown(x, x));
    CHECK(X.grown(a) == X.grown(fmap([&](int) { return a; }, x)));

    CHECK(X.shrunk(x, y) == X.grown(-x, -y));
    CHECK(X.shrunk(x) == X.shrunk(x, x));
    CHECK(X.shrunk(a) == X.shrunk(fmap([&](int) { return a; }, x)));

    CHECK(N == N);
    CHECK(X == X);
    CHECK((N == X) == X.empty());
    CHECK(!(N != N));
    CHECK(!(X != X));
    CHECK((N != X) != (N == X));

    CHECK(X.contains(X.lower()) == !X.empty());
    CHECK(X.contains(X.upper() - 1) == !X.empty());
    CHECK(X.grown(1).contains(X.upper()) == !X.empty());
    CHECK(isdisjoint(X, X) == X.empty());

    // a <= b means "a implies b" for booleans (yes, it looks wrong)
    CHECK((X < Y) <= (X <= Y));
    CHECK((X > Y) <= (X >= Y));
    CHECK((X <= Y) <= (X.empty() || !isdisjoint(X, Y)));
    CHECK((X >= Y) <= (Y.empty() || !isdisjoint(X, Y)));
    CHECK(!(X < Y && Y < X));
    CHECK((X <= Y && X >= Y) == (X == Y));
    CHECK((X < X.grown(1)) == (D > 0 && !X.empty()));
    CHECK((X.shrunk(1) < X) == (D > 0 && !X.empty()));

    CHECK(N <= N);
    CHECK(!(N < N));
    CHECK(N <= X);
    CHECK((N < X) == !X.empty());

    const auto BXY = bounding_box(X, Y);
    CHECK(bounding_box(N, X) == X);
    CHECK(bounding_box(X, N) == X);
    CHECK(bounding_box(X, Y) == bounding_box(Y, X));
    CHECK(bounding_box(bounding_box(X, Y), Z) ==
          bounding_box(X, bounding_box(Y, Z)));

    CHECK(X <= BXY);
    CHECK(Y <= BXY);
    CHECK((X.grown(1) <= BXY && Y.grown(1) <= BXY) == (D == 0 || BXY.empty()));
    CHECK(
        eqb(bounding_box(X.grown(abs(x)), Y.grown(abs(x))), BXY.grown(abs(x))));
    CHECK(eqb(bounding_box(X >> x, Y >> x), BXY >> x));
    CHECK(eqb(bounding_box(X * x, Y * x), BXY * x));
    CHECK(
        eqb(bounding_box(X.grown(abs(x)), Y.grown(abs(x))), BXY.grown(abs(x))));

    const B E = bounding_box(bounding_box(X, Y), Z).grown(10);

    CHECK((N & X) == N);
    CHECK((X & N) == N);
    CHECK((E & X) == X);
    CHECK((X & E) == X);
    CHECK((X & Y) == (Y & X));
    CHECK(((X & Y) & Z) == (X & (Y & Z)));

    const B IXY = X & Y;
    CHECK((IXY <= X && IXY <= Y) == true);
    CHECK((IXY.grown(1) <= X && IXY.grown(1) <= Y) == (D == 0 || IXY.empty()));

  } // for iter
}

TEST_CASE("Box<std::ptrdiff_t,0>", "[regions]") {
  test_Box(Box<std::ptrdiff_t, 0>());
}
TEST_CASE("Box<std::ptrdiff_t,1>", "[regions]") {
  test_Box(Box<std::ptrdiff_t, 1>());
}
TEST_CASE("Box<std::ptrdiff_t,2>", "[regions]") {
  test_Box(Box<std::ptrdiff_t, 2>());
}
TEST_CASE("Box<std::ptrdiff_t,3>", "[regions]") {
  test_Box(Box<std::ptrdiff_t, 3>());
}

TEST_CASE("Box<double,0>", "[regions]") { test_Box(Box<double, 0>()); }
TEST_CASE("Box<double,1>", "[regions]") { test_Box(Box<double, 1>()); }
TEST_CASE("Box<double,2>", "[regions]") { test_Box(Box<double, 2>()); }
TEST_CASE("Box<double,3>", "[regions]") { test_Box(Box<double, 3>()); }

TEST_CASE("NDBox<std::ptrdiff_t>(0)", "[regions]") {
  test_Box(NDBox<std::ptrdiff_t>(0));
}
TEST_CASE("NDBox<std::ptrdiff_t>(1)", "[regions]") {
  test_Box(NDBox<std::ptrdiff_t>(1));
}
TEST_CASE("NDBox<std::ptrdiff_t>(2)", "[regions]") {
  test_Box(NDBox<std::ptrdiff_t>(2));
}
TEST_CASE("NDBox<std::ptrdiff_t>(3)", "[regions]") {
  test_Box(NDBox<std::ptrdiff_t>(3));
}

TEST_CASE("NDBox<double>(0)", "[regions]") { test_Box(NDBox<double>(0)); }
TEST_CASE("NDBox<double>(1)", "[regions]") { test_Box(NDBox<double>(1)); }
TEST_CASE("NDBox<double>(2)", "[regions]") { test_Box(NDBox<double>(2)); }
TEST_CASE("NDBox<double>(3)", "[regions]") { test_Box(NDBox<double>(3)); }
