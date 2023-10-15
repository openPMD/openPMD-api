#ifndef OPENPMD_ATTRIBUTE_H
#define OPENPMD_ATTRIBUTE_H

#include <openPMD/binding/c/Datatype.h>

#include <complex.h>
#undef I

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
    struct openPMD_Attributable_cfloat
    {
        float re, im;
    };
    struct openPMD_Attributable_cdouble
    {
        double re, im;
    };
    struct openPMD_Attributable_clong_double
    {
        long double re, im;
    };
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
#ifndef __cplusplus
            complex float cfloat_data;
            complex double cdouble_data;
            complex long double clong_double_data;
#else
        struct openPMD_Attributable_cfloat cfloat_data;
        struct openPMD_Attributable_cdouble cdouble_data;
        struct openPMD_Attributable_clong_double clong_double_data;
#endif
            bool bool_data;
        };
    } openPMD_Attribute;

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_ATTRIBUTE_H
