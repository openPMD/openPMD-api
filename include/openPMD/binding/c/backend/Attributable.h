#ifndef OPENPMD_ATTRIBUTABLE_H
#define OPENPMD_ATTRIBUTABLE_H

#include <openPMD/binding/c/IO/Access.h>
#include <openPMD/binding/c/backend/Attribute.h>

#include <complex.h>
#include <stdbool.h>

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
    bool openPMD_Attributable_setAttribute_cfloat(
        openPMD_Attributable *attributable,
        const char *key,
        _Complex float value);
    bool openPMD_Attributable_setAttribute_cdouble(
        openPMD_Attributable *attributable,
        const char *key,
        _Complex double value);
    bool openPMD_Attributable_setAttribute_clong_double(
        openPMD_Attributable *attributable,
        const char *key,
        _Complex long double value);
    bool openPMD_Attributable_setAttribute_bool(
        openPMD_Attributable *attributable, const char *key, bool value);

    openPMD_Attribute openPMD_Attributable_getAttribute(
        const openPMD_Attributable *attributable, const char *key);

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
