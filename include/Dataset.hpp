#pragma once


#include <type_traits>
#include <string>
#include <vector>

#include "Datatype.hpp"


using Extent = std::vector< std::uint64_t >;
using Offset = std::vector< std::uint64_t >;

class Dataset
{
    friend class RecordComponent;

private:
    Dataset();

    std::shared_ptr<void> m_ownership;
    void* m_data;

public:
    template< typename T >
    Dataset(std::shared_ptr< T >, Extent);

    uint8_t rank;
    Extent extents;
    Datatype dtype;

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
    using DT = Datatype;
    if( decay_equiv< T, double >::value ) { dtype = DT::DOUBLE; }
    else if( decay_equiv< T, float >::value ) { dtype = DT::FLOAT; }
    else if( decay_equiv< T, int16_t >::value ) { dtype = DT::INT16; }
    else if( decay_equiv< T, int32_t >::value ) { dtype = DT::INT32; }
    else if( decay_equiv< T, int64_t >::value ) { dtype = DT::INT64; }
    else if( decay_equiv< T, uint16_t >::value ) { dtype = DT::UINT16; }
    else if( decay_equiv< T, uint32_t >::value ) { dtype = DT::UINT32; }
    else if( decay_equiv< T, uint64_t >::value ) { dtype = DT::UINT64; }
    else if( decay_equiv< T, char >::value ) { dtype = DT::CHAR; }
    else if( decay_equiv< T, unsigned char >::value ) { dtype = DT::UCHAR; }
    else if( decay_equiv< T, bool >::value ) { dtype = DT::BOOL; }
    else
    {
        throw std::runtime_error(
                std::string() + "Unknown datatype" + typeid(ptr.get()).name());
    }
}
