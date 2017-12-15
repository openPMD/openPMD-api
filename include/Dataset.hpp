#pragma once


#include <memory>
#include <type_traits>
#include <vector>

#include "Datatype.hpp"


using Extent = std::vector< std::uint64_t >;
using Offset = std::vector< std::uint64_t >;

class Dataset
{
    friend class RecordComponent;

public:
    Dataset(Datatype, Extent);

    Dataset& extend(Extent newExtent);
    Dataset& setChunkSize(std::vector< size_t > const&);
    Dataset& setCompression(std::string const&, uint8_t const);
    Dataset& setCustomTransform(std::string const&);

    Extent extent;
    Datatype dtype;
    uint8_t rank;
    Extent chunkSize;
    std::string compression;
    std::string transform;
};

