#pragma once

#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/auxiliary/Visibility.hpp"

#include <map>
#include <type_traits>
#include <string>


namespace openPMD
{
class OPENPMD_PUBLIC Record : public BaseRecord< RecordComponent >
{
    friend class Container< Record >;
    friend class Iteration;
    friend class ParticleSpecies;

public:
    Record(Record const&);
    virtual ~Record() override;

    Record& setUnitDimension(std::map< UnitDimension, double > const&);

    template< typename T >
    T timeOffset() const;
    template< typename T >
    Record& setTimeOffset(T);

private:
    Record();

    void flush(std::string const&) override;
    void read() override;
};  //Record


template< typename T >
inline T
Record::timeOffset() const
{ return readFloatingpoint< T >("timeOffset"); }

template< typename T >
inline Record&
Record::setTimeOffset(T to)
{
    static_assert(std::is_floating_point< T >::value, "Type of attribute must be floating point");

    setAttribute("timeOffset", to);
    return *this;
}
} // openPMD
