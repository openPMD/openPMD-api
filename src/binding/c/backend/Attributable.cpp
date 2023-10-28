#include <openPMD/binding/c/backend/Attributable.h>

#include <openPMD/backend/Attributable.hpp>

#include <cstdlib>
#include <cstring>
#include <string>

openPMD_Attributable *openPMD_Attributable_new()
{
    const auto cxx_attributable = new openPMD::Attributable();
    return (openPMD_Attributable *)cxx_attributable;
}

void openPMD_Attributable_delete(openPMD_Attributable *attributable)
{
    const auto cxx_attributable = (openPMD::Attributable *)attributable;
    delete cxx_attributable;
}

#define DEFINE_SETATTTRIBUTE(NAME, TYPE)                                       \
    bool openPMD_Attributable_setAttribute_##NAME(                             \
        openPMD_Attributable *attributable, const char *key, TYPE value)       \
    {                                                                          \
        const auto cxx_attributable = (openPMD::Attributable *)attributable;   \
        return cxx_attributable->setAttribute(std::string(key), value);        \
    }
#define DEFINE_SETATTTRIBUTE2(NAME, TYPE)                                      \
    bool openPMD_Attributable_setAttribute_##NAME##2(                          \
        openPMD_Attributable * attributable,                                   \
        const char *key,                                                       \
        TYPE value_re,                                                         \
        TYPE value_im)                                                         \
    {                                                                          \
        const auto cxx_attributable = (openPMD::Attributable *)attributable;   \
        return cxx_attributable->setAttribute(                                 \
            std::string(key), std::complex<TYPE>(value_re, value_im));         \
    }
DEFINE_SETATTTRIBUTE(char, char)
DEFINE_SETATTTRIBUTE(uchar, unsigned char)
DEFINE_SETATTTRIBUTE(schar, signed char)
DEFINE_SETATTTRIBUTE(short, short)
DEFINE_SETATTTRIBUTE(int, int)
DEFINE_SETATTTRIBUTE(long, long)
DEFINE_SETATTTRIBUTE(longlong, long long)
DEFINE_SETATTTRIBUTE(ushort, unsigned short)
DEFINE_SETATTTRIBUTE(uint, unsigned int)
DEFINE_SETATTTRIBUTE(ulong, unsigned long)
DEFINE_SETATTTRIBUTE(ulonglong, unsigned long long)
DEFINE_SETATTTRIBUTE(float, float)
DEFINE_SETATTTRIBUTE(double, double)
DEFINE_SETATTTRIBUTE(long_double, long double)
DEFINE_SETATTTRIBUTE2(cfloat, float)
DEFINE_SETATTTRIBUTE2(cdouble, double)
DEFINE_SETATTTRIBUTE2(clong_double, long double)
DEFINE_SETATTTRIBUTE(string, const char *)
DEFINE_SETATTTRIBUTE(bool, bool)
#undef DEFINE_SETATTTRIBUTE
#undef DEFINE_SETATTTRIBUTE2

openPMD_Datatype openPMD_Attributable_attributeDatatype(
    const openPMD_Attributable *attributable, const char *key)
{
    const auto cxx_attributable = (const openPMD::Attributable *)attributable;
    const std::string cxx_key = std::string(key);
    if (!cxx_attributable->containsAttribute(cxx_key))
        return (openPMD_Datatype)openPMD::Datatype::UNDEFINED;
    const openPMD::Attribute cxx_attribute =
        cxx_attributable->getAttribute(cxx_key);
    const openPMD::Datatype cxx_datatype = cxx_attribute.dtype;
    return (openPMD_Datatype)cxx_datatype;
}

#define DEFINE_GETATTRIBUTE(NAME, TYPE)                                        \
    bool openPMD_Attributable_getAttribute_##NAME(                             \
        const openPMD_Attributable *attributable,                              \
        const char *key,                                                       \
        TYPE *value)                                                           \
    {                                                                          \
        const auto cxx_attributable =                                          \
            (const openPMD::Attributable *)attributable;                       \
        const auto cxx_attribute =                                             \
            cxx_attributable->getAttribute(std::string(key));                  \
        const auto cxx_value = cxx_attribute.getOptional<TYPE>();              \
        if (!cxx_value)                                                        \
            return false;                                                      \
        *value = *cxx_value;                                                   \
        return true;                                                           \
    }
#define DEFINE_GETATTRIBUTE2(NAME, TYPE)                                       \
    bool openPMD_Attributable_getAttribute_##NAME##2(                          \
        const openPMD_Attributable *attributable,                              \
        const char *key,                                                       \
        TYPE *value)                                                           \
    {                                                                          \
        const auto cxx_attributable =                                          \
            (const openPMD::Attributable *)attributable;                       \
        const auto cxx_attribute =                                             \
            cxx_attributable->getAttribute(std::string(key));                  \
        const auto cxx_value =                                                 \
            cxx_attribute.getOptional<std::complex<TYPE>>();                   \
        if (!cxx_value)                                                        \
            return false;                                                      \
        value[0] = cxx_value->real();                                          \
        value[1] = cxx_value->imag();                                          \
        return true;                                                           \
    }

DEFINE_GETATTRIBUTE(char, char)
DEFINE_GETATTRIBUTE(uchar, unsigned char)
DEFINE_GETATTRIBUTE(schar, signed char)
DEFINE_GETATTRIBUTE(short, short)
DEFINE_GETATTRIBUTE(int, int)
DEFINE_GETATTRIBUTE(long, long)
DEFINE_GETATTRIBUTE(longlong, long long)
DEFINE_GETATTRIBUTE(ushort, unsigned short)
DEFINE_GETATTRIBUTE(uint, unsigned int)
DEFINE_GETATTRIBUTE(ulong, unsigned long)
DEFINE_GETATTRIBUTE(ulonglong, unsigned long long)
DEFINE_GETATTRIBUTE(float, float)
DEFINE_GETATTRIBUTE(double, double)
DEFINE_GETATTRIBUTE(long_double, long double)
DEFINE_GETATTRIBUTE2(cfloat, float)
DEFINE_GETATTRIBUTE2(cdouble, double)
DEFINE_GETATTRIBUTE2(clong_double, long double)
DEFINE_GETATTRIBUTE(bool, bool)
#undef DEFINE_GETATTRIBUTE
#undef DEFINE_GETATTRIBUTE2

bool openPMD_Attributable_getAttribute_string(
    const openPMD_Attributable *attributable, const char *key, char **value)
{
    const auto cxx_attributable = (const openPMD::Attributable *)attributable;
    const auto cxx_attribute = cxx_attributable->getAttribute(std::string(key));
    const auto cxx_value = cxx_attribute.getOptional<std::string>();
    if (!cxx_value)
        return false;
    *value = strdup(cxx_value->c_str());
    return true;
}

bool openPMD_Attributable_deleteAttribute(
    openPMD_Attributable *attributable, const char *key)
{
    const auto cxx_attributable = (openPMD::Attributable *)attributable;
    return cxx_attributable->deleteAttribute(std::string(key));
}

char **openPMD_Attributable_attributes(const openPMD_Attributable *attributable)
{
    const auto cxx_attributable = (const openPMD::Attributable *)attributable;
    const auto cxx_attributes = cxx_attributable->attributes();
    const std::size_t num_attributes = cxx_attributes.size();
    char **const attributes =
        (char **)malloc((num_attributes + 1) * sizeof *attributes);
    for (std::size_t n = 0; n < num_attributes; ++n)
        attributes[n] = strdup(cxx_attributes[n].c_str());
    attributes[num_attributes] = nullptr;
    return attributes;
}

size_t
openPMD_Attributable_numAttributes(const openPMD_Attributable *attributable)
{
    const auto cxx_attributable = (const openPMD::Attributable *)attributable;
    return cxx_attributable->numAttributes();
}

bool openPMD_Attributable_containsAttribute(
    const openPMD_Attributable *attributable, const char *key)
{
    const auto cxx_attributable = (const openPMD::Attributable *)attributable;
    return cxx_attributable->containsAttribute(std::string(key));
}

// result must be freed
char *openPMD_Attributable_comment(const openPMD_Attributable *attributable)
{
    const auto cxx_attributable = (const openPMD::Attributable *)attributable;
    return strdup(cxx_attributable->comment().c_str());
}

void openPMD_Attributable_setComment(
    openPMD_Attributable *attributable, const char *comment)
{
    const auto cxx_attributable = (openPMD::Attributable *)attributable;
    cxx_attributable->setComment(std::string(comment));
}

// backendConfig may be NULL
void openPMD_Attributable_seriesFlush(
    openPMD_Attributable *attributable, const char *backendConfig)
{
    const auto cxx_attributable = (openPMD::Attributable *)attributable;
    cxx_attributable->seriesFlush(
        std::string(backendConfig ? backendConfig : "{}"));
}

void openPMD_Attributable_MyPath_free(openPMD_Attributable_MyPath *myPath)
{
    free(myPath->directory);
    free(myPath->seriesName);
    free(myPath->seriesExtension);
    for (size_t n = 0; myPath->group[n]; ++n)
        free(myPath->group[n]);
    free(myPath->group);
}

char *
openPMD_Attributable_MyPath_filePath(const openPMD_Attributable_MyPath *myPath)
{
    openPMD::Attributable::MyPath cxx_myPath;
    cxx_myPath.directory = std::string(myPath->directory);
    cxx_myPath.seriesName = std::string(myPath->seriesName);
    cxx_myPath.seriesExtension = std::string(myPath->seriesExtension);
    for (size_t n = 0; myPath->group[n]; ++n)
        cxx_myPath.group.emplace_back(myPath->group[n]);
    cxx_myPath.access = openPMD::Access(myPath->access);
    return strdup(cxx_myPath.filePath().c_str());
}

openPMD_Attributable_MyPath *
openPMD_Attributable_myPath(const openPMD_Attributable *attributable)
{
    const auto cxx_attributable = (const openPMD::Attributable *)attributable;
    const auto cxx_myPath = cxx_attributable->myPath();
    openPMD_Attributable_MyPath *myPath =
        (openPMD_Attributable_MyPath *)malloc(sizeof *myPath);
    myPath->directory = strdup(cxx_myPath.directory.c_str());
    myPath->seriesName = strdup(cxx_myPath.seriesName.c_str());
    myPath->seriesExtension = strdup(cxx_myPath.seriesExtension.c_str());
    const size_t size = cxx_myPath.group.size();
    myPath->group = (char **)malloc((size + 1) * sizeof *myPath->group);
    for (size_t n = 0; n < size; ++n)
        myPath->group[n] = strdup(cxx_myPath.group[n].c_str());
    myPath->group[size] = nullptr;
    myPath->access = openPMD_Access(cxx_myPath.access);
    return myPath;
}
