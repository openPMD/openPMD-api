#pragma once

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/Dataset.hpp"


namespace openPMD
{
class BaseRecordComponent : public Attributable
{
    template< typename T_elem >
    friend
    class BaseRecord;

public:
    virtual ~BaseRecordComponent()
    { }

    double unitSI() const;

    BaseRecordComponent& resetDatatype(Datatype);

    Datatype getDatatype() const;

protected:
    BaseRecordComponent();

    Dataset m_dataset;
    bool m_isConstant;
};  //BaseRecordComponent
} // openPMD
