#ifndef OPENPMD_DATATYPE_H
#define OPENPMD_DATATYPE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum openPMD_Datatype
    {
        openPMD_Datatype_CHAR,
        openPMD_Datatype_UCHAR,
        openPMD_Datatype_SCHAR,
        openPMD_Datatype_SHORT,
        openPMD_Datatype_INT,
        openPMD_Datatype_LONG,
        openPMD_Datatype_LONGLONG,
        openPMD_Datatype_USHORT,
        openPMD_Datatype_UINT,
        openPMD_Datatype_ULONG,
        openPMD_Datatype_ULONGLONG,
        openPMD_Datatype_FLOAT,
        openPMD_Datatype_DOUBLE,
        openPMD_Datatype_LONG_DOUBLE,
        openPMD_Datatype_CFLOAT,
        openPMD_Datatype_CDOUBLE,
        openPMD_Datatype_CLONG_DOUBLE,
        openPMD_Datatype_STRING,
        openPMD_Datatype_VEC_CHAR,
        openPMD_Datatype_VEC_SHORT,
        openPMD_Datatype_VEC_INT,
        openPMD_Datatype_VEC_LONG,
        openPMD_Datatype_VEC_LONGLONG,
        openPMD_Datatype_VEC_UCHAR,
        openPMD_Datatype_VEC_USHORT,
        openPMD_Datatype_VEC_UINT,
        openPMD_Datatype_VEC_ULONG,
        openPMD_Datatype_VEC_ULONGLONG,
        openPMD_Datatype_VEC_FLOAT,
        openPMD_Datatype_VEC_DOUBLE,
        openPMD_Datatype_VEC_LONG_DOUBLE,
        openPMD_Datatype_VEC_CFLOAT,
        openPMD_Datatype_VEC_CDOUBLE,
        openPMD_Datatype_VEC_CLONG_DOUBLE,
        openPMD_Datatype_VEC_SCHAR,
        openPMD_Datatype_VEC_STRING,
        openPMD_Datatype_ARR_DBL_7,
        openPMD_Datatype_BOOL,
        openPMD_Datatype_UNDEFINED
    } openPMD_Datatype;

    const openPMD_Datatype *openPMD_Datatypes();
    size_t openPMD_DatatypesSize();

    size_t openPMD_toBytes(openPMD_Datatype datatype);
    size_t openPMD_toBits(openPMD_Datatype datatype);
    bool openPMD_isVector(openPMD_Datatype datatype);
    bool openPMD_isFloatingPoint(openPMD_Datatype datatype);
    bool openPMD_isComplexFloatingPoint(openPMD_Datatype datatype);
    bool openPMD_isInteger(openPMD_Datatype datatype);
    bool openPMD_isSigned(openPMD_Datatype datatype);
    bool openPMD_isChar(openPMD_Datatype datatype);
    bool openPMD_isSame(openPMD_Datatype datatype1, openPMD_Datatype datatype2);
    openPMD_Datatype openPMD_basicDatatype(openPMD_Datatype datatype);
    openPMD_Datatype openPMD_toVectorType(openPMD_Datatype datatype);
    const char *openPMD_datatypeToString(openPMD_Datatype datatype);
    openPMD_Datatype openPMD_stringToDatatype(const char *string);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_DATATYPE_H
