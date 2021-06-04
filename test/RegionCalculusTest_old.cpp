#include "openPMD/RegionCalculus.hpp"

#if 0

#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
#include <functional>
#include <random>
#include <set>
#include <sstream>
#include <vector>

using std::array;
using std::equal_to;
using std::hash;
using std::less;
using std::ostringstream;
using std::set;
using std::vector;

int irand(int imax) {
  static std::mt19937 rng;
  return std::uniform_int_distribution<>(0, imax)(rng);
}

using namespace RegionCalculus;

TEST(RegionCalculus, point) {
  typedef point<int, 3> point;
  point p;
  EXPECT_FALSE(all(p));
  EXPECT_FALSE(any(p));
  point p0(0);
  EXPECT_FALSE(all(p0));
  EXPECT_FALSE(any(p0));
  EXPECT_TRUE(all(p0 == p));
  point p1(::point<bool, 3>(true));
  EXPECT_TRUE(all(p1));
  EXPECT_TRUE(any(p1));
  EXPECT_TRUE(all(p1 != p));
  EXPECT_TRUE(all(p1 == +p1));
  EXPECT_TRUE(all(p0 == -p0));
  EXPECT_TRUE(all(p1 == abs(-p1)));
  EXPECT_TRUE(all(-p1 == ~p0));
  EXPECT_TRUE(all(!p0));
  point p2(point(2));
  EXPECT_TRUE(equal_to<point>()(p2, point(::point<long long, 3>(p2))));
  EXPECT_TRUE(all(p1 + p1 == p2));
  EXPECT_TRUE(all(p2 - p1 == p1));
  EXPECT_TRUE(all(p2 * p1 == p2));
  EXPECT_TRUE(all((p2 & p1) == p0));
  EXPECT_TRUE(all((p2 | p1) == p1 + p2));
  EXPECT_TRUE(all((p2 ^ p1) == p1 + p2));
  EXPECT_TRUE(all(p1 && p2));
  EXPECT_TRUE(all(p1 || p2));
  EXPECT_TRUE(all(p0 <= p1 && p0 < p1 && p1 >= p0 && p1 > p0));
  EXPECT_EQ(2, minval(p2));
  EXPECT_EQ(2, maxval(p2));
  EXPECT_EQ(3, sum(p1));
  EXPECT_EQ(8, prod(p2));
  point p3;
  p3.elt[0] = 0;
  p3.elt[1] = 1;
  p3.elt[2] = 2;
  EXPECT_TRUE(all((p1 + p3) * point(2) - p3 == p3 + p2));
  EXPECT_TRUE(all(p3 == ::point<int, 3>(array<int, 3>{{0, 1, 2}})));
  EXPECT_TRUE(all(p3 == ::point<int, 3>(array<short, 3>{{0, 1, 2}})));
  EXPECT_TRUE(all(p3 == ::point<int, 3>(vector<int>{{0, 1, 2}})));
  EXPECT_TRUE(all(p3 == ::point<int, 3>(vector<short>{{0, 1, 2}})));
  EXPECT_TRUE(all(p2 == ::point<int, 3>(::point<int, 3>(2))));
  EXPECT_TRUE(all(p2 == ::point<int, 3>(::point<short, 3>(2))));
  auto v2a = array<int, 3>{2, 2, 2};
  EXPECT_EQ(v2a, (array<int, 3>(::point<int, 3>(2))));
  EXPECT_EQ(v2a, (array<int, 3>(::point<short, 3>(2))));
  auto v2 = vector<int>({2, 2, 2});
  EXPECT_EQ(v2, vector<int>(::point<int, 3>(2)));
  EXPECT_EQ(v2, vector<int>(::point<short, 3>(2)));
  ostringstream buf;
  buf << p3;
  EXPECT_EQ("[0,1,2]", buf.str());
}

TEST(RegionCalculus, box) {
  typedef point<int, 3> point;
  typedef box<int, 3> box;
  box b;
  EXPECT_TRUE(b.empty());
  point p0(0), p1(1);
  box b1(box(p0, p1));
  EXPECT_EQ(1, b1.size());
  box b4(p0, point(4));
  box b5 = b4 >> p1;
  box b3 = b4 << p1;
  box b6 = b3 * point(2);
  box b7 = b4.grow(p0, p1);
  box b8 = b4.grow(p1, p0);
  EXPECT_EQ(b4, (box(::box<long long, 3>(b4))));
  EXPECT_TRUE(b4 == b4);
  EXPECT_TRUE(b4 != b5);
  EXPECT_TRUE(b4.contains(p1));
  EXPECT_FALSE(b5.contains(p0));
  EXPECT_FALSE(b5.isdisjoint(b3));
  EXPECT_TRUE(b1.isdisjoint(b5));
  EXPECT_TRUE(b.isdisjoint(b5));
  EXPECT_TRUE(b1.isdisjoint(b));
  EXPECT_TRUE(b6.issuperset(b3));
  EXPECT_FALSE(b3.issuperset(b6));
  EXPECT_TRUE(b4.issuperset(b4));
  EXPECT_TRUE(b4.issuperset(b));
  EXPECT_FALSE(b.issuperset(b4));
  EXPECT_TRUE(b.issuperset(b));
  EXPECT_TRUE(b7.issuperset(b4));
  EXPECT_TRUE(b7.issuperset(b5));
  EXPECT_TRUE(b8.issuperset(b4));
  EXPECT_TRUE(b8.issuperset(b3));
  EXPECT_FALSE(b4.is_strict_superset(b4));
  EXPECT_TRUE(b4.is_strict_superset(b));
  EXPECT_FALSE(b.is_strict_superset(b4));
  EXPECT_FALSE(b.is_strict_superset(b));
  EXPECT_EQ(box(p1, point(3)), b5.intersection(b3));
  EXPECT_TRUE(b1.intersection(b5).empty());
  EXPECT_TRUE(b.intersection(b5).empty());
  EXPECT_TRUE(b1.intersection(b).empty());
  ostringstream buf;
  buf << b4;
  EXPECT_EQ("([0,0,0]:[4,4,4])", buf.str());
}

#if !REGIONCALCULUS_TREE

TEST(RegionCalculus, region1) {
  typedef point<int, 3> point;
  typedef box<int, 3> box;
  typedef region<int, 3> region;
  region r;
  EXPECT_TRUE(r.invariant());
  EXPECT_TRUE(r.empty());
  point p;
  point p1(1);
  box b;
  box b1(p, p1);
  region r0(b);
  EXPECT_TRUE(r0.invariant());
  EXPECT_TRUE(r0.empty());
  region r1(b1);
  EXPECT_TRUE(r1.invariant());
  EXPECT_FALSE(r1.empty());
  EXPECT_TRUE(r == r);
  EXPECT_TRUE(r1 == r1);
  EXPECT_FALSE(r != r);
  EXPECT_TRUE(r != r1);
  point p2(2);
  box b2(p, p2);
  region r2(b2);
  EXPECT_EQ(r2, (region(::region<long long, 3>(r2))));
  EXPECT_TRUE(r == r.intersection(r1));
  EXPECT_TRUE(r == r1.intersection(r));
  EXPECT_TRUE(r1 == r1.intersection(r2));
  EXPECT_TRUE(r1 == r2.intersection(r1));
  EXPECT_TRUE(r == r.difference(r));
  EXPECT_TRUE(r1 == r1.difference(r));
  EXPECT_TRUE(r == r.difference(r1));
  EXPECT_TRUE(r == r1.difference(r1));
  EXPECT_TRUE(r == r.setunion(r));
  EXPECT_TRUE(r1 == r1.setunion(r));
  EXPECT_TRUE(r1 == r.setunion(r1));
  EXPECT_TRUE(r1 == r1.setunion(r1));
  EXPECT_TRUE(r == r.symmetric_difference(r));
  EXPECT_TRUE(r1 == r1.symmetric_difference(r));
  EXPECT_TRUE(r1 == r.symmetric_difference(r1));
  EXPECT_TRUE(r == r1.symmetric_difference(r1));
  vector<box> r12vals;
  r12vals.push_back(b1);
  r12vals.push_back(box(p1, p2));
  region r12(r12vals);
  vector<box> r12boxes = r12;
  EXPECT_EQ(r12vals, r12boxes);
  vector<region> rs;
  rs.push_back(r);
  rs.push_back(r1);
  rs.push_back(r2);
  box b4(p, point(4));
  region r4(b4);
  rs.push_back(r4);
  rs.push_back(r12);
  rs.push_back(r2.difference(r1));
  rs.push_back(r2.symmetric_difference(r12));
  for (std::size_t i = 0; i < rs.size(); ++i) {
    const auto &ri = rs[i];
    auto rgrown = ri.grow(p1);
    auto rshrunk = ri.shrink(p1);
    EXPECT_TRUE(ri.invariant());
    EXPECT_TRUE(rgrown.invariant());
    EXPECT_TRUE(rshrunk.invariant());
    EXPECT_TRUE(rgrown.issuperset(ri));
    if (ri.empty())
      EXPECT_TRUE(rgrown.empty());
    EXPECT_TRUE(ri.issuperset(rshrunk));
    if (!rshrunk.empty())
      EXPECT_TRUE(rshrunk.grow(p1) == ri);
    EXPECT_TRUE(rgrown.shrink(p1) == ri);
    for (std::size_t j = 0; j < rs.size(); ++j) {
      const auto &rj = rs[j];
      auto rintersection = ri.intersection(rj);
      auto rdifference = ri.difference(rj);
      auto rsetunion = ri.setunion(rj);
      auto rsymmetric_difference = ri.symmetric_difference(rj);
      EXPECT_TRUE(rintersection.invariant());
      EXPECT_TRUE(rdifference.invariant());
      EXPECT_TRUE(rsetunion.invariant());
      EXPECT_TRUE(rsymmetric_difference.invariant());
      EXPECT_TRUE(ri.issuperset(rintersection));
      EXPECT_TRUE(ri.issuperset(rintersection));
      EXPECT_TRUE(rj.issuperset(rintersection));
      EXPECT_TRUE(ri.issuperset(rdifference));
      EXPECT_TRUE(rsetunion.issuperset(ri));
      EXPECT_TRUE(rsetunion.issuperset(rj));
      EXPECT_TRUE(rintersection.isdisjoint(rsymmetric_difference));
      EXPECT_TRUE(rdifference.isdisjoint(rj));
      EXPECT_TRUE(rsetunion.issuperset(rintersection));
      EXPECT_TRUE(rsetunion.issuperset(rdifference));
      EXPECT_TRUE(rsetunion.issuperset(rsymmetric_difference));
      EXPECT_TRUE(rintersection.setunion(rsymmetric_difference) == rsetunion);
      EXPECT_TRUE(rdifference.setunion(rj) == rsetunion);
      EXPECT_TRUE(rintersection == rj.intersection(ri));
      EXPECT_TRUE(rsetunion == rj.setunion(ri));
      EXPECT_TRUE(rsymmetric_difference == rj.symmetric_difference(ri));
      if (ri == rj) {
        EXPECT_TRUE(rintersection == ri);
        EXPECT_TRUE(rdifference.empty());
        EXPECT_TRUE(rsetunion == ri);
        EXPECT_TRUE(rsymmetric_difference.empty());
      }
    }
  }
  ostringstream buf;
  buf << r12;
  EXPECT_EQ("{([0,0,0]:[1,1,1]),([1,1,1]:[2,2,2])}", buf.str());
}

#else // #if REGIONCALCULUS_TREE

TEST(RegionCalculus, region) {
  typedef point<int, 3> point;
  typedef box<int, 3> box;
  typedef region<int, 3> region;
  region r;
  EXPECT_TRUE(r.invariant());
  EXPECT_TRUE(r.empty());
  point p;
  point p1(1);
  box b;
  box b1(p, p1);
  region r0(b);
  EXPECT_TRUE(r0.invariant());
  EXPECT_TRUE(r0.empty());
  region r1(b1);
  EXPECT_TRUE(r1.invariant());
  EXPECT_FALSE(r1.empty());
  EXPECT_TRUE(r == r);
  EXPECT_TRUE(r1 == r1);
  EXPECT_FALSE(r != r);
  EXPECT_TRUE(r != r1);
  point p2(2);
  box b2(p, p2);
  region r2(b2);
  EXPECT_EQ(r2, region(::region<long long, 3>(r2)));
  EXPECT_TRUE(r == r.intersection(r1));
  EXPECT_TRUE(r == r1.intersection(r));
  EXPECT_TRUE(r1 == r1.intersection(r2));
  EXPECT_TRUE(r1 == r2.intersection(r1));
  EXPECT_TRUE(r == r.difference(r));
  EXPECT_TRUE(r1 == r1.difference(r));
  EXPECT_TRUE(r == r.difference(r1));
  EXPECT_TRUE(r == r1.difference(r1));
  EXPECT_TRUE(r == r.setunion(r));
  EXPECT_TRUE(r1 == r1.setunion(r));
  EXPECT_TRUE(r1 == r.setunion(r1));
  EXPECT_TRUE(r1 == r1.setunion(r1));
  EXPECT_TRUE(r == r.symmetric_difference(r));
  EXPECT_TRUE(r1 == r1.symmetric_difference(r));
  EXPECT_TRUE(r1 == r.symmetric_difference(r1));
  EXPECT_TRUE(r == r1.symmetric_difference(r1));
  EXPECT_EQ(0, vector<box>(r).size());
  EXPECT_EQ(1, vector<box>(r1).size());
  EXPECT_EQ(1, vector<box>(r2).size());
  auto r21 = r2 - r1;
  vector<box> r21boxes(r21);
  EXPECT_EQ(3, r21boxes.size());
  EXPECT_EQ(box(point(1, 0, 0), point(2, 1, 1)), r21boxes.at(0));
  EXPECT_EQ(box(point(0, 1, 0), point(2, 2, 1)), r21boxes.at(1));
  EXPECT_EQ(box(point(0, 0, 1), point(2, 2, 2)), r21boxes.at(2));
  vector<box> r12vals;
  r12vals.push_back(b1);
  r12vals.push_back(box(p1, p2));
  region r12(r12vals);
  vector<box> r12boxes = r12;
  EXPECT_EQ(r12vals, r12boxes);
  vector<region> rs;
  rs.push_back(r);
  rs.push_back(r1);
  rs.push_back(r2);
  box b4(p, point(4));
  region r4(b4);
  rs.push_back(r4);
  rs.push_back(r12);
  rs.push_back(r2.difference(r1));
  rs.push_back(r2.symmetric_difference(r12));
  for (int i = 0; i < 10; ++i) {
    vector<box> bs;
    for (int n = irand(5); n >= 0; --n)
      bs.push_back(box(point(irand(10), irand(10), irand(10)),
                       point(irand(10), irand(10), irand(10))));
    rs.push_back(region(bs));
  }
  for (std::size_t i = 0; i < rs.size(); ++i) {
    const auto &ri = rs[i];
    EXPECT_TRUE(ri.invariant());
    const auto ri_size = ri.size();
    for (std::size_t j = 0; j < rs.size(); ++j) {
      const auto &rj = rs[j];
      const auto rj_size = rj.size();
      auto rintersection = ri.intersection(rj);
      auto rdifference = ri.difference(rj);
      auto rsetunion = ri.setunion(rj);
      auto rsymmetric_difference = ri.symmetric_difference(rj);
      auto rintersection_size = rintersection.size();
      auto rdifference_size = rdifference.size();
      auto rsetunion_size = rsetunion.size();
      auto rsymmetric_difference_size = rsymmetric_difference.size();
      EXPECT_TRUE(rintersection.invariant());
      EXPECT_TRUE(rdifference.invariant());
      EXPECT_TRUE(rsetunion.invariant());
      EXPECT_TRUE(rsymmetric_difference.invariant());
      EXPECT_TRUE(rintersection_size <= ri_size &&
                  rintersection_size <= rj_size);
      EXPECT_TRUE(rdifference_size <= ri_size &&
                  rdifference_size >= ri_size - rj_size);
      EXPECT_TRUE(rsetunion_size >= ri_size && rsetunion_size >= rj_size &&
                  rsetunion_size <= ri_size + rj_size);
      EXPECT_TRUE(rsymmetric_difference_size <= ri_size + rj_size &&
                  rsymmetric_difference_size >= abs(ri_size - rj_size));
      EXPECT_TRUE(ri.issuperset(rintersection));
      EXPECT_TRUE(rj.issuperset(rintersection));
      EXPECT_TRUE(ri.issuperset(rdifference));
      EXPECT_TRUE(rsetunion.issuperset(ri));
      EXPECT_TRUE(rsetunion.issuperset(rj));
      EXPECT_TRUE(rintersection.isdisjoint(rsymmetric_difference));
      EXPECT_TRUE(rdifference.isdisjoint(rj));
      EXPECT_TRUE(rsetunion.issuperset(rintersection));
      EXPECT_TRUE(rsetunion.issuperset(rdifference));
      EXPECT_TRUE(rsetunion.issuperset(rsymmetric_difference));
      EXPECT_TRUE(rintersection.setunion(rsymmetric_difference) == rsetunion);
      EXPECT_TRUE(rdifference.setunion(rj) == rsetunion);
      EXPECT_TRUE(rintersection == rj.intersection(ri));
      EXPECT_TRUE(rsetunion == rj.setunion(ri));
      EXPECT_TRUE(rsymmetric_difference == rj.symmetric_difference(ri));
      for (int n = 0; n < 10; ++n) {
        int i = irand(10), j = irand(10), k = irand(10);
        point p(i, j, k);
        EXPECT_EQ(ri.contains(p) & rj.contains(p), rintersection.contains(p));
        EXPECT_EQ(ri.contains(p) & !rj.contains(p), rdifference.contains(p));
        EXPECT_EQ(ri.contains(p) | rj.contains(p), rsetunion.contains(p));
        EXPECT_EQ(ri.contains(p) ^ rj.contains(p),
                  rsymmetric_difference.contains(p));
      }
      if (ri == rj) {
        EXPECT_TRUE(rintersection == ri);
        EXPECT_TRUE(rdifference.empty());
        EXPECT_TRUE(rsetunion == ri);
        EXPECT_TRUE(rsymmetric_difference.empty());
      }
    }
  }
  ostringstream buf;
  buf << r12;
  EXPECT_EQ("{([0,0,0]:[1,1,1]),([1,1,1]:[2,2,2])}", buf.str());
}

#endif // #if REGIONCALCULUS_TREE

TEST(RegionCalculus, region2) {
  typedef point<int, 3> point;
  typedef box<int, 3> box;
  typedef region<int, 3> region;
  region r;
  EXPECT_TRUE(r.invariant());
  EXPECT_TRUE(r.empty());
  point p;
  point p1(1);
  box b;
  box b1(p, p1);
  region r0(b);
  EXPECT_TRUE(r0.invariant());
  EXPECT_TRUE(r0.empty());
  region r1(b1);
  EXPECT_TRUE(r1.invariant());
  EXPECT_FALSE(r1.empty());
  EXPECT_TRUE(r == r);
  EXPECT_TRUE(r1 == r1);
  EXPECT_FALSE(r != r);
  EXPECT_TRUE(r != r1);
  point p2(2);
  box b2(p, p2);
  region r2(b2);
  EXPECT_EQ(r2, (region(::region<long long, 3>(r2))));
  EXPECT_TRUE(r == r.intersection(r1));
  EXPECT_TRUE(r == r1.intersection(r));
  EXPECT_TRUE(r1 == r1.intersection(r2));
  EXPECT_TRUE(r1 == r2.intersection(r1));
  EXPECT_TRUE(r == r.difference(r));
  EXPECT_TRUE(r1 == r1.difference(r));
  EXPECT_TRUE(r == r.difference(r1));
  EXPECT_TRUE(r == r1.difference(r1));
  EXPECT_TRUE(r == r.setunion(r));
  EXPECT_TRUE(r1 == r1.setunion(r));
  EXPECT_TRUE(r1 == r.setunion(r1));
  EXPECT_TRUE(r1 == r1.setunion(r1));
  EXPECT_TRUE(r == r.symmetric_difference(r));
  EXPECT_TRUE(r1 == r1.symmetric_difference(r));
  EXPECT_TRUE(r1 == r.symmetric_difference(r1));
  EXPECT_TRUE(r == r1.symmetric_difference(r1));
  vector<box> r12vals;
  r12vals.push_back(b1);
  r12vals.push_back(box(p1, p2));
  region r12(r12vals);
  vector<box> r12boxes = r12;
  EXPECT_EQ(r12vals, r12boxes);
  vector<region> rs;
  rs.push_back(r);
  rs.push_back(r1);
  rs.push_back(r2);
  box b4(p, point(4));
  region r4(b4);
  rs.push_back(r4);
  rs.push_back(r12);
  rs.push_back(r2.difference(r1));
  rs.push_back(r2.symmetric_difference(r12));
  for (std::size_t i = 0; i < rs.size(); ++i) {
    const auto &ri = rs[i];
    auto rgrown = ri.grow(p1);
    auto rshrunk = ri.shrink(p1);
    EXPECT_TRUE(ri.invariant());
    EXPECT_TRUE(rgrown.invariant());
    EXPECT_TRUE(rshrunk.invariant());
    EXPECT_TRUE(rgrown.issuperset(ri));
    if (ri.empty())
      EXPECT_TRUE(rgrown.empty());
    region rgrown_test;
    for (int dk = -1; dk <= +1; ++dk)
      for (int dj = -1; dj <= +1; ++dj)
        for (int di = -1; di <= +1; ++di) {
          auto shifted = ri >> point(di, dj, dk);
          rgrown_test |= shifted;
        }
    EXPECT_TRUE(rgrown_test == rgrown);
    EXPECT_TRUE(ri.issuperset(rshrunk));
    if (!rshrunk.empty())
      EXPECT_TRUE(rshrunk.grow(p1) == ri);
    region rshrunk_test = ri;
    for (int dk = -1; dk <= +1; ++dk)
      for (int dj = -1; dj <= +1; ++dj)
        for (int di = -1; di <= +1; ++di) {
          auto shifted = ri >> point(di, dj, dk);
          rshrunk_test &= shifted;
        }
    EXPECT_TRUE(rshrunk_test == rshrunk);
    region rgrown_shrunk_test = rgrown;
    for (int dk = -1; dk <= +1; ++dk)
      for (int dj = -1; dj <= +1; ++dj)
        for (int di = -1; di <= +1; ++di) {
          auto shifted = rgrown >> point(di, dj, dk);
          rgrown_shrunk_test &= shifted;
        }
    EXPECT_TRUE(rgrown_shrunk_test == ri);
    EXPECT_TRUE(rgrown.shrink(p1) == ri);
    for (std::size_t j = 0; j < rs.size(); ++j) {
      const auto &rj = rs[j];
      auto rintersection = ri.intersection(rj);
      auto rdifference = ri.difference(rj);
      auto rsetunion = ri.setunion(rj);
      auto rsymmetric_difference = ri.symmetric_difference(rj);
      EXPECT_TRUE(rintersection.invariant());
      EXPECT_TRUE(rdifference.invariant());
      EXPECT_TRUE(rsetunion.invariant());
      EXPECT_TRUE(rsymmetric_difference.invariant());
      EXPECT_TRUE(ri.issuperset(rintersection));
      EXPECT_TRUE(ri.issuperset(rintersection));
      EXPECT_TRUE(rj.issuperset(rintersection));
      EXPECT_TRUE(ri.issuperset(rdifference));
      EXPECT_TRUE(rsetunion.issuperset(ri));
      EXPECT_TRUE(rsetunion.issuperset(rj));
      EXPECT_TRUE(rintersection.isdisjoint(rsymmetric_difference));
      EXPECT_TRUE(rdifference.isdisjoint(rj));
      EXPECT_TRUE(rsetunion.issuperset(rintersection));
      EXPECT_TRUE(rsetunion.issuperset(rdifference));
      EXPECT_TRUE(rsetunion.issuperset(rsymmetric_difference));
      EXPECT_TRUE(rintersection.setunion(rsymmetric_difference) == rsetunion);
      EXPECT_TRUE(rdifference.setunion(rj) == rsetunion);
      EXPECT_TRUE(rintersection == rj.intersection(ri));
      EXPECT_TRUE(rsetunion == rj.setunion(ri));
      EXPECT_TRUE(rsymmetric_difference == rj.symmetric_difference(ri));
      if (ri == rj) {
        EXPECT_TRUE(rintersection == ri);
        EXPECT_TRUE(rdifference.empty());
        EXPECT_TRUE(rsetunion == ri);
        EXPECT_TRUE(rsymmetric_difference.empty());
      }
      EXPECT_TRUE(rintersection == (ri & rj));
      auto rintersection1 = ri;
      rintersection1 &= rj;
      EXPECT_TRUE(rintersection == rintersection1);
      EXPECT_TRUE(rdifference == (ri - rj));
      auto rdifference1 = ri;
      rdifference1 -= rj;
      EXPECT_TRUE(rdifference == rdifference1);
      EXPECT_TRUE(rsetunion == (ri | rj));
      auto rsetunion1 = ri;
      rsetunion1 |= rj;
      EXPECT_TRUE(rsetunion == rsetunion1);
      EXPECT_TRUE(rsymmetric_difference == (ri ^ rj));
      auto rsymmetric_difference1 = ri;
      rsymmetric_difference1 ^= rj;
      EXPECT_TRUE(rsymmetric_difference == rsymmetric_difference1);
    }
  }
  ostringstream buf;
  buf << r12;
  EXPECT_EQ("{([0,0,0]:[1,1,1]),([1,1,1]:[2,2,2])}", buf.str());
}

// Get random set element
template <typename T> T getelt(const set<T> &xs) {
  assert(!xs.empty());
  int n = irand(xs.size() - 1);
  auto p = xs.begin();
  for (int i = 0; i < n; ++i)
    ++p;
  return *p;
}

template <int D> void test_point() {
  typedef point<int, D> P;

  // Create a few points
  set<P> ps;
  while (ps.size() < 10) {
    array<int, D> xs;
    for (int d = 0; d < D; ++d)
      xs[d] = irand(10);
    P p(xs);
    ps.insert(p);
  }

  const int niters = 100;
  for (int n = 0; n < niters; ++n) {
    auto p1 = getelt(ps);
    auto p2 = getelt(ps);
    auto p3 = getelt(ps);
    std::equal_to<P> eq;
    std::less<P> lt;
    std::hash<P> h;
    auto eq1 = eq(p1, p2);
    auto eq2 = eq(p2, p1);
    auto eq3 = eq(p2, p3);
    auto eq4 = eq(p1, p3);
    auto lt1 = lt(p1, p2);
    auto lt2 = lt(p2, p1);
    auto lt3 = lt(p2, p3);
    auto lt4 = lt(p1, p3);
    EXPECT_TRUE(eq1 == eq2); // eq is reflexive
    EXPECT_FALSE(eq1 && lt1);
    EXPECT_FALSE(eq1 && lt2);
    EXPECT_FALSE(lt1 && lt2); // lt is irreflexive
    EXPECT_EQ(1, eq1 + lt1 + lt2);
    if (eq1 && eq3)
      EXPECT_TRUE(eq4); // eq is transitive
    if (lt1 && lt3)
      EXPECT_TRUE(lt4); // lt is transitive
    if (eq1)
      EXPECT_EQ(h(p1), h(p2));
    if (eq2)
      EXPECT_EQ(h(p2), h(p1));
    if (eq3)
      EXPECT_EQ(h(p2), h(p3));
    if (eq4)
      EXPECT_EQ(h(p1), h(p3));
    if (h(p1) != h(p2))
      EXPECT_FALSE(eq1);
    if (h(p2) != h(p1))
      EXPECT_FALSE(eq2);
    if (h(p2) != h(p3))
      EXPECT_FALSE(eq3);
    if (h(p1) != h(p3))
      EXPECT_FALSE(eq4);
  }
}

template <int D> void test_box() {
  typedef point<int, D> P;
  typedef box<int, D> B;

  // Create a few boxes
  set<B> bs;
  bs.insert(B());
  while (bs.size() < 10) {
    array<int, D> xs, ys;
    for (int d = 0; d < D; ++d) {
      xs[d] = irand(10);
      ys[d] = irand(10);
    }
    P px(xs), py(ys);
    B b{xs, ys};
    bs.insert(b);
  }

  const int niters = 100;
  for (int n = 0; n < niters; ++n) {
    const auto b1 = getelt(bs);
    const auto b2 = getelt(bs);
    const auto bint = b1 & b2;
    const auto bbox = b1.bounding_box(b2);
    EXPECT_TRUE(bint <= b1);
    EXPECT_TRUE(bint <= b2);
    EXPECT_TRUE(b1 <= bbox);
    EXPECT_TRUE(b2 <= bbox);
    EXPECT_TRUE(bint <= bbox);
    bs.insert(bint);
    bs.insert(bbox);

    array<int, D> xs, ys;
    for (int d = 0; d < D; ++d) {
      xs[d] = irand(5) - 2;
      ys[d] = irand(5) - 2;
    }
    P plo(xs);
    P phi(ys);
    const auto bshl = b1 << plo;
    const auto bshr = b2 >> phi;
    const auto bgro = b1.grow(plo, phi);
    const auto bred = b2.shrink(plo, phi);
    EXPECT_TRUE((bshl >> plo) == b1);
    EXPECT_TRUE((bshr << phi) == b2);
    if (all(plo + phi >= P(0))) {
      EXPECT_TRUE(bgro.size() >= b1.size());
      EXPECT_TRUE(bred.size() <= b2.size());
      EXPECT_TRUE(bgro.shrink(plo, phi) >= b1);
      EXPECT_TRUE(bred.grow(plo, phi) <= b2);
    }
    bs.insert(bshl);
    bs.insert(bshr);
    bs.insert(bgro);
    bs.insert(bred);
  }

  for (int n = 0; n < niters; ++n) {
    auto b1 = getelt(bs);
    auto b2 = getelt(bs);
    auto b3 = getelt(bs);
    std::equal_to<B> eq;
    std::less<B> lt;
    std::hash<B> h;
    auto eq1 = eq(b1, b2);
    auto eq2 = eq(b2, b1);
    auto eq3 = eq(b2, b3);
    auto eq4 = eq(b1, b3);
    auto lt1 = lt(b1, b2);
    auto lt2 = lt(b2, b1);
    auto lt3 = lt(b2, b3);
    auto lt4 = lt(b1, b3);
    EXPECT_TRUE(eq1 == eq2); // eq is reflexive
    EXPECT_FALSE(eq1 && lt1);
    EXPECT_FALSE(eq1 && lt2);
    EXPECT_FALSE(lt1 && lt2); // lt is irreflexive
    EXPECT_EQ(1, eq1 + lt1 + lt2);
    if (eq1 && eq3)
      EXPECT_TRUE(eq4); // eq is transitive
    if (lt1 && lt3)
      EXPECT_TRUE(lt4); // lt is transitive
    if (eq1)
      EXPECT_EQ(h(b1), h(b2));
    if (eq2)
      EXPECT_EQ(h(b2), h(b1));
    if (eq3)
      EXPECT_EQ(h(b2), h(b3));
    if (eq4)
      EXPECT_EQ(h(b1), h(b3));
    if (h(b1) != h(b2))
      EXPECT_FALSE(eq1);
    if (h(b2) != h(b1))
      EXPECT_FALSE(eq2);
    if (h(b2) != h(b3))
      EXPECT_FALSE(eq3);
    if (h(b1) != h(b3))
      EXPECT_FALSE(eq4);
  }
}

template <int D> void test_region() {
  typedef point<int, D> P;
  typedef box<int, D> B;
  typedef region<int, D> R;

  // Create a few regions
  set<R> rs;
  rs.insert(R());
  while (rs.size() < 10) {
    array<int, D> xs, ys;
    for (int d = 0; d < D; ++d) {
      xs[d] = irand(10);
      ys[d] = irand(10);
    }
    P px(xs), py(ys);
    B b{xs, ys};
    R r(b);
    rs.insert(r);
  }

  // Perform a few operations
  const auto getelt = [](const set<R> &rs) {
    assert(!rs.empty());
    int n = irand(rs.size() - 1);
    auto p = rs.begin();
    for (int i = 0; i < n; ++i)
      ++p;
    return *p;
  };

  const int niters = 1000 >> (2 * (D - 1));
  for (int n = 0; n < niters; ++n) {
    const auto r1 = getelt(rs);
    const auto r2 = getelt(rs);
    const auto rint = r1 & r2;
    const auto runi = r1 | r2;
    const auto rsym = r1 ^ r2;
    const auto rdif = r1 - r2;
    EXPECT_TRUE(rint <= r1);
    EXPECT_TRUE(rint <= r2);
    EXPECT_TRUE(r1 <= runi);
    EXPECT_TRUE(r2 <= runi);
    EXPECT_TRUE(rint <= runi);
    EXPECT_TRUE(rsym <= runi);
    EXPECT_TRUE(rsym.isdisjoint(rint));
    EXPECT_TRUE(rdif <= r1);
    EXPECT_TRUE(rdif.isdisjoint(r2));
    EXPECT_TRUE((rsym | rint) == runi);
    rs.insert(rint);
    rs.insert(runi);
    rs.insert(rsym);
    rs.insert(rdif);

    P plo, phi;
    while (1) {
      array<int, D> xs, ys;
      for (int d = 0; d < D; ++d) {
        xs[d] = irand(5) - 2;
        ys[d] = irand(5) - 2;
      }
      plo = P(xs);
      phi = P(ys);
      if (all(plo + phi >= P(0)))
        break;
    }
    const auto rshl = r1 << plo;
    const auto rshr = r2 >> phi;
    const auto rgro = r1.grow(plo, phi);
    const auto rred = r2.shrink(plo, phi);
    EXPECT_TRUE((rshl >> plo) == r1);
    EXPECT_TRUE((rshr << phi) == r2);
    EXPECT_TRUE(rgro.size() >= r1.size());
    EXPECT_TRUE(rred.size() <= r2.size());
    EXPECT_TRUE(rgro.shrink(plo, phi) >= r1);
    EXPECT_TRUE(rred.grow(plo, phi) <= r2);
    rs.insert(rshl);
    rs.insert(rshr);
    rs.insert(rgro);
    rs.insert(rred);
  }

  for (int n = 0; n < niters; ++n) {
    auto r1 = getelt(rs);
    auto r2 = getelt(rs);
    auto r3 = getelt(rs);
    std::equal_to<R> eq;
    std::less<R> lt;
    std::hash<R> h;
    auto eq1 = eq(r1, r2);
    auto eq2 = eq(r2, r1);
    auto eq3 = eq(r2, r3);
    auto eq4 = eq(r1, r3);
    auto lt1 = lt(r1, r2);
    auto lt2 = lt(r2, r1);
    auto lt3 = lt(r2, r3);
    auto lt4 = lt(r1, r3);
    EXPECT_TRUE(eq1 == eq2); // eq is reflexive
    EXPECT_FALSE(eq1 && lt1);
    EXPECT_FALSE(eq1 && lt2);
    EXPECT_FALSE(lt1 && lt2); // lt is irreflexive
    EXPECT_EQ(1, eq1 + lt1 + lt2);
    if (eq1 && eq3)
      EXPECT_TRUE(eq4); // eq is transitive
    if (lt1 && lt3)
      EXPECT_TRUE(lt4); // lt is transitive
    if (eq1)
      EXPECT_EQ(h(r1), h(r2));
    if (eq2)
      EXPECT_EQ(h(r2), h(r1));
    if (eq3)
      EXPECT_EQ(h(r2), h(r3));
    if (eq4)
      EXPECT_EQ(h(r1), h(r3));
    if (h(r1) != h(r2))
      EXPECT_FALSE(eq1);
    if (h(r2) != h(r1))
      EXPECT_FALSE(eq2);
    if (h(r2) != h(r3))
      EXPECT_FALSE(eq3);
    if (h(r1) != h(r3))
      EXPECT_FALSE(eq4);
  }
}

TEST(RegionCalculus, point_1d) { test_point<1>(); }
TEST(RegionCalculus, point_2d) { test_point<2>(); }
TEST(RegionCalculus, point_3d) { test_point<3>(); }
TEST(RegionCalculus, point_4d) { test_point<4>(); }

TEST(RegionCalculus, box_1d) { test_box<1>(); }
TEST(RegionCalculus, box_2d) { test_box<2>(); }
TEST(RegionCalculus, box_3d) { test_box<3>(); }
TEST(RegionCalculus, box_4d) { test_box<4>(); }

TEST(RegionCalculus, region_1d) { test_region<1>(); }
TEST(RegionCalculus, region_2d) { test_region<2>(); }
TEST(RegionCalculus, region_3d) { test_region<3>(); }
TEST(RegionCalculus, region_4d) { test_region<4>(); }

TEST(RegionCalculus, dpoint) {
  const int dim = 3;
  typedef dpoint<int> dpoint;
  dpoint p(dim);
  EXPECT_FALSE(all(p));
  EXPECT_FALSE(any(p));
  dpoint p0(dim, 0);
  EXPECT_FALSE(all(p0));
  EXPECT_FALSE(any(p0));
  EXPECT_TRUE(all(p0 == p));
  dpoint p1(::dpoint<bool>(dim, true));
  EXPECT_TRUE(all(p1));
  EXPECT_TRUE(any(p1));
  EXPECT_TRUE(all(p1 != p));
  EXPECT_TRUE(all(p1 == +p1));
  EXPECT_TRUE(all(p0 == -p0));
  EXPECT_TRUE(all(p1 == abs(-p1)));
  EXPECT_TRUE(all(-p1 == ~p0));
  EXPECT_TRUE(all(!p0));
  dpoint p2(dpoint(dim, 2));
  EXPECT_TRUE(equal_to<dpoint>()(p2, dpoint(::dpoint<long long>(p2))));
  EXPECT_TRUE(all(p1 + p1 == p2));
  EXPECT_TRUE(all(p2 - p1 == p1));
  EXPECT_TRUE(all(p2 * p1 == p2));
  EXPECT_TRUE(all((p2 & p1) == p0));
  EXPECT_TRUE(all((p2 | p1) == p1 + p2));
  EXPECT_TRUE(all((p2 ^ p1) == p1 + p2));
  EXPECT_TRUE(all(p1 && p2));
  EXPECT_TRUE(all(p1 || p2));
  EXPECT_TRUE(all(p0 <= p1 && p0 < p1 && p1 >= p0 && p1 > p0));
  EXPECT_EQ(2, minval(p2));
  EXPECT_EQ(2, maxval(p2));
  EXPECT_EQ(3, sum(p1));
  EXPECT_EQ(8, prod(p2));
  vector<int> p3vals(dim);
  p3vals[0] = 0;
  p3vals[1] = 1;
  p3vals[2] = 2;
  dpoint p3(p3vals);
  EXPECT_TRUE(all((p1 + p3) * dpoint(dim, 2) - p3 == p3 + p2));
  EXPECT_TRUE(all(p3 == ::dpoint<int>(vector<int>{{0, 1, 2}})));
  EXPECT_TRUE(all(p3 == ::dpoint<int>(vector<short>{{0, 1, 2}})));
  EXPECT_TRUE(all(p3 == ::dpoint<int>(array<int, 3>{{0, 1, 2}})));
  EXPECT_TRUE(all(p3 == ::dpoint<int>(array<short, 3>{{0, 1, 2}})));
  EXPECT_TRUE(all(p2 == ::dpoint<int>(::dpoint<int>(3, 2))));
  EXPECT_TRUE(all(p2 == ::dpoint<int>(::dpoint<short>(3, 2))));
  auto v2 = vector<int>({2, 2, 2});
  EXPECT_EQ(v2, vector<int>(::dpoint<int>(3, 2)));
  EXPECT_EQ(v2, vector<int>(::dpoint<short>(3, 2)));
  ostringstream buf;
  buf << p3;
  EXPECT_EQ("[0,1,2]", buf.str());
}

TEST(RegionCalculus, dbox) {
  const int dim = 3;
  typedef dpoint<int> dpoint;
  typedef dbox<int> dbox;
  dbox b(dim);
  EXPECT_TRUE(b.empty());
  dpoint p0(dim, 0), p1(dim, 1);
  dbox b1(dbox(p0, p1));
  EXPECT_EQ(1, b1.size());
  dbox b4(p0, dpoint(dim, 4));
  dbox b5 = b4 >> p1;
  dbox b3 = b4 << p1;
  dbox b6 = b3 * dpoint(dim, 2);
  EXPECT_EQ(b4, dbox(::dbox<long long>(b4)));
  EXPECT_TRUE(b4 == b4);
  EXPECT_TRUE(b4 != b5);
  EXPECT_TRUE(b4.contains(p1));
  EXPECT_FALSE(b5.contains(p0));
  EXPECT_FALSE(b5.isdisjoint(b3));
  EXPECT_TRUE(b1.isdisjoint(b5));
  EXPECT_TRUE(b.isdisjoint(b5));
  EXPECT_TRUE(b1.isdisjoint(b));
  EXPECT_TRUE(b6.issuperset(b3));
  EXPECT_FALSE(b3.issuperset(b6));
  EXPECT_TRUE(b4.issuperset(b4));
  EXPECT_TRUE(b4.issuperset(b));
  EXPECT_FALSE(b.issuperset(b4));
  EXPECT_TRUE(b.issuperset(b));
  EXPECT_FALSE(b4.is_strict_superset(b4));
  EXPECT_TRUE(b4.is_strict_superset(b));
  EXPECT_FALSE(b.is_strict_superset(b4));
  EXPECT_FALSE(b.is_strict_superset(b));
  EXPECT_EQ(dbox(p1, dpoint(dim, 3)), b5.intersection(b3));
  EXPECT_TRUE(b1.intersection(b5).empty());
  EXPECT_TRUE(b.intersection(b5).empty());
  EXPECT_TRUE(b1.intersection(b).empty());
  ostringstream buf;
  buf << b4;
  EXPECT_EQ("([0,0,0]:[4,4,4])", buf.str());
}

TEST(RegionCalculus, dregion) {
  const int dim = 3;
  typedef dpoint<int> dpoint;
  typedef dbox<int> dbox;
  typedef dregion<int> dregion;
  dregion r(dim);
  EXPECT_TRUE(r.invariant());
  EXPECT_TRUE(r.empty());
  dpoint p(dim);
  dpoint p1(dim, 1);
  dbox b(dim);
  dbox b1(p, p1);
  dregion r0(b);
  EXPECT_TRUE(r0.invariant());
  EXPECT_TRUE(r0.empty());
  dregion r1(b1);
  EXPECT_TRUE(r1.invariant());
  EXPECT_FALSE(r1.empty());
  EXPECT_TRUE(r == r);
  EXPECT_TRUE(r1 == r1);
  EXPECT_FALSE(r != r);
  EXPECT_TRUE(r != r1);
  dpoint p2(dim, 2);
  dbox b2(p, p2);
  dregion r2(b2);
  EXPECT_EQ(r2, dregion(::dregion<long long>(r2)));
  EXPECT_TRUE(r == r.intersection(r1));
  EXPECT_TRUE(r == r1.intersection(r));
  EXPECT_TRUE(r1 == r1.intersection(r2));
  EXPECT_TRUE(r1 == r2.intersection(r1));
  EXPECT_TRUE(r == r.difference(r));
  EXPECT_TRUE(r1 == r1.difference(r));
  EXPECT_TRUE(r == r.difference(r1));
  EXPECT_TRUE(r == r1.difference(r1));
  EXPECT_TRUE(r == r.setunion(r));
  EXPECT_TRUE(r1 == r1.setunion(r));
  EXPECT_TRUE(r1 == r.setunion(r1));
  EXPECT_TRUE(r1 == r1.setunion(r1));
  EXPECT_TRUE(r == r.symmetric_difference(r));
  EXPECT_TRUE(r1 == r1.symmetric_difference(r));
  EXPECT_TRUE(r1 == r.symmetric_difference(r1));
  EXPECT_TRUE(r == r1.symmetric_difference(r1));
  vector<dbox> r12vals;
  r12vals.push_back(b1);
  r12vals.push_back(dbox(p1, p2));
  dregion r12(r12vals);
  vector<dbox> r12boxes = r12;
  EXPECT_EQ(r12vals, r12boxes);
  vector<dregion> rs;
  rs.push_back(r);
  rs.push_back(r1);
  rs.push_back(r2);
  dbox b4(p, dpoint(dim, 4));
  dregion r4(b4);
  rs.push_back(r4);
  rs.push_back(r12);
  rs.push_back(r2.difference(r1));
  rs.push_back(r2.symmetric_difference(r12));
  for (std::size_t i = 0; i < rs.size(); ++i) {
    const auto &ri = rs[i];
    EXPECT_TRUE(ri.invariant());
    for (std::size_t j = 0; j < rs.size(); ++j) {
      const auto &rj = rs[j];
      auto rintersection = ri.intersection(rj);
      auto rdifference = ri.difference(rj);
      auto rsetunion = ri.setunion(rj);
      auto rsymmetric_difference = ri.symmetric_difference(rj);
      EXPECT_TRUE(rintersection.invariant());
      EXPECT_TRUE(rdifference.invariant());
      EXPECT_TRUE(rsetunion.invariant());
      EXPECT_TRUE(rsymmetric_difference.invariant());
      EXPECT_TRUE(ri.issuperset(rintersection));
      EXPECT_TRUE(rj.issuperset(rintersection));
      EXPECT_TRUE(ri.issuperset(rdifference));
      EXPECT_TRUE(rsetunion.issuperset(ri));
      EXPECT_TRUE(rsetunion.issuperset(rj));
      EXPECT_TRUE(rintersection.isdisjoint(rsymmetric_difference));
      EXPECT_TRUE(rdifference.isdisjoint(rj));
      EXPECT_TRUE(rsetunion.issuperset(rintersection));
      EXPECT_TRUE(rsetunion.issuperset(rdifference));
      EXPECT_TRUE(rsetunion.issuperset(rsymmetric_difference));
      EXPECT_TRUE(rintersection.setunion(rsymmetric_difference) == rsetunion);
      EXPECT_TRUE(rdifference.setunion(rj) == rsetunion);
      EXPECT_TRUE(rintersection == rj.intersection(ri));
      EXPECT_TRUE(rsetunion == rj.setunion(ri));
      EXPECT_TRUE(rsymmetric_difference == rj.symmetric_difference(ri));
      if (ri == rj) {
        EXPECT_TRUE(rintersection == ri);
        EXPECT_TRUE(rdifference.empty());
        EXPECT_TRUE(rsetunion == ri);
        EXPECT_TRUE(rsymmetric_difference.empty());
      }
      EXPECT_TRUE(rintersection == (ri & rj));
      auto rintersection1 = ri;
      rintersection1 &= rj;
      EXPECT_TRUE(rintersection == rintersection1);
      EXPECT_TRUE(rdifference == (ri - rj));
      auto rdifference1 = ri;
      rdifference1 -= rj;
      EXPECT_TRUE(rdifference == rdifference1);
      EXPECT_TRUE(rsetunion == (ri | rj));
      auto rsetunion1 = ri;
      rsetunion1 |= rj;
      EXPECT_TRUE(rsetunion == rsetunion1);
      EXPECT_TRUE(rsymmetric_difference == (ri ^ rj));
      auto rsymmetric_difference1 = ri;
      rsymmetric_difference1 ^= rj;
      EXPECT_TRUE(rsymmetric_difference == rsymmetric_difference1);
    }
  }
  ostringstream buf;
  buf << r12;
  EXPECT_EQ("{([0,0,0]:[1,1,1]),([1,1,1]:[2,2,2])}", buf.str());
}

TEST(RegionCalculus, dpoint_dim) {
  typedef dpoint<int> dpoint;
  for (int dim = 0; dim < 4; ++dim) {
    dpoint p(dim);
    if (dim == 0)
      EXPECT_TRUE(all(p));
    else
      EXPECT_FALSE(all(p));
    EXPECT_FALSE(any(p));
    dpoint p0(dim, 0);
    if (dim == 0)
      EXPECT_TRUE(all(p0));
    else
      EXPECT_FALSE(all(p0));
    EXPECT_FALSE(any(p0));
    EXPECT_TRUE(all(p0 == p));
    dpoint p1(::dpoint<bool>(dim, true));
    EXPECT_TRUE(all(p1));
    if (dim == 0)
      EXPECT_FALSE(any(p1));
    else
      EXPECT_TRUE(any(p1));
    EXPECT_TRUE(all(p1 != p));
    EXPECT_TRUE(all(p1 == +p1));
    EXPECT_TRUE(all(p0 == -p0));
    EXPECT_TRUE(all(p1 == abs(-p1)));
    EXPECT_TRUE(all(-p1 == ~p0));
    EXPECT_TRUE(all(!p0));
    dpoint p2(dpoint(dim, 2));
    EXPECT_TRUE(equal_to<dpoint>()(p2, dpoint(::dpoint<long long>(p2))));
    EXPECT_TRUE(all(p1 + p1 == p2));
    EXPECT_TRUE(all(p2 - p1 == p1));
    EXPECT_TRUE(all(p2 * p1 == p2));
    EXPECT_TRUE(all((p2 & p1) == p0));
    EXPECT_TRUE(all((p2 | p1) == p1 + p2));
    EXPECT_TRUE(all((p2 ^ p1) == p1 + p2));
    EXPECT_TRUE(all(p1 && p2));
    EXPECT_TRUE(all(p1 || p2));
    EXPECT_TRUE(all(p0 <= p1 && p0 < p1 && p1 >= p0 && p1 > p0));
    EXPECT_EQ(dim == 0 ? numeric_limits<int>::max() : 2, minval(p2));
    EXPECT_EQ(dim == 0 ? numeric_limits<int>::min() : 2, maxval(p2));
    EXPECT_EQ(dim, sum(p1));
    EXPECT_EQ(1 << dim, prod(p2));
    vector<int> p3vals(dim);
    for (int d = 0; d < dim; ++d)
      p3vals[d] = d;
    dpoint p3(p3vals);
    EXPECT_TRUE(all((p1 + p3) * dpoint(dim, 2) - p3 == p3 + p2));
    vector<int> vi(dim);
    for (int d = 0; d < dim; ++d)
      vi[d] = d;
    EXPECT_TRUE(all(p3 == ::dpoint<int>(vi)));
    vector<short> vs(dim);
    for (int d = 0; d < dim; ++d)
      vs[d] = d;
    EXPECT_TRUE(all(p3 == ::dpoint<int>(vs)));
    // EXPECT_TRUE(all(p3 == ::dpoint<int>(array<int, 3>{{0, 1, 2}})));
    // EXPECT_TRUE(all(p3 == ::dpoint<int>(array<short, 3>{{0, 1, 2}})));
    EXPECT_TRUE(all(p2 == ::dpoint<int>(::dpoint<int>(dim, 2))));
    EXPECT_TRUE(all(p2 == ::dpoint<int>(::dpoint<short>(dim, 2))));
    auto v2 = vector<int>(dim, 2);
    EXPECT_EQ(v2, vector<int>(::dpoint<int>(dim, 2)));
    EXPECT_EQ(v2, vector<int>(::dpoint<short>(dim, 2)));
    ostringstream buf;
    buf << p3;
    ostringstream goodbuf;
    goodbuf << "[";
    for (int d = 0; d < dim; ++d) {
      if (d > 0)
        goodbuf << ",";
      goodbuf << d;
    }
    goodbuf << "]";
    EXPECT_EQ(goodbuf.str(), buf.str());
  }
}

TEST(RegionCalculus, dbox_dim) {
  typedef dpoint<int> dpoint;
  typedef dbox<int> dbox;
  for (int dim = 0; dim < 4; ++dim) {
    dbox b(dim);
    EXPECT_TRUE(b.empty());
    dpoint p0(dim, 0), p1(dim, 1);
    dbox b1(dbox(p0, p1));
    EXPECT_EQ(1, b1.size());
    dbox b4(p0, dpoint(dim, 4));
    dbox b5 = b4 >> p1;
    dbox b3 = b4 << p1;
    dbox b6 = b3 * dpoint(dim, 2);
    dbox b7 = b4.grow(p0, p1);
    dbox b8 = b4.grow(p1, p0);
    dbox b9 = b4.shrink(p0, p1);
    EXPECT_EQ(b4, dbox(::dbox<long long>(b4)));
    EXPECT_TRUE(b4 == b4);
    if (dim == 0)
      EXPECT_FALSE(b4 != b5);
    else
      EXPECT_TRUE(b4 != b5);
    EXPECT_TRUE(b4.contains(p1));
    if (dim == 0)
      EXPECT_TRUE(b5.contains(p0));
    else
      EXPECT_FALSE(b5.contains(p0));
    EXPECT_FALSE(b5.isdisjoint(b3));
    if (dim == 0)
      EXPECT_FALSE(b1.isdisjoint(b5));
    else
      EXPECT_TRUE(b1.isdisjoint(b5));
    EXPECT_TRUE(b.isdisjoint(b5));
    EXPECT_TRUE(b1.isdisjoint(b));
    EXPECT_TRUE(b6.issuperset(b3));
    if (dim == 0)
      EXPECT_TRUE(b3.issuperset(b6));
    else
      EXPECT_FALSE(b3.issuperset(b6));
    EXPECT_TRUE(b4.issuperset(b4));
    EXPECT_TRUE(b4.issuperset(b));
    EXPECT_FALSE(b.issuperset(b4));
    EXPECT_TRUE(b.issuperset(b));
    EXPECT_FALSE(b4.is_strict_superset(b4));
    EXPECT_TRUE(b4.is_strict_superset(b));
    EXPECT_FALSE(b.is_strict_superset(b4));
    EXPECT_FALSE(b.is_strict_superset(b));
    EXPECT_EQ(dbox(p1, dpoint(dim, 3)), b5.intersection(b3));
    if (dim == 0)
      EXPECT_FALSE(b1.intersection(b5).empty());
    else
      EXPECT_TRUE(b1.intersection(b5).empty());
    EXPECT_TRUE(b.intersection(b5).empty());
    EXPECT_TRUE(b1.intersection(b).empty());
    ostringstream buf;
    buf << b4;
    ostringstream goodbuf;
    goodbuf << "(";
    if (dim == 0) {
      goodbuf << 1;
    } else {
      goodbuf << "[";
      for (int d = 0; d < dim; ++d) {
        if (d > 0)
          goodbuf << ",";
        goodbuf << 0;
      }
      goodbuf << "]:[";
      for (int d = 0; d < dim; ++d) {
        if (d > 0)
          goodbuf << ",";
        goodbuf << 4;
      }
      goodbuf << "]";
    }
    goodbuf << ")";
    EXPECT_EQ(goodbuf.str(), buf.str());
  }
}

TEST(RegionCalculus, dregion_dim) {
  typedef dpoint<int> dpoint;
  typedef dbox<int> dbox;
  typedef dregion<int> dregion;
  for (int dim = 0; dim < 4; ++dim) {
    dregion r(dim);
    EXPECT_TRUE(r.invariant());
    EXPECT_TRUE(r.empty());
    dpoint p(dim);
    dpoint p1(dim, 1);
    dbox b(dim);
    dbox b1(p, p1);
    dregion r0(b);
    EXPECT_TRUE(r0.invariant());
    EXPECT_TRUE(r0.empty());
    dregion r1(b1);
    EXPECT_TRUE(r1.invariant());
    EXPECT_FALSE(r1.empty());
    EXPECT_TRUE(r == r);
    EXPECT_TRUE(r1 == r1);
    EXPECT_FALSE(r != r);
    EXPECT_TRUE(r != r1);
    dpoint p2(dim, 2);
    dbox b2(p, p2);
    dregion r2(b2);
    EXPECT_EQ(r2, dregion(::dregion<long long>(r2)));
    EXPECT_TRUE(r == r.intersection(r1));
    EXPECT_TRUE(r == r1.intersection(r));
    EXPECT_TRUE(r1 == r1.intersection(r2));
    EXPECT_TRUE(r1 == r2.intersection(r1));
    EXPECT_TRUE(r == r.difference(r));
    EXPECT_TRUE(r1 == r1.difference(r));
    EXPECT_TRUE(r == r.difference(r1));
    EXPECT_TRUE(r == r1.difference(r1));
    EXPECT_TRUE(r == r.setunion(r));
    EXPECT_TRUE(r1 == r1.setunion(r));
    EXPECT_TRUE(r1 == r.setunion(r1));
    EXPECT_TRUE(r1 == r1.setunion(r1));
    EXPECT_TRUE(r == r.symmetric_difference(r));
    EXPECT_TRUE(r1 == r1.symmetric_difference(r));
    EXPECT_TRUE(r1 == r.symmetric_difference(r1));
    EXPECT_TRUE(r == r1.symmetric_difference(r1));
    vector<dbox> r12vals;
    if (dim == 0) {
      r12vals.push_back(b1);
    } else if (dim == 1) {
      r12vals.push_back(dbox(p, p2));
    } else {
      r12vals.push_back(b1);
      r12vals.push_back(dbox(p1, p2));
    }
    dregion r12(r12vals);
    vector<dbox> r12boxes = r12;
    EXPECT_EQ(r12vals, r12boxes);
    vector<dregion> rs;
    rs.push_back(r);
    rs.push_back(r1);
    rs.push_back(r2);
    dbox b4(p, dpoint(dim, 4));
    dregion r4(b4);
    rs.push_back(r4);
    rs.push_back(r12);
    rs.push_back(r2.difference(r1));
    rs.push_back(r2.symmetric_difference(r12));
    rs.push_back(r2 >> p1);
    rs.push_back(r2 << p1);
    rs.push_back(r2.grow(p1));
    rs.push_back(r2.shrink(p1));
    for (std::size_t i = 0; i < rs.size(); ++i) {
      const auto &ri = rs[i];
      EXPECT_TRUE(ri.invariant());
      for (std::size_t j = 0; j < rs.size(); ++j) {
        const auto &rj = rs[j];
        auto rintersection = ri.intersection(rj);
        auto rdifference = ri.difference(rj);
        auto rsetunion = ri.setunion(rj);
        auto rsymmetric_difference = ri.symmetric_difference(rj);
        EXPECT_TRUE(rintersection.invariant());
        EXPECT_TRUE(rdifference.invariant());
        EXPECT_TRUE(rsetunion.invariant());
        EXPECT_TRUE(rsymmetric_difference.invariant());
        EXPECT_TRUE(ri.issuperset(rintersection));
        EXPECT_TRUE(rj.issuperset(rintersection));
        EXPECT_TRUE(ri.issuperset(rdifference));
        EXPECT_TRUE(rsetunion.issuperset(ri));
        EXPECT_TRUE(rsetunion.issuperset(rj));
        EXPECT_TRUE(rintersection.isdisjoint(rsymmetric_difference));
        EXPECT_TRUE(rdifference.isdisjoint(rj));
        EXPECT_TRUE(rsetunion.issuperset(rintersection));
        EXPECT_TRUE(rsetunion.issuperset(rdifference));
        EXPECT_TRUE(rsetunion.issuperset(rsymmetric_difference));
        EXPECT_TRUE(rintersection.setunion(rsymmetric_difference) == rsetunion);
        EXPECT_TRUE(rdifference.setunion(rj) == rsetunion);
        EXPECT_TRUE(rintersection == rj.intersection(ri));
        EXPECT_TRUE(rsetunion == rj.setunion(ri));
        EXPECT_TRUE(rsymmetric_difference == rj.symmetric_difference(ri));
        if (ri == rj) {
          EXPECT_TRUE(rintersection == ri);
          EXPECT_TRUE(rdifference.empty());
          EXPECT_TRUE(rsetunion == ri);
          EXPECT_TRUE(rsymmetric_difference.empty());
        }
      }
    }
    ostringstream buf;
    buf << r12;
    ostringstream goodbuf;
    goodbuf << "{";
    if (dim == 0) {
      goodbuf << "(" << true << ")";
    } else {
      goodbuf << "([";
      for (int d = 0; d < dim; ++d) {
        if (d > 0)
          goodbuf << ",";
        goodbuf << 0;
      }
      goodbuf << "]:[";
      if (dim > 1) {
        for (int d = 0; d < dim; ++d) {
          if (d > 0)
            goodbuf << ",";
          goodbuf << 1;
        }
        goodbuf << "]),([";
        for (int d = 0; d < dim; ++d) {
          if (d > 0)
            goodbuf << ",";
          goodbuf << 1;
        }
        goodbuf << "]:[";
      }
      for (int d = 0; d < dim; ++d) {
        if (d > 0)
          goodbuf << ",";
        goodbuf << 2;
      }
      goodbuf << "])";
    }
    goodbuf << "}";
    EXPECT_EQ(goodbuf.str(), buf.str());
  }
}

TEST(RegionCalculus, point_double) {
  typedef point<double, 3> point;
  point p;
  point p0(0);
  EXPECT_TRUE(all(p0 == p));
  point p1(::point<float, 3>(1));
  EXPECT_TRUE(all(p1 != p));
  EXPECT_TRUE(all(p1 == +p1));
  EXPECT_TRUE(all(p0 == -p0));
  EXPECT_TRUE(all(p1 == abs(-p1)));
  point p2(point(2));
  EXPECT_TRUE(equal_to<point>()(p2, point(::point<float, 3>(p2))));
  EXPECT_TRUE(all(p1 + p1 == p2));
  EXPECT_TRUE(all(p2 - p1 == p1));
  EXPECT_TRUE(all(p2 * p1 == p2));
  EXPECT_TRUE(all(p0 <= p1 && p0 < p1 && p1 >= p0 && p1 > p0));
  EXPECT_EQ(2, minval(p2));
  EXPECT_EQ(2, maxval(p2));
  EXPECT_EQ(3, sum(p1));
  EXPECT_EQ(8, prod(p2));
  point p3;
  p3.elt[0] = 0;
  p3.elt[1] = 1;
  p3.elt[2] = 2;
  EXPECT_TRUE(all((p1 + p3) * point(2) - p3 == p3 + p2));
  EXPECT_TRUE(all(p3 == ::point<double, 3>(vector<double>{{0, 1, 2}})));
  EXPECT_TRUE(all(p3 == ::point<double, 3>(vector<float>{{0, 1, 2}})));
  EXPECT_TRUE(all(p3 == ::point<double, 3>(array<double, 3>{{0, 1, 2}})));
  EXPECT_TRUE(all(p3 == ::point<double, 3>(array<float, 3>{{0, 1, 2}})));
  EXPECT_TRUE(all(p2 == ::point<double, 3>(::point<double, 3>(2))));
  EXPECT_TRUE(all(p2 == ::point<double, 3>(::point<float, 3>(2))));
  auto v2 = vector<double>({2, 2, 2});
  EXPECT_EQ(v2, vector<double>(::point<double, 3>(2)));
  EXPECT_EQ(v2, vector<double>(::point<float, 3>(2)));
  ostringstream buf;
  buf << p3;
  EXPECT_EQ("[0,1,2]", buf.str());
}

TEST(RegionCalculus, box_double) {
  typedef point<double, 3> point;
  typedef box<double, 3> box;
  box b;
  EXPECT_TRUE(b.empty());
  point p0(0), p1(1);
  box b1(box(p0, p1));
  EXPECT_EQ(1, b1.size());
  box b4(p0, point(4));
  box b5 = b4 >> p1;
  box b3 = b4 << p1;
  box b6 = b3 * point(2);
  box b7 = b4.grow(p0, p1);
  box b8 = b4.grow(p1, p0);
  box b9 = b4.shrink(p0, p1);
  EXPECT_EQ(b4, (box(::box<double, 3>(b4))));
  EXPECT_TRUE(b4 == b4);
  EXPECT_TRUE(b4 != b5);
  EXPECT_TRUE(b4.contains(p1));
  EXPECT_FALSE(b5.contains(p0));
  EXPECT_FALSE(b5.isdisjoint(b3));
  EXPECT_TRUE(b1.isdisjoint(b5));
  EXPECT_TRUE(b.isdisjoint(b5));
  EXPECT_TRUE(b1.isdisjoint(b));
  EXPECT_TRUE(b6.issuperset(b3));
  EXPECT_FALSE(b3.issuperset(b6));
  EXPECT_TRUE(b4.issuperset(b4));
  EXPECT_TRUE(b4.issuperset(b));
  EXPECT_FALSE(b.issuperset(b4));
  EXPECT_TRUE(b.issuperset(b));
  EXPECT_TRUE(b7.issuperset(b4));
  EXPECT_TRUE(b7.issuperset(b5));
  EXPECT_TRUE(b8.issuperset(b4));
  EXPECT_TRUE(b8.issuperset(b3));
  EXPECT_FALSE(b4.is_strict_superset(b4));
  EXPECT_TRUE(b4.is_strict_superset(b));
  EXPECT_FALSE(b.is_strict_superset(b4));
  EXPECT_FALSE(b.is_strict_superset(b));
  EXPECT_EQ(box(p1, point(3)), b5.intersection(b3));
  EXPECT_TRUE(b1.intersection(b5).empty());
  EXPECT_TRUE(b.intersection(b5).empty());
  EXPECT_TRUE(b1.intersection(b).empty());
  ostringstream buf;
  buf << b4;
  EXPECT_EQ("([0,0,0]:[4,4,4])", buf.str());
}

TEST(RegionCalculus, region_double) {
  typedef point<double, 3> point;
  typedef box<double, 3> box;
  typedef region<double, 3> region;
  region r;
  EXPECT_TRUE(r.invariant());
  EXPECT_TRUE(r.empty());
  point p;
  point p1(1);
  box b;
  box b1(p, p1);
  region r0(b);
  EXPECT_TRUE(r0.invariant());
  EXPECT_TRUE(r0.empty());
  region r1(b1);
  EXPECT_TRUE(r1.invariant());
  EXPECT_FALSE(r1.empty());
  EXPECT_TRUE(r == r);
  EXPECT_TRUE(r1 == r1);
  EXPECT_FALSE(r != r);
  EXPECT_TRUE(r != r1);
  point p2(2);
  box b2(p, p2);
  region r2(b2);
  EXPECT_EQ(r2, (region(::region<long long, 3>(r2))));
  EXPECT_TRUE(r == r.intersection(r1));
  EXPECT_TRUE(r == r1.intersection(r));
  EXPECT_TRUE(r1 == r1.intersection(r2));
  EXPECT_TRUE(r1 == r2.intersection(r1));
  EXPECT_TRUE(r == r.difference(r));
  EXPECT_TRUE(r1 == r1.difference(r));
  EXPECT_TRUE(r == r.difference(r1));
  EXPECT_TRUE(r == r1.difference(r1));
  EXPECT_TRUE(r == r.setunion(r));
  EXPECT_TRUE(r1 == r1.setunion(r));
  EXPECT_TRUE(r1 == r.setunion(r1));
  EXPECT_TRUE(r1 == r1.setunion(r1));
  EXPECT_TRUE(r == r.symmetric_difference(r));
  EXPECT_TRUE(r1 == r1.symmetric_difference(r));
  EXPECT_TRUE(r1 == r.symmetric_difference(r1));
  EXPECT_TRUE(r == r1.symmetric_difference(r1));
  vector<box> r12vals;
  r12vals.push_back(b1);
  r12vals.push_back(box(p1, p2));
  region r12(r12vals);
  vector<box> r12boxes = r12;
  EXPECT_EQ(r12vals, r12boxes);
  vector<region> rs;
  rs.push_back(r);
  rs.push_back(r1);
  rs.push_back(r2);
  box b4(p, point(4));
  region r4(b4);
  rs.push_back(r4);
  rs.push_back(r12);
  rs.push_back(r2.difference(r1));
  rs.push_back(r2.symmetric_difference(r12));
  for (std::size_t i = 0; i < rs.size(); ++i) {
    const auto &ri = rs[i];
    auto rgrown = ri.grow(p1);
    auto rshrunk = ri.shrink(p1);
    EXPECT_TRUE(ri.invariant());
    EXPECT_TRUE(rgrown.invariant());
    EXPECT_TRUE(rshrunk.invariant());
    EXPECT_TRUE(rgrown.issuperset(ri));
    if (ri.empty())
      EXPECT_TRUE(rgrown.empty());
    region rgrown_test;
    for (int dk = -1; dk <= +1; ++dk)
      for (int dj = -1; dj <= +1; ++dj)
        for (int di = -1; di <= +1; ++di) {
          auto shifted = ri >> point(di, dj, dk);
          rgrown_test |= shifted;
        }
    EXPECT_TRUE(rgrown_test == rgrown);
    EXPECT_TRUE(ri.issuperset(rshrunk));
    if (!rshrunk.empty())
      EXPECT_TRUE(rshrunk.grow(p1) == ri);
    region rshrunk_test = ri;
    for (int dk = -1; dk <= +1; ++dk)
      for (int dj = -1; dj <= +1; ++dj)
        for (int di = -1; di <= +1; ++di) {
          auto shifted = ri >> point(di, dj, dk);
          rshrunk_test &= shifted;
        }
    EXPECT_TRUE(rshrunk_test == rshrunk);
    region rgrown_shrunk_test = rgrown;
    for (int dk = -1; dk <= +1; ++dk)
      for (int dj = -1; dj <= +1; ++dj)
        for (int di = -1; di <= +1; ++di) {
          auto shifted = rgrown >> point(di, dj, dk);
          rgrown_shrunk_test &= shifted;
        }
    EXPECT_TRUE(rgrown_shrunk_test == ri);
    EXPECT_TRUE(rgrown.shrink(p1) == ri);
    for (std::size_t j = 0; j < rs.size(); ++j) {
      const auto &rj = rs[j];
      auto rintersection = ri.intersection(rj);
      auto rdifference = ri.difference(rj);
      auto rsetunion = ri.setunion(rj);
      auto rsymmetric_difference = ri.symmetric_difference(rj);
      EXPECT_TRUE(rintersection.invariant());
      EXPECT_TRUE(rdifference.invariant());
      EXPECT_TRUE(rsetunion.invariant());
      EXPECT_TRUE(rsymmetric_difference.invariant());
      EXPECT_TRUE(ri.issuperset(rintersection));
      EXPECT_TRUE(ri.issuperset(rintersection));
      EXPECT_TRUE(rj.issuperset(rintersection));
      EXPECT_TRUE(ri.issuperset(rdifference));
      EXPECT_TRUE(rsetunion.issuperset(ri));
      EXPECT_TRUE(rsetunion.issuperset(rj));
      EXPECT_TRUE(rintersection.isdisjoint(rsymmetric_difference));
      EXPECT_TRUE(rdifference.isdisjoint(rj));
      EXPECT_TRUE(rsetunion.issuperset(rintersection));
      EXPECT_TRUE(rsetunion.issuperset(rdifference));
      EXPECT_TRUE(rsetunion.issuperset(rsymmetric_difference));
      EXPECT_TRUE(rintersection.setunion(rsymmetric_difference) == rsetunion);
      EXPECT_TRUE(rdifference.setunion(rj) == rsetunion);
      EXPECT_TRUE(rintersection == rj.intersection(ri));
      EXPECT_TRUE(rsetunion == rj.setunion(ri));
      EXPECT_TRUE(rsymmetric_difference == rj.symmetric_difference(ri));
      if (ri == rj) {
        EXPECT_TRUE(rintersection == ri);
        EXPECT_TRUE(rdifference.empty());
        EXPECT_TRUE(rsetunion == ri);
        EXPECT_TRUE(rsymmetric_difference.empty());
      }
    }
  }
  ostringstream buf;
  buf << r12;
  EXPECT_EQ("{([0,0,0]:[1,1,1]),([1,1,1]:[2,2,2])}", buf.str());
}

TEST(RegionCalculus, double_) {
  for (int d = 0; d <= 4; ++d) {
    dpoint<double> p(d);
    dbox<double> b(d);
    dregion<double> r(d);
  }
}

namespace benchmark {
constexpr int D = 3;
typedef double T;
typedef point<T, D> P;
typedef box<T, D> B;
typedef region<T, D> R;
array<R, 2> regs;
} // namespace benchmark

TEST(RegionCalculus, benchmark_region_setup) {
  using namespace benchmark;

  constexpr int niters = 10000;
  for (int i = 0; i < 2; ++i) {
    vector<B> bs;
    bs.reserve(niters);
    for (int n = 0; n < niters; ++n) {
      array<int, D> xs, ys;
      for (int d = 0; d < D; ++d) {
        xs[d] = irand(100);
        ys[d] = xs[d] + irand(10);
      }
      P px(xs), py(ys);
      B b{px, py};
      bs.push_back(b);
    }
    regs[i] = R(bs);
  }
}

TEST(RegionCalculus, benchmark_region_intersection) {
  using namespace benchmark;
  auto rint = regs[0] & regs[1];
}
TEST(RegionCalculus, benchmark_region_union) {
  using namespace benchmark;
  auto runi = regs[0] | regs[1];
}
TEST(RegionCalculus, benchmark_region_symmetric_difference) {
  using namespace benchmark;
  auto rsym = regs[0] ^ regs[1];
}
TEST(RegionCalculus, benchmark_region_difference) {
  using namespace benchmark;
  auto rdif = regs[0] - regs[1];
}
TEST(RegionCalculus, benchmark_region_equal) {
  using namespace benchmark;
  auto req = regs[0] == regs[1];
}
TEST(RegionCalculus, benchmark_region_subset) {
  using namespace benchmark;
  auto rle = regs[0] & regs[1];
}
TEST(RegionCalculus, benchmark_region_strict_subset) {
  using namespace benchmark;
  auto rlt = regs[0] < regs[1];
}

#endif
