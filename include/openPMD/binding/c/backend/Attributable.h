#ifndef OPENPMD_ATTRIBUTABLE_H
#define OPENPMD_ATTRIBUTABLE_H

#include <openPMD/binding/c/Datatype.h>
#include <openPMD/binding/c/IO/Access.h>

#include <complex.h>
#undef I
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_Attributable openPMD_Attributable;

    openPMD_Attributable *openPMD_Attributable_new();
    void openPMD_Attributable_delete(openPMD_Attributable *attributable);

    bool openPMD_Attributable_setAttribute_char(
        openPMD_Attributable *attributable, const char *key, char value);
    bool openPMD_Attributable_setAttribute_uchar(
        openPMD_Attributable *attributable,
        const char *key,
        unsigned char value);
    bool openPMD_Attributable_setAttribute_schar(
        openPMD_Attributable *attributable, const char *key, signed char value);
    bool openPMD_Attributable_setAttribute_short(
        openPMD_Attributable *attributable, const char *key, short value);
    bool openPMD_Attributable_setAttribute_int(
        openPMD_Attributable *attributable, const char *key, int value);
    bool openPMD_Attributable_setAttribute_long(
        openPMD_Attributable *attributable, const char *key, long value);
    bool openPMD_Attributable_setAttribute_longlong(
        openPMD_Attributable *attributable, const char *key, long long value);
    bool openPMD_Attributable_setAttribute_ushort(
        openPMD_Attributable *attributable,
        const char *key,
        unsigned short value);
    bool openPMD_Attributable_setAttribute_uint(
        openPMD_Attributable *attributable,
        const char *key,
        unsigned int value);
    bool openPMD_Attributable_setAttribute_ulong(
        openPMD_Attributable *attributable,
        const char *key,
        unsigned long value);
    bool openPMD_Attributable_setAttribute_ulonglong(
        openPMD_Attributable *attributable,
        const char *key,
        unsigned long long value);
    bool openPMD_Attributable_setAttribute_float(
        openPMD_Attributable *attributable, const char *key, float value);
    bool openPMD_Attributable_setAttribute_double(
        openPMD_Attributable *attributable, const char *key, double value);
    bool openPMD_Attributable_setAttribute_long_double(
        openPMD_Attributable *attributable, const char *key, long double value);
    bool openPMD_Attributable_setAttribute_cfloat2(
        openPMD_Attributable *attributable,
        const char *key,
        float value_re,
        float value_im);
    bool openPMD_Attributable_setAttribute_cdouble2(
        openPMD_Attributable *attributable,
        const char *key,
        double value_re,
        double value_im);
    bool openPMD_Attributable_setAttribute_clong_double2(
        openPMD_Attributable *attributable,
        const char *key,
        long double value_re,
        long double value_im);
#ifndef __cplusplus
    bool openPMD_Attributable_setAttribute_cfloat(
        openPMD_Attributable *attributable,
        const char *key,
        complex float value);
    bool openPMD_Attributable_setAttribute_cdouble(
        openPMD_Attributable *attributable,
        const char *key,
        complex double value);
    bool openPMD_Attributable_setAttribute_clong_double(
        openPMD_Attributable *attributable,
        const char *key,
        complex long double value);
#endif
    bool openPMD_Attributable_setAttribute_string(
        openPMD_Attributable *attributable, const char *key, const char *value);
    bool openPMD_Attributable_setAttribute_vec_char(
        openPMD_Attributable *attributable,
        const char *key,
        const char *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_uchar(
        openPMD_Attributable *attributable,
        const char *key,
        const unsigned char *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_schar(
        openPMD_Attributable *attributable,
        const char *key,
        const signed char *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_short(
        openPMD_Attributable *attributable,
        const char *key,
        const short *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_int(
        openPMD_Attributable *attributable,
        const char *key,
        const int *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_long(
        openPMD_Attributable *attributable,
        const char *key,
        const long *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_longlong(
        openPMD_Attributable *attributable,
        const char *key,
        const long long *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_ushort(
        openPMD_Attributable *attributable,
        const char *key,
        const unsigned short *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_uint(
        openPMD_Attributable *attributable,
        const char *key,
        const unsigned int *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_ulong(
        openPMD_Attributable *attributable,
        const char *key,
        const unsigned long *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_ulonglong(
        openPMD_Attributable *attributable,
        const char *key,
        const unsigned long long *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_float(
        openPMD_Attributable *attributable,
        const char *key,
        const float *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_double(
        openPMD_Attributable *attributable,
        const char *key,
        const double *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_long_double(
        openPMD_Attributable *attributable,
        const char *key,
        const long double *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_cfloat2(
        openPMD_Attributable *attributable,
        const char *key,
        const float *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_cdouble2(
        openPMD_Attributable *attributable,
        const char *key,
        const double *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_clong_double2(
        openPMD_Attributable *attributable,
        const char *key,
        const long double *values,
        size_t size);
#ifndef __cplusplus
    bool openPMD_Attributable_setAttribute_vec_cfloat(
        openPMD_Attributable *attributable,
        const char *key,
        const complex float *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_cdouble(
        openPMD_Attributable *attributable,
        const char *key,
        const complex double *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_vec_clong_double(
        openPMD_Attributable *attributable,
        const char *key,
        const complex long double *values,
        size_t size);
#endif
    bool openPMD_Attributable_setAttribute_vec_string(
        openPMD_Attributable *attributable,
        const char *key,
        const char *const *values,
        size_t size);
    bool openPMD_Attributable_setAttribute_bool(
        openPMD_Attributable *attributable, const char *key, bool value);

    openPMD_Datatype openPMD_Attributable_attributeDatatype(
        const openPMD_Attributable *attributable, const char *key);

    bool openPMD_Attributable_getAttribute_char(
        const openPMD_Attributable *attributable, const char *key, char *value);
    bool openPMD_Attributable_getAttribute_uchar(
        const openPMD_Attributable *attributable,
        const char *key,
        unsigned char *value);
    bool openPMD_Attributable_getAttribute_schar(
        const openPMD_Attributable *attributable,
        const char *key,
        signed char *value);
    bool openPMD_Attributable_getAttribute_short(
        const openPMD_Attributable *attributable,
        const char *key,
        short *value);
    bool openPMD_Attributable_getAttribute_int(
        const openPMD_Attributable *attributable, const char *key, int *value);
    bool openPMD_Attributable_getAttribute_long(
        const openPMD_Attributable *attributable, const char *key, long *value);
    bool openPMD_Attributable_getAttribute_longlong(
        const openPMD_Attributable *attributable,
        const char *key,
        long long *value);
    bool openPMD_Attributable_getAttribute_ushort(
        const openPMD_Attributable *attributable,
        const char *key,
        unsigned short *value);
    bool openPMD_Attributable_getAttribute_uint(
        const openPMD_Attributable *attributable,
        const char *key,
        unsigned int *value);
    bool openPMD_Attributable_getAttribute_ulong(
        const openPMD_Attributable *attributable,
        const char *key,
        unsigned long *value);
    bool openPMD_Attributable_getAttribute_ulonglong(
        const openPMD_Attributable *attributable,
        const char *key,
        unsigned long long *value);
    bool openPMD_Attributable_getAttribute_float(
        const openPMD_Attributable *attributable,
        const char *key,
        float *value);
    bool openPMD_Attributable_getAttribute_double(
        const openPMD_Attributable *attributable,
        const char *key,
        double *value);
    bool openPMD_Attributable_getAttribute_long_double(
        const openPMD_Attributable *attributable,
        const char *key,
        long double *value);
    bool openPMD_Attributable_getAttribute_cfloat2(
        const openPMD_Attributable *attributable,
        const char *key,
        float *value);
    bool openPMD_Attributable_getAttribute_cdouble2(
        const openPMD_Attributable *attributable,
        const char *key,
        double *value);
    bool openPMD_Attributable_getAttribute_clong_double2(
        const openPMD_Attributable *attributable,
        const char *key,
        long double *value);
#ifndef __cplusplus
    bool openPMD_Attributable_getAttribute_cfloat(
        const openPMD_Attributable *attributable,
        const char *key,
        complex float *value);
    bool openPMD_Attributable_getAttribute_cdouble(
        const openPMD_Attributable *attributable,
        const char *key,
        complex double *value);
    bool openPMD_Attributable_getAttribute_clong_double(
        const openPMD_Attributable *attributable,
        const char *key,
        complex long double *value);
#endif
    bool openPMD_Attributable_getAttribute_string(
        const openPMD_Attributable *attributable,
        const char *key,
        char **value);
    bool openPMD_Attributable_getAttribute_vec_char(
        const openPMD_Attributable *attributable,
        const char *key,
        char **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_uchar(
        const openPMD_Attributable *attributable,
        const char *key,
        unsigned char **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_schar(
        const openPMD_Attributable *attributable,
        const char *key,
        signed char **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_short(
        const openPMD_Attributable *attributable,
        const char *key,
        short **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_int(
        const openPMD_Attributable *attributable,
        const char *key,
        int **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_long(
        const openPMD_Attributable *attributable,
        const char *key,
        long **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_longlong(
        const openPMD_Attributable *attributable,
        const char *key,
        long long **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_ushort(
        const openPMD_Attributable *attributable,
        const char *key,
        unsigned short **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_uint(
        const openPMD_Attributable *attributable,
        const char *key,
        unsigned int **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_ulong(
        const openPMD_Attributable *attributable,
        const char *key,
        long **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_ulonglong(
        const openPMD_Attributable *attributable,
        const char *key,
        unsigned long long **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_float(
        const openPMD_Attributable *attributable,
        const char *key,
        float **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_double(
        const openPMD_Attributable *attributable,
        const char *key,
        double **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_long_double(
        const openPMD_Attributable *attributable,
        const char *key,
        long double **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_cfloat2(
        const openPMD_Attributable *attributable,
        const char *key,
        float **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_cdouble2(
        const openPMD_Attributable *attributable,
        const char *key,
        double **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_clong_double2(
        const openPMD_Attributable *attributable,
        const char *key,
        long double **values,
        size_t *size);
#ifndef __cplusplus
    bool openPMD_Attributable_getAttribute_vec_cfloat(
        const openPMD_Attributable *attributable,
        const char *key,
        complex float **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_cdouble(
        const openPMD_Attributable *attributable,
        const char *key,
        complex double **values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_vec_clong_double(
        const openPMD_Attributable *attributable,
        const char *key,
        complex long double **values,
        size_t *size);
#endif
    bool openPMD_Attributable_getAttribute_vec_string(
        const openPMD_Attributable *attributable,
        const char *key,
        char ***values,
        size_t *size);
    bool openPMD_Attributable_getAttribute_bool(
        const openPMD_Attributable *attributable, const char *key, bool *value);

    bool openPMD_Attributable_deleteAttribute(
        openPMD_Attributable *attributable, const char *key);

    // result is a pointer to pointers, both layers must be freed
    char **
    openPMD_Attributable_attributes(const openPMD_Attributable *attributable);

    size_t openPMD_Attributable_numAttributes(
        const openPMD_Attributable *attributable);

    bool openPMD_Attributable_containsAttribute(
        const openPMD_Attributable *attributable, const char *key);

    // result must be freed
    char *
    openPMD_Attributable_comment(const openPMD_Attributable *attributable);

    void openPMD_Attributable_setComment(
        openPMD_Attributable *attributable, const char *comment);

    // backendConfig may be NULL
    void openPMD_Attributable_seriesFlush(
        openPMD_Attributable *attributable, const char *backendConfig);

    typedef struct openPMD_Attributable_MyPath
    {
        char *directory;
        char *seriesName;
        char *seriesExtension;
        char **group; // NULL terminated
        openPMD_Access access;
    } openPMD_Attributable_MyPath;

    void openPMD_Attributable_MyPath_free(openPMD_Attributable_MyPath *myPath);

    char *openPMD_Attributable_MyPath_filePath(
        const openPMD_Attributable_MyPath *myPath);

    openPMD_Attributable_MyPath *
    openPMD_Attributable_myPath(const openPMD_Attributable *attributable);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_ATTRIBUTABLE_H
