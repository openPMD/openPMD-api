#include <openPMD/binding/c/backend/Attributable.h>

bool openPMD_Attributable_setAttribute_cfloat(
    openPMD_Attributable *attributable, const char *key, complex float value)
{
    return openPMD_Attributable_setAttribute_cfloat2(
        attributable, key, crealf(value), cimagf(value));
}

bool openPMD_Attributable_setAttribute_cdouble(
    openPMD_Attributable *attributable, const char *key, complex double value)
{
    return openPMD_Attributable_setAttribute_cdouble2(
        attributable, key, creal(value), cimag(value));
}

bool openPMD_Attributable_setAttribute_clong_double(
    openPMD_Attributable *attributable,
    const char *key,
    complex long double value)
{
    return openPMD_Attributable_setAttribute_clong_double2(
        attributable, key, creall(value), cimagl(value));
}

bool openPMD_Attributable_setAttribute_vec_cfloat(
    openPMD_Attributable *attributable,
    const char *key,
    const complex float *values,
    size_t size)
{
    return openPMD_Attributable_setAttribute_vec_cfloat2(
        attributable, key, (const float *)values, size);
}

bool openPMD_Attributable_setAttribute_vec_cdouble(
    openPMD_Attributable *attributable,
    const char *key,
    const complex double *values,
    size_t size)
{
    return openPMD_Attributable_setAttribute_vec_cdouble2(
        attributable, key, (const double *)values, size);
}

bool openPMD_Attributable_setAttribute_vec_clong_double(
    openPMD_Attributable *attributable,
    const char *key,
    const complex long double *values,
    size_t size)
{
    return openPMD_Attributable_setAttribute_vec_clong_double2(
        attributable, key, (const long double *)values, size);
}

bool openPMD_Attributable_getAttribute_cfloat(
    const openPMD_Attributable *attributable,
    const char *key,
    complex float *value)
{
    return openPMD_Attributable_getAttribute_cfloat2(
        attributable, key, (float *)value);
}

bool openPMD_Attributable_getAttribute_cdouble(
    const openPMD_Attributable *attributable,
    const char *key,
    complex double *value)
{
    return openPMD_Attributable_getAttribute_cdouble2(
        attributable, key, (double *)value);
}

bool openPMD_Attributable_getAttribute_clong_double(
    const openPMD_Attributable *attributable,
    const char *key,
    complex long double *value)
{
    return openPMD_Attributable_getAttribute_clong_double2(
        attributable, key, (long double *)value);
}

bool openPMD_Attributable_getAttribute_vec_cfloat(
    const openPMD_Attributable *attributable,
    const char *key,
    complex float **values,
    size_t *size)
{
    float *real_values;
    const bool did_exist = openPMD_Attributable_getAttribute_vec_cfloat2(
        attributable, key, (float **)&real_values, size);
    if (!did_exist)
        return false;
    *values = (complex float *)real_values;
    return true;
}

bool openPMD_Attributable_getAttribute_vec_cdouble(
    const openPMD_Attributable *attributable,
    const char *key,
    complex double **values,
    size_t *size)
{
    double *real_values;
    const bool did_exist = openPMD_Attributable_getAttribute_vec_cdouble2(
        attributable, key, (double **)&real_values, size);
    if (!did_exist)
        return false;
    *values = (complex double *)real_values;
    return true;
}

bool openPMD_Attributable_getAttribute_vec_clong_double(
    const openPMD_Attributable *attributable,
    const char *key,
    complex long double **values,
    size_t *size)
{
    long double *real_values;
    const bool did_exist = openPMD_Attributable_getAttribute_vec_clong_double2(
        attributable, key, (long double **)&real_values, size);
    if (!did_exist)
        return false;
    *values = (complex long double *)real_values;
    return true;
}
