#include <openPMD/binding/c/RecordComponent.h>

#include <openPMD/RecordComponent.hpp>

#include <stdlib.h>
#include <string.h>

#include <complex>
#include <string>
#include <utility>

const openPMD_BaseRecordComponent *
openPMD_RecordComponent_getConstBaseRecordComponent(
    const openPMD_RecordComponent *component)
{
    const auto cxx_component = (const openPMD::RecordComponent *)component;
    const auto cxx_baseComponent =
        (const openPMD::BaseRecordComponent *)cxx_component;
    const auto baseComponent =
        (const openPMD_BaseRecordComponent *)cxx_baseComponent;
    return baseComponent;
}

openPMD_BaseRecordComponent *openPMD_RecordComponent_getBaseRecordComponent(
    openPMD_RecordComponent *component)
{
    const auto cxx_component = (openPMD::RecordComponent *)component;
    const auto cxx_baseComponent =
        (openPMD::BaseRecordComponent *)cxx_component;
    const auto baseComponent = (openPMD_BaseRecordComponent *)cxx_baseComponent;
    return baseComponent;
}

void openPMD_RecordComponent_setUnitSI(
    openPMD_RecordComponent *component, double unit)
{
    const auto cxx_component = (openPMD::RecordComponent *)component;
    cxx_component->setUnitSI(unit);
}

void openPMD_RecordComponent_resetDataset(
    openPMD_RecordComponent *component,
    openPMD_Datatype datatype,
    const uint64_t *extent,
    size_t rank,
    const char *options)
{
    const auto cxx_component = (openPMD::RecordComponent *)component;
    const openPMD::Dataset cxx_dataset(
        openPMD::Datatype(datatype),
        openPMD::Extent(extent, extent + rank),
        std::string(options ? options : "{}"));
    cxx_component->resetDataset(std::move(cxx_dataset));
}

uint8_t openPMD_RecordComponent_getDimensionality(
    const openPMD_RecordComponent *component)
{
    const auto cxx_component = (const openPMD::RecordComponent *)component;
    return cxx_component->getDimensionality();
}

uint64_t *
openPMD_RecordComponent_getExtent(const openPMD_RecordComponent *component)
{
    const auto cxx_component = (const openPMD::RecordComponent *)component;
    const auto cxx_extent = cxx_component->getExtent();
    uint64_t *extent = (uint64_t *)malloc(cxx_extent.size() * sizeof *extent);
    memcpy(extent, cxx_extent.data(), cxx_extent.size() * sizeof *extent);
    return extent;
}

size_t
openPMD_RecordComponent_getExtentSize(const openPMD_RecordComponent *component)
{
    const auto cxx_component = (const openPMD::RecordComponent *)component;
    const auto cxx_extent = cxx_component->getExtent();
    return cxx_extent.size();
}

void openPMD_RecordComponent_makeConstant(
    openPMD_RecordComponent *component,
    openPMD_Datatype datatype,
    const void *data)
{
    const auto cxx_component = (openPMD::RecordComponent *)component;
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

void openPMD_RecordComponent_makeEmpty(
    openPMD_RecordComponent *component,
    openPMD_Datatype datatype,
    uint8_t dimensions)
{
    const auto cxx_component = (openPMD::RecordComponent *)component;
    cxx_component->makeEmpty(openPMD::Datatype(datatype), dimensions);
}

bool openPMD_RecordComponent_empty(const openPMD_RecordComponent *component)
{
    const auto cxx_component = (const openPMD::RecordComponent *)component;
    return cxx_component->empty();
}

void openPMD_RecordComponent_loadChunkRaw(
    openPMD_RecordComponent *component,
    void *data,
    openPMD_Datatype datatype,
    const uint64_t *offset,
    const uint64_t *extent,
    size_t size)
{
    const auto cxx_component = (openPMD::RecordComponent *)component;
    const openPMD::Offset cxx_offset(offset, offset + size);
    const openPMD::Extent cxx_extent(extent, extent + size);
    switch (datatype)
    {
    case openPMD_Datatype_CHAR:
        cxx_component->loadChunkRaw(
            (char *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_UCHAR:
        cxx_component->loadChunkRaw(
            (unsigned char *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_SCHAR:
        cxx_component->loadChunkRaw(
            (signed char *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_SHORT:
        cxx_component->loadChunkRaw(
            (short *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_INT:
        cxx_component->loadChunkRaw(
            (int *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_LONG:
        cxx_component->loadChunkRaw(
            (long *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_LONGLONG:
        cxx_component->loadChunkRaw(
            (long long *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_USHORT:
        cxx_component->loadChunkRaw(
            (unsigned short *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_UINT:
        cxx_component->loadChunkRaw(
            (unsigned int *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_ULONG:
        cxx_component->loadChunkRaw(
            (unsigned long *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_ULONGLONG:
        cxx_component->loadChunkRaw(
            (unsigned long long *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_FLOAT:
        cxx_component->loadChunkRaw(
            (float *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_DOUBLE:
        cxx_component->loadChunkRaw(
            (double *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_LONG_DOUBLE:
        cxx_component->loadChunkRaw(
            (long double *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_CFLOAT:
        cxx_component->loadChunkRaw(
            (std::complex<float> *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_CDOUBLE:
        cxx_component->loadChunkRaw(
            (std::complex<double> *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_CLONG_DOUBLE:
        cxx_component->loadChunkRaw(
            (std::complex<long double> *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_BOOL:
        cxx_component->loadChunkRaw(
            (bool *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    default:
        abort();
    }
}

void openPMD_RecordComponent_storeChunkRaw(
    openPMD_RecordComponent *component,
    void *data,
    openPMD_Datatype datatype,
    const uint64_t *offset,
    const uint64_t *extent,
    size_t size)
{
    const auto cxx_component = (openPMD::RecordComponent *)component;
    const openPMD::Offset cxx_offset(offset, offset + size);
    const openPMD::Extent cxx_extent(extent, extent + size);
    switch (datatype)
    {
    case openPMD_Datatype_CHAR:
        cxx_component->storeChunkRaw(
            (char *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_UCHAR:
        cxx_component->storeChunkRaw(
            (unsigned char *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_SCHAR:
        cxx_component->storeChunkRaw(
            (signed char *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_SHORT:
        cxx_component->storeChunkRaw(
            (short *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_INT:
        cxx_component->storeChunkRaw(
            (int *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_LONG:
        cxx_component->storeChunkRaw(
            (long *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_LONGLONG:
        cxx_component->storeChunkRaw(
            (long long *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_USHORT:
        cxx_component->storeChunkRaw(
            (unsigned short *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_UINT:
        cxx_component->storeChunkRaw(
            (unsigned int *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_ULONG:
        cxx_component->storeChunkRaw(
            (unsigned long *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_ULONGLONG:
        cxx_component->storeChunkRaw(
            (unsigned long long *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_FLOAT:
        cxx_component->storeChunkRaw(
            (float *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_DOUBLE:
        cxx_component->storeChunkRaw(
            (double *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_LONG_DOUBLE:
        cxx_component->storeChunkRaw(
            (long double *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    case openPMD_Datatype_CFLOAT:
        cxx_component->storeChunkRaw(
            (std::complex<float> *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_CDOUBLE:
        cxx_component->storeChunkRaw(
            (std::complex<double> *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_CLONG_DOUBLE:
        cxx_component->storeChunkRaw(
            (std::complex<long double> *)data,
            std::move(cxx_offset),
            std::move(cxx_extent));
        break;
    case openPMD_Datatype_BOOL:
        cxx_component->storeChunkRaw(
            (bool *)data, std::move(cxx_offset), std::move(cxx_extent));
        break;
    default:
        abort();
    }
}

char const *openPMD_RecordComponent_SCALAR()
{
    return openPMD::RecordComponent::SCALAR;
}
