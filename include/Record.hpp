#pragma once


#include <memory>

#include <boost/variant.hpp>

#include "Attributable.hpp"
#include "Container.hpp"
#include "RecordComponent.hpp"


class Record : public Container< RecordComponent >
{
    template<
            typename T,
            typename T_key
    >
    friend class Container;
    friend class Mesh;

private:
//    Record(Extent ext, std::initializer_list< std::string > comps);
    Record();

public:
    enum class UnitDimension : uint8_t
    {
        L = 0, M, T, I, theta, N, J
    };

    Record(Record const&);

    std::array< double, 7 > unitDimension() const;
    Record& setUnitDimension(std::map< Record::UnitDimension, double > const&);

    float timeOffset() const;
    Record& setTimeOffset(float const);

protected:
//    bool m_isComponent;
//    Extent m_extent;
};  //Record
