#ifndef OPENPMD_ATTRIBUTE_H
#define OPENPMD_ATTRIBUTE_H

#include <openPMD/binding/c/Datatype.h>

#include <complex.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_Attribute
    {
        openPMD_Datatype datatype;
        union
        {
            char char_data;
            unsigned char uchar_data;
            signed char schar_data;
            short short_data;
            int int_data;
            long long_data;
            long long longlong_data;
            unsigned short ushort_data;
            unsigned int uint_data;
            unsigned long ulong_data;
            unsigned long long ulonglong_data;
            float float_data;
            double double_data;
            long double long_double_data;
            _Complex float cfloat_data;
            _Complex double cdouble_data;
            _Complex long double clong_double_data;
            bool bool_data;
        };
    } openPMD_Attribute;

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_ATTRIBUTE_H
