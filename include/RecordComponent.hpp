#pragma once


#include "Attributable.hpp"


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
        BOOL
    };
    RecordComponent();

    double unitSI() const;
    RecordComponent setUnitSI(double);

    template< typename T >
    void linkData(T* ptr, Dtype, std::vector< std::size_t > ext);

    template< typename T >
    T* retrieveData();

    void unlinkData();

    std::array< std::size_t, 3 > extents;

private:
    std::vector< double > position() const;
    RecordComponent setPosition(std::vector< double >);

    unsigned char* m_data;
    Dtype m_dtype;
};

template< typename T >
inline void
RecordComponent::linkData(T* ptr, Dtype dt, std::vector< std::size_t > ext)
{
    assert(0 < ext.size() && ext.size() < 4);
    m_data = reinterpret_cast<unsigned char*>(ptr);
    m_dtype = dt;
    for( int i = 0; i < ext.size(); ++i )
    {
        extents[2 - i] = ext[ext.size() - 1 - i];
    }
}

template< typename T >
inline T*
RecordComponent::retrieveData()
{
    return reinterpret_cast<T*>(m_data);
}
