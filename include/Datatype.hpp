#pragma once

#include <iosfwd>
#include <memory>
#include <vector>


enum class Datatype : int
{
    CHAR = 0, UCHAR,
    INT16, INT32, INT64,
    UINT16, UINT32, UINT64,
    FLOAT, DOUBLE, LONG_DOUBLE,
    STRING,
    VEC_CHAR,
    VEC_INT16,
    VEC_INT32,
    VEC_INT64,
    VEC_UCHAR,
    VEC_UINT16,
    VEC_UINT32,
    VEC_UINT64,
    VEC_FLOAT,
    VEC_DOUBLE,
    VEC_LONG_DOUBLE,
    VEC_STRING,
    ARR_DBL_7,

    BOOL,

    DATATYPE = 1000,

    UNDEFINED
};  //Datatype

template<
        typename T,
        typename U
>
struct decay_equiv :
        std::is_same<
                typename std::remove_pointer<
                        typename std::remove_cv<
                                typename std::decay<
                                        typename std::remove_all_extents< T >::type
                                >::type
                        >::type
                >::type,
                U
        >::type
{ };

template< typename T >
#if __cplusplus >= 201402L
constexpr
#endif
Datatype
determineDatatype()
{
    using DT = Datatype;
    if( decay_equiv< T, long double >::value ) { return DT::LONG_DOUBLE; }
    if( decay_equiv< T, std::vector< long double > >::value ) { return DT::VEC_LONG_DOUBLE; }
    if( decay_equiv< T, double >::value ) { return DT::DOUBLE; }
    if( decay_equiv< T, std::vector< double > >::value ) { return DT::VEC_DOUBLE; }
    if( decay_equiv< T, float >::value ) { return  DT::FLOAT; }
    if( decay_equiv< T, std::vector< float > >::value ) { return  DT::VEC_FLOAT; }
    if( decay_equiv< T, int16_t >::value ) { return  DT::INT16; }
    if( decay_equiv< T, int32_t >::value ) { return  DT::INT32; }
    if( decay_equiv< T, int64_t >::value ) { return  DT::INT64; }
    if( decay_equiv< T, uint16_t >::value ) { return  DT::UINT16; }
    if( decay_equiv< T, uint32_t >::value ) { return  DT::UINT32; }
    if( decay_equiv< T, uint64_t >::value ) { return  DT::UINT64; }
    if( decay_equiv< T, char >::value ) { return  DT::CHAR; }
    if( decay_equiv< T, unsigned char >::value ) { return  DT::UCHAR; }
    if( decay_equiv< T, bool >::value ) { return  DT::BOOL; }
    return Datatype::UNDEFINED;
}

template< typename T >
#if __cplusplus >= 201402L
constexpr
#endif
inline Datatype
determineDatatype(std::shared_ptr< T >)
{
    using DT = Datatype;
    if( decay_equiv< T, double >::value ) { return DT::DOUBLE; }
    if( decay_equiv< T, float >::value ) { return  DT::FLOAT; }
    if( decay_equiv< T, int16_t >::value ) { return  DT::INT16; }
    if( decay_equiv< T, int32_t >::value ) { return  DT::INT32; }
    if( decay_equiv< T, int64_t >::value ) { return  DT::INT64; }
    if( decay_equiv< T, uint16_t >::value ) { return  DT::UINT16; }
    if( decay_equiv< T, uint32_t >::value ) { return  DT::UINT32; }
    if( decay_equiv< T, uint64_t >::value ) { return  DT::UINT64; }
    if( decay_equiv< T, char >::value ) { return  DT::CHAR; }
    if( decay_equiv< T, unsigned char >::value ) { return  DT::UCHAR; }
    if( decay_equiv< T, bool >::value ) { return  DT::BOOL; }

    return DT::UNDEFINED;
}

std::ostream&
operator<<(std::ostream&, Datatype);
