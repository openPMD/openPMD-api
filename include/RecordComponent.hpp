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
        BOOL,
        UNDEFINED
    };

    RecordComponent();

    double unitSI() const;
    RecordComponent& setUnitSI(double);

    template< typename T >
    void linkData(T* ptr, Dtype, std::vector< std::size_t > ext);

    template< typename T >
    T* retrieveData() const;

    void unlinkData();

    uint8_t rank;
    std::vector< std::size_t > extents;
    Dtype dtype;

private:
    std::vector< double > position() const;
    RecordComponent& setPosition(std::vector< double >);

    unsigned char* m_data;
};

template< typename T >
inline void
RecordComponent::linkData(T* ptr, Dtype dt, std::vector< std::size_t > ext)
{
    m_data = reinterpret_cast<unsigned char*>(ptr);
    rank = ext.size();
    dtype = dt;
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
