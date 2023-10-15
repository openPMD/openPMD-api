#include <openPMD/binding/c/backend/Attributable.h>

#include <openPMD/backend/Attributable.hpp>

#include <complex.h>

#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
openPMD_Attribute Attribute_cxx2c(const openPMD::Attribute &cxx_attribute)
{
    openPMD_Datatype datatype = (openPMD_Datatype)cxx_attribute.index();
    openPMD_Attribute attribute;
    attribute.datatype = datatype;
    switch (attribute.datatype)
    {
    case openPMD_Datatype_CHAR:
        attribute.char_data = cxx_attribute.get<char>();
        break;
    case openPMD_Datatype_UCHAR:
        attribute.uchar_data = cxx_attribute.get<unsigned char>();
        break;
    case openPMD_Datatype_SCHAR:
        attribute.schar_data = cxx_attribute.get<signed char>();
        break;
    case openPMD_Datatype_SHORT:
        attribute.short_data = cxx_attribute.get<short>();
        break;
    case openPMD_Datatype_INT:
        attribute.int_data = cxx_attribute.get<int>();
        break;
    case openPMD_Datatype_LONG:
        attribute.long_data = cxx_attribute.get<long>();
        break;
    case openPMD_Datatype_LONGLONG:
        attribute.longlong_data = cxx_attribute.get<long long>();
        break;
    case openPMD_Datatype_USHORT:
        attribute.ushort_data = cxx_attribute.get<unsigned short>();
        break;
    case openPMD_Datatype_UINT:
        attribute.uint_data = cxx_attribute.get<unsigned int>();
        break;
    case openPMD_Datatype_ULONG:
        attribute.ulong_data = cxx_attribute.get<unsigned long>();
        break;
    case openPMD_Datatype_ULONGLONG:
        attribute.ulonglong_data = cxx_attribute.get<unsigned long long>();
        break;
    case openPMD_Datatype_FLOAT:
        attribute.float_data = cxx_attribute.get<float>();
        break;
    case openPMD_Datatype_DOUBLE:
        attribute.double_data = cxx_attribute.get<double>();
        break;
    case openPMD_Datatype_LONG_DOUBLE:
        attribute.long_double_data = cxx_attribute.get<long double>();
        break;
    case openPMD_Datatype_CFLOAT: {
        const auto value = cxx_attribute.get<std::complex<float>>();
        std::memcpy(&attribute.cfloat_data, &value, sizeof value);
        break;
    }
    case openPMD_Datatype_CDOUBLE: {
        const auto value = cxx_attribute.get<std::complex<double>>();
        std::memcpy(&attribute.cdouble_data, &value, sizeof value);
        break;
    }
    case openPMD_Datatype_CLONG_DOUBLE: {
        const auto value = cxx_attribute.get<std::complex<long double>>();
        std::memcpy(&attribute.clong_double_data, &value, sizeof value);
        break;
    }
    case openPMD_Datatype_BOOL:
        attribute.bool_data = cxx_attribute.get<bool>();
        break;
    default:
        std::abort();
    }
    return attribute;
}
} // namespace

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
DEFINE_SETATTTRIBUTE(cfloat, std::complex<float>)
DEFINE_SETATTTRIBUTE(cdouble, std::complex<double>)
DEFINE_SETATTTRIBUTE(clong_double, std::complex<long double>)
DEFINE_SETATTTRIBUTE(bool, bool)
#undef DEFINE_SETATTTRIBUTE

openPMD_Attribute openPMD_Attributable_getAttribute(
    const openPMD_Attributable *attributable, const char *key)
{
    const auto cxx_attributable = (const openPMD::Attributable *)attributable;
    const openPMD::Attribute cxx_attribute =
        cxx_attributable->getAttribute(std::string(key));
    const openPMD_Attribute attribute = Attribute_cxx2c(cxx_attribute);
    return attribute;
}

bool openPMD_Attributable_deleteAttribute(
    openPMD_Attributable *attributable, const char *key)
{
    const auto cxx_attributable = (openPMD::Attributable *)attributable;
    return cxx_attributable->deleteAttribute(std::string(key));
}
