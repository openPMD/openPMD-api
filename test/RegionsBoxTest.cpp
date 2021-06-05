#include <openPMD/regions/Box.hpp>

#include <catch2/catch.hpp>

#include <cstddef>
#include <functional>
#include <limits>
#include <random>
#include <type_traits>

using namespace openPMD::Regions;

template <typename B> void test_Box(const B &b) {
  REQUIRE(b.empty());
  const std::size_t D = b.ndims();
  using T = typename B::value_type;
  const auto p = b.lower();
  typedef std::decay_t<decltype(p)> P;
  const std::equal_to<P> eqp;
  const std::equal_to<B> eqb;
  const std::less<P> ltp;
  const std::less<B> ltb;

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

  for (int iter = 0; iter < 100; ++iter) {

    B N(b);
    REQUIRE(N.ndims() == D);
    REQUIRE(N.empty());
    for (std::size_t d = 0; d < D; ++d)
      REQUIRE(N.lower()[d] >= N.upper()[d]);

    const B X = randb();
    const B Y = randb();
    const B Z = randb();

    const P n = fmap([](auto) { return T(0); }, p);
    REQUIRE(eqp(n + n, n));
    const P x = randp();
    const P y = randp();
    const P z = randp();

    const T a = rand();
    const T b = rand();

    REQUIRE(N.empty());
    if (D > 0) {
      REQUIRE(X.empty() == all(X.shape() == 0));
      REQUIRE(Y.empty() == all(Y.shape() == 0));
      REQUIRE(Z.empty() == all(Z.shape() == 0));
    }

    REQUIRE(X.empty() == (X.size() == 0));
    REQUIRE(Y.empty() == (Y.size() == 0));
    REQUIRE(Z.empty() == (Z.size() == 0));

    if (D > 0) {
      REQUIRE(X.empty() == all(fmap([](auto a, auto b) { return a >= b; },
                                    X.lower(), X.upper())));
      REQUIRE(Y.empty() == all(fmap([](auto a, auto b) { return a >= b; },
                                    Y.lower(), Y.upper())));
      REQUIRE(Z.empty() == all(fmap([](auto a, auto b) { return a >= b; },
                                    Z.lower(), Z.upper())));
    }

    REQUIRE(eqb(N, N));
    REQUIRE(eqb(X, X));
    REQUIRE(!ltb(N, N));
    REQUIRE(!ltb(X, X));
    if (X.empty()) {
      REQUIRE(eqb(N, X));
      REQUIRE(!ltb(N, X));
    } else {
      REQUIRE(!eqb(N, X));
      REQUIRE(ltb(N, X));
    }

    REQUIRE(eqb((X >> x) << x, X));
    REQUIRE(eqb(X >> x, X << -x));
    REQUIRE(eqb(X >> x, X << -x));
    REQUIRE(eqb(X >> (x + y), (X >> x) >> y));

    REQUIRE(eqb((X * x) * y, X * (x * y)));
    REQUIRE(eqb((X >> x) * y, (X * y) >> (x * y)));

    REQUIRE(eqb(X.grown(1), X) == (D == 0 || X.empty()));
    if (all(x >= 0 && y >= 0))
      REQUIRE(eqb(X.grown(x).grown(y), X.grown(x + y)));
    else
      REQUIRE((X.grown(x).grown(y).empty() ||
               eqb(X.grown(x).grown(y), X.grown(x + y))) == true);
    if (all(x >= 0))
      REQUIRE(eqb(X.grown(x).grown(-x), X));
    else
      REQUIRE((X.grown(x).grown(-x).empty() || eqb(X.grown(x).grown(-x), X)) ==
              true);
    REQUIRE(eqb(X.grown(x), X.grown(x, x)));
    REQUIRE(eqb(X.grown(a), X.grown(P::pure(a))));

    REQUIRE(eqb(X.shrunk(x, y), X.grown(-x, -y)));
    REQUIRE(eqb(X.shrunk(x), X.shrunk(x, x)));
    REQUIRE(eqb(X.shrunk(a), X.shrunk(P::pure(a))));

    REQUIRE(N == N);
    REQUIRE(X == X);
    REQUIRE((N == X) == X.empty());
    REQUIRE(!(N != N));
    REQUIRE(!(X != X));
    REQUIRE((N != X) != (N == X));

    REQUIRE(X.contains(X.lower()) == !X.empty());
    REQUIRE(X.contains(X.upper() - 1) == !X.empty());
    REQUIRE(X.grown(1).contains(X.upper()) == !X.empty());
    REQUIRE(isdisjoint(X, X) == X.empty());

    // a <= b means "a implies b" for booleans (yes, it looks wrong)
    REQUIRE((X < Y) <= (X <= Y));
    REQUIRE((X > Y) <= (X >= Y));
    REQUIRE((X <= Y) <= (X.empty() || !isdisjoint(X, Y)));
    REQUIRE((X >= Y) <= (Y.empty() || !isdisjoint(X, Y)));
    REQUIRE(!(X < Y && Y < X));
    REQUIRE((X <= Y && X >= Y) == (X == Y));
    REQUIRE((X < X.grown(1)) == (D > 0 && !X.empty()));
    REQUIRE((X.shrunk(1) < X) == (D > 0 && !X.empty()));

    REQUIRE(N <= N);
    REQUIRE(!(N < N));
    REQUIRE(N <= X);
    REQUIRE((N < X) == !X.empty());

    const auto BXY = bounding_box(X, Y);
    REQUIRE(bounding_box(N, X) == X);
    REQUIRE(bounding_box(X, N) == X);
    REQUIRE(bounding_box(X, Y) == bounding_box(Y, X));
    REQUIRE(bounding_box(bounding_box(X, Y), Z) ==
            bounding_box(X, bounding_box(Y, Z)));

    REQUIRE(X <= BXY);
    REQUIRE(Y <= BXY);
    REQUIRE((X.grown(1) <= BXY && Y.grown(1) <= BXY) ==
            (D == 0 || BXY.empty()));
    REQUIRE(
        eqb(bounding_box(X.grown(abs(x)), Y.grown(abs(x))), BXY.grown(abs(x))));
    REQUIRE(eqb(bounding_box(X >> x, Y >> x), BXY >> x));
    REQUIRE(eqb(bounding_box(X * x, Y * x), BXY * x));
    REQUIRE(
        eqb(bounding_box(X.grown(abs(x)), Y.grown(abs(x))), BXY.grown(abs(x))));

    const B E = bounding_box(bounding_box(X, Y), Z).grown(10);

    REQUIRE((N & X) == N);
    REQUIRE((X & N) == N);
    REQUIRE((E & X) == X);
    REQUIRE((X & E) == X);
    REQUIRE((X & Y) == (Y & X));
    REQUIRE(((X & Y) & Z) == (X & (Y & Z)));

    REQUIRE((N | X) == X);
    REQUIRE((E | X) == E);
    REQUIRE((X | E) == E);
    // REQUIRE((X | Y) == (Y | X));
    // REQUIRE(((X | Y) | Z) == (X | (Y | Z)));

    // REQUIRE(E - (X & Y) == ((E - X) | (E - Y)));
    // REQUIRE(E - (X | Y) == ((E - X) & (E - Y)));

    const B IXY = X & Y;
    REQUIRE((IXY <= X && IXY <= Y) == true);
    REQUIRE((IXY.grown(1) <= X && IXY.grown(1) <= Y) ==
            (D == 0 || IXY.empty()));

    const std::vector<B> UXY = X | Y;
    for (const auto &U : UXY)
      REQUIRE((U <= X || U <= Y) == true);

    const std::vector<B> DXY = X - Y;
    for (const auto &D : DXY)
      REQUIRE((D <= X || !isdisjoint(D, Y)) == true);

    const std::vector<B> SXY = X ^ Y;
    for (const auto &S : SXY)
      REQUIRE(((S <= X || S <= Y) && isdisjoint(S, IXY)) == true);

    // REQUIRE(IXY <= UXY);
    // REQUIRE(isdisjoint(IXY, SXY));
    // REQUIRE((IXY | SXY) == UXY);

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

#warning "TODO"
#if 0
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
#endif
