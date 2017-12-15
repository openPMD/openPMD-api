#pragma once


#include "Attributable.hpp"
#include "Dataset.hpp"


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

    Datatype getDatatype();

protected:
    BaseRecordComponent();

    Dataset m_dataset;
    bool m_isConstant;
};  //BaseRecordComponent