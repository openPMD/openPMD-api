#include <openPMD/binding/c/backend/MeshRecordComponent.h>

#include <openPMD/backend/MeshRecordComponent.hpp>

#include <cstdlib>
#include <utility>

const openPMD_RecordComponent *
openPMD_MeshRecordComponent_getConstRecordComponent(
    const openPMD_MeshRecordComponent *meshComponent)
{
    const auto cxx_meshComponent =
        (const openPMD::MeshRecordComponent *)meshComponent;
    const auto cxx_component =
        (const openPMD::RecordComponent *)cxx_meshComponent;
    const auto component = (const openPMD_RecordComponent *)cxx_component;
    return component;
}

openPMD_RecordComponent *openPMD_MeshRecordComponent_getRecordComponent(
    openPMD_MeshRecordComponent *meshComponent)
{
    const auto cxx_meshComponent =
        (openPMD::MeshRecordComponent *)meshComponent;
    const auto cxx_component = (openPMD::RecordComponent *)cxx_meshComponent;
    const auto component = (openPMD_RecordComponent *)cxx_component;
    return component;
}

void openPMD_MeshRecordComponent_delete(openPMD_MeshRecordComponent *component)
{
    const auto cxx_component = (openPMD::MeshRecordComponent *)component;
    delete cxx_component;
}

double *openPMD_MeshRecordComponent_position(
    const openPMD_MeshRecordComponent *component)
{
    const auto cxx_component = (const openPMD::MeshRecordComponent *)component;
    const auto cxx_position = cxx_component->position<double>();
    double *position = (double *)malloc(cxx_position.size() * sizeof *position);
    return position;
}

size_t openPMD_MeshRecordComponent_positionSize(
    const openPMD_MeshRecordComponent *component)
{
    const auto cxx_component = (const openPMD::MeshRecordComponent *)component;
    const auto cxx_position = cxx_component->position<double>();
    return cxx_position.size();
}

void openPMD_MeshRecordComponent_setPosition(
    openPMD_MeshRecordComponent *component, const double *position, size_t size)
{
    const auto cxx_component = (openPMD::MeshRecordComponent *)component;
    std::vector<double> cxx_position(position, position + size);
    cxx_component->setPosition(std::move(cxx_position));
}

void openPMD_MeshRecordComponent_makeConstant(
    openPMD_MeshRecordComponent *component,
    const void *data,
    openPMD_Datatype datatype)
{
    const auto cxx_component = (openPMD::MeshRecordComponent *)component;
    switch (datatype)
    {
    case openPMD_Datatype_CHAR:
        cxx_component->makeConstant(*(const char *)data);
        break;
    case openPMD_Datatype_UCHAR:
        cxx_component->makeConstant(*(const unsigned char *)data);
        break;
    case openPMD_Datatype_SCHAR:
        cxx_component->makeConstant(*(const signed char *)data);
        break;
    case openPMD_Datatype_SHORT:
        cxx_component->makeConstant(*(const short *)data);
        break;
    case openPMD_Datatype_INT:
        cxx_component->makeConstant(*(const int *)data);
        break;
    case openPMD_Datatype_LONG:
        cxx_component->makeConstant(*(const long *)data);
        break;
    case openPMD_Datatype_LONGLONG:
        cxx_component->makeConstant(*(const long long *)data);
        break;
    case openPMD_Datatype_USHORT:
        cxx_component->makeConstant(*(const unsigned short *)data);
        break;
    case openPMD_Datatype_UINT:
        cxx_component->makeConstant(*(const unsigned int *)data);
        break;
    case openPMD_Datatype_ULONG:
        cxx_component->makeConstant(*(const unsigned long *)data);
        break;
    case openPMD_Datatype_ULONGLONG:
        cxx_component->makeConstant(*(const unsigned long long *)data);
        break;
    case openPMD_Datatype_FLOAT:
        cxx_component->makeConstant(*(const float *)data);
        break;
    case openPMD_Datatype_DOUBLE:
        cxx_component->makeConstant(*(const double *)data);
        break;
    case openPMD_Datatype_LONG_DOUBLE:
        cxx_component->makeConstant(*(const long double *)data);
        break;
    case openPMD_Datatype_CFLOAT:
        cxx_component->makeConstant(*(const std::complex<float> *)data);
        break;
    case openPMD_Datatype_CDOUBLE:
        cxx_component->makeConstant(*(const std::complex<double> *)data);
        break;
    case openPMD_Datatype_CLONG_DOUBLE:
        cxx_component->makeConstant(*(const std::complex<long double> *)data);
        break;
    case openPMD_Datatype_BOOL:
        cxx_component->makeConstant(*(const bool *)data);
        break;
    default:
        abort();
    }
}
