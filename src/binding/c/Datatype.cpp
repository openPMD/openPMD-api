#include <openPMD/binding/c/Datatype.h>

#include <openPMD/Datatype.hpp>

#include <stdlib.h>

static const std::vector<openPMD_Datatype> &getDatatypes()
{
    static const std::vector<openPMD_Datatype> c_datatypes = []() {
        const auto types = openPMD::openPMD_Datatypes();
        std::vector<openPMD_Datatype> c_types(types.size());
        for (std::size_t n = 0; n < types.size(); ++n)
            c_types[n] = openPMD_Datatype(types[n]);
        return c_types;
    }();
    return c_datatypes;
}
const openPMD_Datatype *openPMD_Datatypes()
{
    return getDatatypes().data();
}
size_t openPMD_DatatypesSize()
{
    return getDatatypes().size();
}

size_t openPMD_toBytes(openPMD_Datatype datatype)
{
    return openPMD::toBytes(openPMD::Datatype(datatype));
}

size_t openPMD_toBits(openPMD_Datatype datatype)
{
    return openPMD::toBits(openPMD::Datatype(datatype));
}

bool openPMD_isVector(openPMD_Datatype datatype)
{
    return openPMD::isVector(openPMD::Datatype(datatype));
}

bool openPMD_isFloatingPoint(openPMD_Datatype datatype)
{
    return openPMD::isFloatingPoint(openPMD::Datatype(datatype));
}

bool openPMD_isComplexFloatingPoint(openPMD_Datatype datatype)
{
    return openPMD::isComplexFloatingPoint(openPMD::Datatype(datatype));
}

bool openPMD_isInteger(openPMD_Datatype datatype)
{
    const auto [isint, issgn] = openPMD::isInteger(openPMD::Datatype(datatype));
    return isint;
}

bool openPMD_isSigned(openPMD_Datatype datatype)
{
    const auto [isint, issgn] = openPMD::isInteger(openPMD::Datatype(datatype));
    return issgn;
}

bool openPMD_isChar(openPMD_Datatype datatype)
{
    return openPMD::isChar(openPMD::Datatype(datatype));
}

bool openPMD_isSame(openPMD_Datatype datatype1, openPMD_Datatype datatype2)
{
    return openPMD::isSame(
        openPMD::Datatype(datatype1), openPMD::Datatype(datatype2));
}

openPMD_Datatype openPMD_basicDatatype(openPMD_Datatype datatype)
{
    return openPMD_Datatype(
        openPMD::basicDatatype(openPMD::Datatype(datatype)));
}

openPMD_Datatype openPMD_toVectorType(openPMD_Datatype datatype)
{
    return openPMD_Datatype(openPMD::toVectorType(openPMD::Datatype(datatype)));
}

const char *openPMD_datatypeToString(openPMD_Datatype datatype)
{
    switch (datatype)
    {
    case openPMD_Datatype_CHAR:
        return "CHAR";
    case openPMD_Datatype_UCHAR:
        return "UCHAR";
    case openPMD_Datatype_SCHAR:
        return "SCHAR";
    case openPMD_Datatype_SHORT:
        return "SHORT";
    case openPMD_Datatype_INT:
        return "INT";
    case openPMD_Datatype_LONG:
        return "LONG";
    case openPMD_Datatype_LONGLONG:
        return "LONGLONG";
    case openPMD_Datatype_USHORT:
        return "USHORT";
    case openPMD_Datatype_UINT:
        return "UINT";
    case openPMD_Datatype_ULONG:
        return "ULONG";
    case openPMD_Datatype_ULONGLONG:
        return "ULONGLONG";
    case openPMD_Datatype_FLOAT:
        return "FLOAT";
    case openPMD_Datatype_DOUBLE:
        return "DOUBLE";
    case openPMD_Datatype_LONG_DOUBLE:
        return "LONG_DOUBLE";
    case openPMD_Datatype_CFLOAT:
        return "CFLOAT";
    case openPMD_Datatype_CDOUBLE:
        return "CDOUBLE";
    case openPMD_Datatype_CLONG_DOUBLE:
        return "CLONG_DOUBLE";
    case openPMD_Datatype_STRING:
        return "STRING";
    case openPMD_Datatype_VEC_CHAR:
        return "VEC_CHAR";
    case openPMD_Datatype_VEC_SHORT:
        return "VEC_SHORT";
    case openPMD_Datatype_VEC_INT:
        return "VEC_INT";
    case openPMD_Datatype_VEC_LONG:
        return "VEC_LONG";
    case openPMD_Datatype_VEC_LONGLONG:
        return "VEC_LONGLONG";
    case openPMD_Datatype_VEC_UCHAR:
        return "VEC_UCHAR";
    case openPMD_Datatype_VEC_USHORT:
        return "VEC_USHORT";
    case openPMD_Datatype_VEC_UINT:
        return "VEC_UINT";
    case openPMD_Datatype_VEC_ULONG:
        return "VEC_ULONG";
    case openPMD_Datatype_VEC_ULONGLONG:
        return "VEC_ULONGLONG";
    case openPMD_Datatype_VEC_FLOAT:
        return "VEC_FLOAT";
    case openPMD_Datatype_VEC_DOUBLE:
        return "VEC_DOUBLE";
    case openPMD_Datatype_VEC_LONG_DOUBLE:
        return "VEC_LONG_DOUBLE";
    case openPMD_Datatype_VEC_CFLOAT:
        return "VEC_CFLOAT";
    case openPMD_Datatype_VEC_CDOUBLE:
        return "VEC_CDOUBLE";
    case openPMD_Datatype_VEC_CLONG_DOUBLE:
        return "VEC_CLONG_DOUBLE";
    case openPMD_Datatype_VEC_SCHAR:
        return "VEC_SCHAR";
    case openPMD_Datatype_VEC_STRING:
        return "VEC_STRING";
    case openPMD_Datatype_ARR_DBL_7:
        return "ARR_DBL_7";
    case openPMD_Datatype_BOOL:
        return "BOOL";
    case openPMD_Datatype_UNDEFINED:
        return "UNDEFINED";
    }
    abort();
}

openPMD_Datatype openPMD_stringToDatatype(const char *string)
{
    return openPMD_Datatype(openPMD::stringToDatatype(std::string(string)));
}
