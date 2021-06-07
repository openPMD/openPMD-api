#include <openPMD/regions/Regions.hpp>

#include <iostream>

int main() {
  using namespace openPMD::Regions;
  using namespace std;

  // Two points
  Point<int, 2> x{1, 2}, y{4, 5};
  auto z = x + y;
  cout << "Points:\n"
       << "x: " << x << "\n"
       << "y: " << y << "\n";

  // A box between these points
  Box<int, 2> b(x, y);
  auto bg = b.grown(1);
  cout << "Boxes are spanned between points (inclusive lower, exclusive upper "
          "bound):\n"
       << "b:  " << b << "\n"
       << "bg: " << bg << "\n";

  // Regions
  Region<int, 2> r(b);
  auto g = Region<int, 2>(bg) - r;
  cout << "Regions are sets of non-overlapping boxes:\n"
       << "r: " << r << "\n"
       << "g: " << g << "\n";

  {
    NDPoint<int> p0(0);
    NDPoint<int> p1(1);
    NDPoint<int> p2(2);
    NDPoint<int> p3(3);
  }

  return 0;
}
