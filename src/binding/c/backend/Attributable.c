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
    return openPMD_Attributable_setAttribute_cfloat2(
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

bool openPMD_Attributable_getAttribute_cfloat(
    const openPMD_Attributable *attributable,
    const char *key,
    complex float *value)
{
    return openPMD_Attributable_getAttribute_cfloat2(
        attributable, key, (float *)(void *)value, (float *)(void *)value + 1);
}

bool openPMD_Attributable_getAttribute_cdouble(
    const openPMD_Attributable *attributable,
    const char *key,
    complex double *value)
{
    return openPMD_Attributable_getAttribute_cdouble2(
        attributable,
        key,
        (double *)(void *)value,
        (double *)(void *)value + 1);
}

bool openPMD_Attributable_getAttribute_clong_double(
    const openPMD_Attributable *attributable,
    const char *key,
    complex long double *value)
{
    return openPMD_Attributable_getAttribute_clong_double2(
        attributable,
        key,
        (long double *)(void *)value,
        (long double *)(void *)value + 1);
}
