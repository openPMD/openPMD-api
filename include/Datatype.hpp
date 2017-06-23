#pragma once

enum class Datatype : int
{
    CHAR = 0, INT, FLOAT, DOUBLE,
    UINT32, UINT64, STRING,
    ARR_DBL_7,
    VEC_INT,
    VEC_FLOAT,
    VEC_DOUBLE,
    VEC_UINT64,
    VEC_STRING,

    INT16 = 1000, INT32, INT64,
    UINT16,
    UCHAR,
    BOOL,

    UNDEFINED
};
