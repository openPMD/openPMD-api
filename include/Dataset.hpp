#pragma once


#include <memory>
#include <type_traits>
#include <string>
#include <vector>

#include "Datatype.hpp"


using Extent = std::vector< std::uint64_t >;
using Offset = std::vector< std::uint64_t >;

class Dataset
{
    friend class RecordComponent;

public:
    Dataset(Datatype, Extent);

    Extent extents;
    Datatype dtype;
    uint8_t rank;
};

