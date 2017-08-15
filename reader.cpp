#include "include/reader/HDF5Reader.hpp"

int main()
{
    HDF5Reader test("../data00000100.h5");
    Output o = test.read();

    uint64_t it = o.iterations.begin()->first;
    std::string m = o.iterations[it].meshes.begin()->first;
    std::string c = o.iterations[it].meshes[m].keys()[0];
    RecordComponent r = o.iterations[it].meshes[m][c];
    return 0;
}
