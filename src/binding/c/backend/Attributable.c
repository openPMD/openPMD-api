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
