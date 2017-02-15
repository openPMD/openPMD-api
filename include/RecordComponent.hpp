#pragma once


#include "Attributable.hpp"


class RecordComponent : public Attributable
{
    friend class Mesh;
public:
    RecordComponent();

    double unitSI() const;
    RecordComponent setUnitSI(double);

    template< typename T >
    void linkData(T* ptr, std::vector< std::size_t > ext);

    template< typename T >
    T* retrieveData();

    void unlinkData();

    std::array< std::size_t, 3 > extents;

private:
    std::vector< double > position() const;
    RecordComponent setPosition(std::vector< double >);

    unsigned char* m_data;
};
