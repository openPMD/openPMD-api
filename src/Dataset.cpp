#include <iostream>

#include "Dataset.hpp"

Dataset::Dataset(Datatype d, Extent e)
        : extent{e},
          dtype{d},
          rank{static_cast<uint8_t>(e.size())},
          chunkSize{e}
{ }

Dataset&
Dataset::extend(Extent newExtents)
{
    if( newExtents.size() != rank )
        throw std::runtime_error("Dimensionality of extended Dataset must match the original dimensionality");
    for( size_t i = 0; i < newExtents.size(); ++i )
        if( newExtents[i] < extent[i] )
            throw std::runtime_error("New Extent must be equal or greater than previous Extent");

    extent = newExtents;
    return *this;
}

Dataset&
Dataset::setChunkSize(std::vector< size_t > const& cs)
{
    if( extent.size() != rank )
        throw std::runtime_error("Dimensionality of extended Dataset must match the original dimensionality");
    for( size_t i = 0; i < cs.size(); ++i )
        if( cs[i] > extent[i] )
            throw std::runtime_error("Dataset chunk size must be equal or smaller than Extent");

    chunkSize = cs;
    return *this;
}

Dataset&
Dataset::setCompression(std::string const& format, uint8_t const level)
{
    if( (format == "zlib" || format == "gzip" || format == "deflate")
        && level > 9 )
        throw std::runtime_error("Compression level out of range for " + format);
    else
        std::cerr << "Unknown compression format " << format
                  << ". This might mean that compression will not be enabled."
                  << std::endl;

    compression = format + ':' + std::to_string(static_cast< int >(level));
    return *this;
}

Dataset&
Dataset::setCustomTransform(std::string const& parameter)
{
    transform = parameter;
    return *this;
}