#include "../include/Dataset.hpp"

Dataset::Dataset(Datatype d, Extent e)
        : extents{e},
          dtype{d},
          rank{static_cast<uint8_t>(e.size())}
{ }
