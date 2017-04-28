#pragma once


#include <type_traits>

#include "Attributable.hpp"

using Extent = std::vector< std::size_t >;
using Offset = std::vector< std::size_t >;

class Dataset
{
    friend class RecordComponent;

private:
    Dataset();

    std::shared_ptr<void> m_ownership;
    void* m_data;

public:
    enum class Dtype
    {
        DOUBLE,
        FLOAT,
        INT16, INT32, INT64,
        UINT16, UINT32, UINT64,
        CHAR,
        UCHAR,
        BOOL,
        UNDEFINED
    };

    template< typename T >
    Dataset(std::shared_ptr< T >, Extent);

    uint8_t rank;
    Extent extents;
    Dtype dtype;

};

template<
        typename T,
        typename U
>
struct decay_equiv :
        std::is_same<
                typename std::decay< typename std::remove_all_extents< T >::type >::type,
                U
        >::type
{ };

template< typename T >
inline Dataset::Dataset(std::shared_ptr< T > ptr, Extent ext)
        : m_ownership{std::static_pointer_cast< void >(ptr)},
          m_data{reinterpret_cast<void*>(ptr.get())},
          rank{static_cast<uint8_t>(ext.size())},
          extents{ext}
{
    if( decay_equiv< T, double >::value ) { dtype = Dtype::DOUBLE; }
    else if( decay_equiv< T, float >::value ) { dtype = Dtype::FLOAT; }
    else if( decay_equiv< T, int16_t >::value ) { dtype = Dtype::INT16; }
    else if( decay_equiv< T, int32_t >::value ) { dtype = Dtype::INT32; }
    else if( decay_equiv< T, int64_t >::value ) { dtype = Dtype::INT64; }
    else if( decay_equiv< T, uint16_t >::value ) { dtype = Dtype::UINT16; }
    else if( decay_equiv< T, uint32_t >::value ) { dtype = Dtype::UINT32; }
    else if( decay_equiv< T, uint64_t >::value ) { dtype = Dtype::UINT64; }
    else if( decay_equiv< T, char >::value ) { dtype = Dtype::CHAR; }
    else if( decay_equiv< T, unsigned char >::value ) { dtype = Dtype::UCHAR; }
    else if( decay_equiv< T, bool >::value ) { dtype = Dtype::BOOL; }
    else
    {
        throw std::runtime_error(
                std::string() + "Unknown datatype" + typeid(ptr.get()).name());
    }
}