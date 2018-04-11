#pragma once

#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/Datatype.hpp"

#include <cstdint>
#include <type_traits>


namespace openPMD
{
class GenericPatchData
{
public:
    GenericPatchData();

    template< typename T >
    GenericPatchData& operator=(T);

    template< typename T >
    operator T();

private:
    enum Dtype {
        FLOAT, DOUBLE,
        UINT8, UINT16, UINT32, UINT64,
        INT8, INT16, INT32, INT64,
        BOOL,
        UNDEFINED };
    using variant_t = auxiliary::Variant< Dtype,
                                 float, double,
                                 uint8_t, uint16_t, uint32_t, uint64_t,
                                 int8_t, int16_t, int32_t, int64_t,
                                 bool >;
    variant_t m_data;
}; // GenericPatchData


template< typename T >
inline
GenericPatchData&
GenericPatchData::operator=(T t)
{
    static_assert(std::is_arithmetic< T >::value, "Only arithmetic types may be saved as patch data.\n");
    // Datatype d = determineDatatype< T >();
    m_data = variant_t(t);
    return *this;
}

template< typename T >
inline
GenericPatchData::operator T()
{
    return m_data.get< T >();
}
} // openPMD
