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

    std::shared_ptr< Dataset > m_dataset;
    std::shared_ptr< bool > m_isConstant;
};  //BaseRecordComponent
} // openPMD
