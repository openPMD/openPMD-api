#include "openPMD/RegionCalculus2.hpp"

#include <catch2/catch.hpp>

#include <cstddef>

using namespace RegionCalculus2;

template <typename T, std::size_t D> void test_point() {
  point<T, D> x;
  REQUIRE(x.size() == D);
  point<T, D> y;
  for (std::size_t d = 0; d < D; ++d)
    y[d] = d;
  point<T, D> z = x + y;
  for (std::size_t d = 0; d < D; ++d)
    REQUIRE(z[d] == d);
  z += y;
  for (std::size_t d = 0; d < D; ++d)
    REQUIRE(z[d] == 2 * d);
}

TEST_CASE("point", "[auxiliary]") {
  test_point<int, 2>();
  test_point<double, 3>();
}

template <typename T> void test_ndpoint(const std::size_t D) {
  ndpoint<T> n;
  REQUIRE(!n);
  ndpoint<T> x(D);
  REQUIRE(x);
  REQUIRE(x.size() == D);
  ndpoint<T> y(D);
  for (std::size_t d = 0; d < D; ++d)
    y[d] = d;
  ndpoint<T> z = x + y;
  for (std::size_t d = 0; d < D; ++d)
    REQUIRE(z[d] == d);
  z += y;
  for (std::size_t d = 0; d < D; ++d)
    REQUIRE(z[d] == 2 * d);
}

TEST_CASE("ndpoint", "[auxiliary]") {
  test_ndpoint<int>(2);
  test_ndpoint<double>(3);
}
