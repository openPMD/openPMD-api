#pragma once


#include <type_traits>

#include "Attributable.hpp"


using Extent = std::vector< std::size_t >;

class RecordComponent : public Attributable
{
    friend class Mesh;

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
    };  //Dtype

    RecordComponent();

    template< typename T >
    RecordComponent(T* ptr, Extent ext);

    double unitSI() const;
    RecordComponent& setUnitSI(double);

    template< typename T >
    T* retrieveData() const;

    void unlinkData();

    uint8_t rank;
    std::vector< std::size_t > extents;
    Dtype dtype;

private:
    std::vector< double > position() const;
    RecordComponent& setPosition(std::vector< double >);

    void* m_data;
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
inline
RecordComponent::RecordComponent(T* ptr, Extent ext)
{
    m_data = reinterpret_cast<void*>(ptr);
    rank = ext.size();
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
    else { throw std::runtime_error(std::string() + "Unknown datatype" + typeid(ptr).name()); }
    extents = ext;
}

template< typename T >
inline T*
RecordComponent::retrieveData() const
{
    return reinterpret_cast<T*>(m_data);
}

inline void
RecordComponent::unlinkData()
{
    m_data = nullptr;
    rank = 0;
    dtype = Dtype::UNDEFINED;
    extents.clear();
}
