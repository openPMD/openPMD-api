#pragma once

#include "openPMD/Dataset.hpp"
#include "openPMD/Datatype.hpp"

#include <functional>
#include <memory>
#include <utility>


namespace openPMD
{
namespace auxiliary
{
std::unique_ptr< void, std::function< void(void*) > >
allocatePtr(Datatype dtype, Extent const& e);

std::unique_ptr< void, std::function< void(void*) > >
allocatePtr(Datatype dtype, size_t numPoints);

inline std::unique_ptr< void, std::function< void(void*) > >
allocatePtr(Datatype dtype, Extent const& e)
{
  size_t numPoints = 1;
  for( auto const& dimensionSize : e )
    numPoints *= dimensionSize;
  return std::move(allocatePtr(dtype, numPoints));
}

inline std::unique_ptr< void, std::function< void(void*) > >
allocatePtr(Datatype dtype, size_t numPoints)
{
    void* data = nullptr;
    std::function< void(void*) > del;
    switch( dtype )
    {
        using DT = Datatype;
        case DT::LONG_DOUBLE:
            data = new long double[numPoints];
            del = [](void* p){ delete[] static_cast< long double* >(p); p = nullptr; };
            break;
        case DT::DOUBLE:
            data = new double[numPoints];
            del = [](void* p){ delete[] static_cast< double* >(p); p = nullptr; };
            break;
        case DT::FLOAT:
            data = new float[numPoints];
            del = [](void* p){ delete[] static_cast< float* >(p); p = nullptr; };
            break;
        case DT::INT16:
            data = new int16_t[numPoints];
            del = [](void* p){ delete[] static_cast< int16_t* >(p); p = nullptr; };
            break;
        case DT::INT32:
            data = new int32_t[numPoints];
            del = [](void* p){ delete[] static_cast< int32_t* >(p); p = nullptr; };
            break;
        case DT::INT64:
            data = new int64_t[numPoints];
            del = [](void* p){ delete[] static_cast< int64_t* >(p); p = nullptr; };
            break;
        case DT::UINT16:
            data = new uint16_t[numPoints];
            del = [](void* p){ delete[] static_cast< uint16_t* >(p); p = nullptr; };
            break;
        case DT::UINT32:
            data = new uint32_t[numPoints];
            del = [](void* p){ delete[] static_cast< uint32_t* >(p); p = nullptr; };
            break;
        case DT::UINT64:
            data = new uint64_t[numPoints];
            del = [](void* p){ delete[] static_cast< uint64_t* >(p); p = nullptr; };
            break;
        case DT::CHAR:
            data = new char[numPoints];
            del = [](void* p){ delete[] static_cast< char* >(p); p = nullptr; };
            break;
        case DT::UCHAR:
            data = new unsigned char[numPoints];
            del = [](void* p){ delete[] static_cast< unsigned char* >(p); p = nullptr; };
            break;
        case DT::BOOL:
            data = new bool[numPoints];
            del = [](void* p){ delete[] static_cast< bool* >(p); p = nullptr; };
            break;
        case DT::UNDEFINED:
        default:
            throw std::runtime_error("Unknown Attribute datatype");
    }

    return std::move(std::unique_ptr< void, std::function< void(void*) > >(data, del));
}
} // auxiliary
} // openPMD
