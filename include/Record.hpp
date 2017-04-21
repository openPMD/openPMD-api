#pragma once


#include <memory>

#include <boost/variant.hpp>

#include "Attributable.hpp"
#include "Container.hpp"
#include "RecordComponent.hpp"

class Record : public Container< RecordComponent >
{
    friend class Mesh;
    friend class HDF5Writer;
public:

    enum class UnitDimension : size_t
    {
        L = 0, M, T, I, theta, N, J
    };

    static Record makeTensorRecord(Extent, std::initializer_list< std::string >);
    static Record makeVectorRecord(std::initializer_list< std::string >);
    static Record makeScalarRecord(Extent);
    static Record makeConstantRecord(Extent, std::initializer_list< int >, std::initializer_list< std::string >);

    Record(Extent ext, std::initializer_list< std::string > comps, bool isRecordComponent = false);
    Record(Record const &);
    Record();

    std::array< double, 7 > unitDimension() const;
    Record& setUnitDimension(std::map< Record::UnitDimension, double > const &);

    float timeOffset() const;
    Record& setTimeOffset(float const);

    RecordComponent scalar;

protected:
    bool m_isComponent;
    Extent m_extent;
};  //Record
