#include "../include/Dataset.hpp"

Dataset::Dataset(Datatype d, Extent e)
        : rank{static_cast<uint8_t>(e.size())},
          extents{e},
          dtype{d}
{ }