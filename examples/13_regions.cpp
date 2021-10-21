#include <openPMD/regions/Regions.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

void point_example() {
  using namespace openPMD::Regions;
  using namespace std;

  cout << "\n"
       << "Points (with dimension known at compile time):\n";

  cout << "  Define a point:\n";
  Point<int, 2> x{1, 2};
  cout << "    x: " << x << "\n";

  cout << "  Define a point from a vector:\n";
  vector vec{4, 5};
  Point<int, 2> y(vec);
  cout << "    y: " << y << "\n";

  cout << "  Arithmetic operations:\n";
  auto z = x + 2 * y;
  cout << "    z: " << z << "\n";

  cout << "  Unit vectors pointing in direction d:\n";
  auto u0 = Point<int, 2>::unit(0);
  auto u1 = Point<int, 2>::unit(1);
  cout << "    u0: " << u0 << "\n"
       << "    u1: " << u1 << "\n";
  cout << "  A points with all elements the same:\n";
  auto p3 = Point<int, 2>::pure(3);
  cout << "    p3: " << p3 << "\n";

  cout << "  Element-wise operations:\n";
  // using std::abs, std::max;
  auto mxy1 = max(abs(x), abs(y));
  // Apply arbitrary functions element-wise
  auto mxy2 = fmap([](auto a, auto b) { return max(abs(a), abs(b)); }, x, y);
  cout << "    mxy1: " << mxy1 << "\n"
       << "    mxy2: " << mxy2 << "\n";

  cout << "  Reduction operations:\n";
  int mx1 = max_element(x);
  // Apply arbitrary reduction operations
  int mx2 = fold([](auto r, auto a) { return max(r, a); }, 0, x);
  cout << "    mx1: " << mx1 << "\n"
       << "    mx2: " << mx2 << "\n";
}

////////////////////////////////////////////////////////////////////////////////

void ndpoint_example() {
  using namespace openPMD::Regions;
  using namespace std;

  cout << "\n"
       << "NDPoints (with dimension only known at run time):\n";

  cout << "  Define a point:\n";
  NDPoint<int> x{1, 2};
  cout << "    x: " << x << "\n";

  cout << "  Define a point from a vector:\n";
  vector vec{4, 5};
  NDPoint<int> y(vec);
  cout << "    y: " << y << "\n";

  // Arithmetic operations
  cout << "  Arithmetic operations:\n";
  auto z = x + 2 * y;
  cout << "    z: " << z << "\n";

  cout << "  Unit vectors pointing in direction d:\n";
  auto u0 = NDPoint<int>::unit(2, 0);
  auto u1 = NDPoint<int>::unit(2, 1);
  cout << "    u0: " << u0 << "\n"
       << "    u1: " << u1 << "\n";
  cout << "  A points with all elements the same:\n";
  auto p3 = NDPoint<int>::pure(2, 3);
  cout << "    p3: " << p3 << "\n";

  cout << "  Element-wise operations:\n";
  // using std::abs, std::max;
  auto mxy1 = max(abs(x), abs(y));
  // Apply arbitrary functions element-wise
  auto mxy2 = fmap([](auto a, auto b) { return max(abs(a), abs(b)); }, x, y);
  cout << "    mxy1: " << mxy1 << "\n"
       << "    mxy2: " << mxy2 << "\n";

  cout << "  Reduction operations:\n";
  int mx1 = max_element(x);
  // Apply arbitrary reduction operations
  int mx2 = fold([](auto r, auto a) { return max(r, a); }, 0, x);
  cout << "    mx1: " << mx1 << "\n"
       << "    mx2: " << mx2 << "\n";
}

////////////////////////////////////////////////////////////////////////////////

void box_example() {
  using namespace openPMD::Regions;
  using namespace std;

  cout << "\n"
       << "Boxes are spanned between points (inclusive lower, exclusive upper "
          "bound):\n";

  cout << "  Define two points:\n";
  Point<int, 2> x{1, 4}, y{2, 5};
  cout << "  x:" << x << "\n"
       << "  y:" << y << "\n";

  cout << "  Define a box between these points:\n";
  Box<int, 2> b(x, y);
  cout << "    b:  " << b << "   b.empty:  " << b.empty() << "\n";
  cout << "  Define an empty box:\n";
  Box<int, 2> be;
  cout << "    be: " << be << "   be.empty: " << be.empty() << "\n";

  cout << "  Boxes can be shifted and scaled (e.g. to change resolution):\n";
  Point<int, 2> offset{1, 2};
  auto b1 = b >> offset;
  Point<int, 2> scale{2, 2};
  auto b2 = b * scale;
  // Boxes can be grown and shrunk (e.g. to add ghost zones)
  auto bg = b.grown(1);
  auto bs = b.shrunk(1);
  cout << "    shifted box: " << b1 << "\n"
       << "    scaled box:  " << b2 << "\n"
       << "    grown box:   " << bg << "\n"
       << "    shrunk box:  " << bs << "\n";

  cout << "  The bounding box containing two boxes:\n";
  auto bb = bounding_box(b, b1);
  cout << "    bounding box: " << bb << "\n";

  cout << "  Boxes can be intersected:\n";
  auto bi = b & b1;
  cout << "  intersection: " << bi << "\n";

  cout << "  Set tests:\n";
  cout << "    b == b1 (equality):            " << (b == b1) << "\n"
       << "    b <= b1 (is_subset_of):        " << (b <= b1) << "\n"
       << "    b <  b1 (is_strict_subset_of): " << (b < b1) << "\n";
}

////////////////////////////////////////////////////////////////////////////////

void ndbox_example() {
  using namespace openPMD::Regions;
  using namespace std;

  cout << "\n"
       << "Boxes are spanned between points (inclusive lower, exclusive upper "
          "bound):\n";

  cout << "  Define two points:\n";
  NDPoint<int> x{1, 4}, y{2, 5};
  cout << "  x:" << x << "\n"
       << "  y:" << y << "\n";

  cout << "  Define a box between these points:\n";
  NDBox<int> b(x, y);
  cout << "    b:  " << b << "   b.empty:  " << b.empty() << "\n";
  cout << "  Define an empty box:\n";
  NDBox<int> be(2);
  cout << "    be: " << be << "   be.empty: " << be.empty() << "\n";

  cout << "  Boxes can be shifted and scaled (e.g. to change resolution):\n";
  NDPoint<int> offset{1, 2};
  auto b1 = b >> offset;
  NDPoint<int> scale{2, 2};
  auto b2 = b * scale;
  // Boxes can be grown and shrunk (e.g. to add ghost zones)
  auto bg = b.grown(1);
  auto bs = b.shrunk(1);
  cout << "    shifted box: " << b1 << "\n"
       << "    scaled box:  " << b2 << "\n"
       << "    grown box:   " << bg << "\n"
       << "    shrunk box:  " << bs << "\n";

  cout << "  The bounding box containing two boxes:\n";
  auto bb = bounding_box(b, b1);
  cout << "    bounding box: " << bb << "\n";

  cout << "  Boxes can be intersected:\n";
  auto bi = b & b1;
  cout << "  intersection: " << bi << "\n";

  cout << "  Set tests:\n";
  cout << "    b == b1 (equality):            " << (b == b1) << "\n"
       << "    b <= b1 (is_subset_of):        " << (b <= b1) << "\n"
       << "    b <  b1 (is_strict_subset_of): " << (b < b1) << "\n";
}

////////////////////////////////////////////////////////////////////////////////

void region_example() {
  using namespace openPMD::Regions;
  using namespace std;

  cout << "\n"
       << "Regions consist of a set of boxes:\n";

  cout << "  Define two points:\n";
  Point<int, 2> x{1, 4}, y{2, 5};
  cout << "  x:" << x << "\n"
       << "  y:" << y << "\n";

  cout << "  Define a box between these points:\n";
  Box<int, 2> b(x, y);
  cout << "  b:" << b << "\n";

  cout << "  Define a region consisting of this box:\n";
  Region<int, 2> r(b);
  cout << "  r:  " << r << "   r.empty:  " << r.empty() << "\n";
  cout << "  Define an empty region:\n";
  Region<int, 2> re;
  cout << "  re: " << re << "   re.empty: " << re.empty() << "\n";

  cout << "  Regions can be shifted and scaled (e.g. to change resolution):\n";
  Point<int, 2> offset{1, 2};
  auto r1 = r >> offset;
  Point<int, 2> scale{2, 2};
  auto r2 = r * scale;
  // Regions can be grown and shrunk (e.g. to add ghost zones)
  auto rg = r.grown(1);
  auto rs = r.shrunk(1);
  cout << "    shifted region: " << r1 << "\n"
       << "    scaled region:  " << r2 << "\n"
       << "    grown region:   " << rg << "\n"
       << "    shrunk region:  " << rs << "\n";

  cout << "  The bounding box containing a region:\n";
  auto bb = bounding_box(r);
  cout << "    bounding box: " << bb << "\n";

  cout << "  Regions can be treated as sets:\n";
  auto ri = r & r1;
  auto ru = r | r1;
  auto rd = r - r1;
  auto rx = r ^ r1;
  cout << "    intersection:         " << ri << "\n";
  cout << "    union:                " << ru << "\n";
  cout << "    difference:           " << rd << "\n";
  cout << "    symmetric difference: " << rx << "\n";

  cout << "  Set tests:\n";
  cout << "    r == r1 (equality):            " << (r == r1) << "\n"
       << "    r <= r1 (is_subset_of):        " << (r <= r1) << "\n"
       << "    r <  r1 (is_strict_subset_of): " << (r < r1) << "\n";

  cout << "  Regions can be converted to a list of boxes:\n";
  cout << "    rg - r:\n";
  for (const auto &bx : std::vector<Box<int, 2>>(rg - r))
    cout << "      " << bx << "\n";

  // Don't benchmark the debug version; it uses an N^2 algorithm for self-checks
  const int n = REGIONS_DEBUG ? 10 : 1000;
  cout << "  Create a list of " << n
       << " 3d boxes and convert it to a region:\n";
  std::mt19937 gen;
  std::uniform_int_distribution dist(0, 100);
  const auto random_point = [&]() {
    return Point<int, 3>::make([&](auto) { return dist(gen); });
  };
  const auto random_box = [&]() {
    auto p1 = random_point();
    auto p2 = random_point();
    // Sort the points to avoid creating many empty boxes
    return Box<int, 3>(min(p1, p2), max(p1, p2));
  };
  using clock = std::chrono::high_resolution_clock;
  using timestamp = std::chrono::time_point<clock>;
  using second = std::chrono::duration<double, std::ratio<1>>;
  const auto gettime = []() { return clock::now(); };
  const auto elapsed = [](timestamp t0, timestamp t1) {
    return std::chrono::duration_cast<second>(t1 - t0).count();
  };
  std::vector<Box<int, 3>> boxlist;
  for (int i = 0; i < n; ++i)
    boxlist.push_back(random_box());
  timestamp t0 = gettime();
  Region<int, 3> reg(boxlist);
  timestamp t1 = gettime();
  cout << "    reg.size:   " << reg.size() << "\n"
       << "    reg.nboxes: " << reg.nboxes() << "\n"
       << "    runtime:    " << elapsed(t0, t1) << " sec\n";
  cout << "  Grow the region by 1 point:\n";
  timestamp t2 = gettime();
  Region<int, 3> reg1 = reg.grown(1);
  timestamp t3 = gettime();
  cout << "    reg.size:   " << reg1.size() << "\n"
       << "    reg.nboxes: " << reg1.nboxes() << "\n"
       << "    runtime:    " << elapsed(t2, t3) << " sec\n";
}

////////////////////////////////////////////////////////////////////////////////

void ndregion_example() {
  using namespace openPMD::Regions;
  using namespace std;

  cout << "\n"
       << "Regions consist of a set of boxes:\n";

  cout << "  Define two points:\n";
  NDPoint<int> x{1, 4}, y{2, 5};
  cout << "  x:" << x << "\n"
       << "  y:" << y << "\n";

  cout << "  Define a box between these points:\n";
  NDBox<int> b(x, y);
  cout << "  b:" << b << "\n";

  cout << "  Define a region consisting of this box:\n";
  NDRegion<int> r(b);
  cout << "  r:  " << r << "   r.empty:  " << r.empty() << "\n";
  cout << "  Define an empty region:\n";
  NDRegion<int> re(2);
  cout << "  re: " << re << "   re.empty: " << re.empty() << "\n";

  cout << "  Regions can be shifted and scaled (e.g. to change resolution):\n";
  NDPoint<int> offset{1, 2};
  auto r1 = r >> offset;
  NDPoint<int> scale{2, 2};
  auto r2 = r * scale;
  // Regions can be grown and shrunk (e.g. to add ghost zones)
  auto rg = r.grown(1);
  auto rs = r.shrunk(1);
  cout << "    shifted region: " << r1 << "\n"
       << "    scaled region:  " << r2 << "\n"
       << "    grown region:   " << rg << "\n"
       << "    shrunk region:  " << rs << "\n";

  cout << "  The bounding box containing a region:\n";
  auto bb = bounding_box(r);
  cout << "    bounding box: " << bb << "\n";

  cout << "  Regions can be treated as sets:\n";
  auto ri = r & r1;
  auto ru = r | r1;
  auto rd = r - r1;
  auto rx = r ^ r1;
  cout << "    intersection:         " << ri << "\n";
  cout << "    union:                " << ru << "\n";
  cout << "    difference:           " << rd << "\n";
  cout << "    symmetric difference: " << rx << "\n";

  cout << "  Set tests:\n";
  cout << "    r == r1 (equality):            " << (r == r1) << "\n"
       << "    r <= r1 (is_subset_of):        " << (r <= r1) << "\n"
       << "    r <  r1 (is_strict_subset_of): " << (r < r1) << "\n";

  cout << "  Regions can be converted to a list of boxes:\n";
  cout << "    rg - r:\n";
  for (const auto &bx : std::vector<NDBox<int>>(rg - r))
    cout << "      " << bx << "\n";

  // Don't benchmark the debug version; it uses an N^2 algorithm for self-checks
  const int n = REGIONS_DEBUG ? 10 : 1000;
  cout << "  Create a list of " << n
       << " 3d boxes and convert it to a region:\n";
  std::mt19937 gen;
  std::uniform_int_distribution dist(0, 100);
  const auto random_point = [&]() {
    return NDPoint<int>::make(3, [&](auto) { return dist(gen); });
  };
  const auto random_box = [&]() {
    auto p1 = random_point();
    auto p2 = random_point();
    // Sort the points to avoid creating many empty boxes
    return NDBox<int>(min(p1, p2), max(p1, p2));
  };
  using clock = std::chrono::high_resolution_clock;
  using timestamp = std::chrono::time_point<clock>;
  using second = std::chrono::duration<double, std::ratio<1>>;
  const auto gettime = []() { return clock::now(); };
  const auto elapsed = [](timestamp t0, timestamp t1) {
    return std::chrono::duration_cast<second>(t1 - t0).count();
  };
  std::vector<NDBox<int>> boxlist;
  for (int i = 0; i < n; ++i)
    boxlist.push_back(random_box());
  timestamp t0 = gettime();
  NDRegion<int> reg(3, boxlist);
  timestamp t1 = gettime();
  cout << "    reg.size:   " << reg.size() << "\n"
       << "    reg.nboxes: " << reg.nboxes() << "\n"
       << "    runtime:    " << elapsed(t0, t1) << " sec\n";
  cout << "  Grow the region by 1 point:\n";
  timestamp t2 = gettime();
  NDRegion<int> reg1 = reg.grown(1);
  timestamp t3 = gettime();
  cout << "    reg.size:   " << reg1.size() << "\n"
       << "    reg.nboxes: " << reg1.nboxes() << "\n"
       << "    runtime:    " << elapsed(t2, t3) << " sec\n";
}

////////////////////////////////////////////////////////////////////////////////

int main() {
  point_example();
  ndpoint_example();

  box_example();
  ndbox_example();

  region_example();
  ndregion_example();

  return 0;
}
