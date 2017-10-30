#pragma once


#include <iostream>
#include <memory>

#include "Attributable.hpp"
#include "Container.hpp"
#include "RecordComponent.hpp"


template< typename T_elem >
class BaseRecord : public Container< T_elem >
{
protected:
    BaseRecord()
            : m_containsScalar{false}
    {
        this->setAttribute("unitDimension",
                           std::array< double, 7 >{{0., 0., 0., 0., 0., 0., 0.}});
        this->setAttribute("timeOffset",
                           static_cast<float>(0));
    }

    virtual void read() = 0;
    void readBase()
    {
        using DT = Datatype;
        Parameter< Operation::READ_ATT > attribute_parameter;

        attribute_parameter.name = "unitDimension";
        this->IOHandler->enqueue(IOTask(this, attribute_parameter));
        this->IOHandler->flush();
        if( *attribute_parameter.dtype == DT::ARR_DBL_7 )
            this->setAttribute("unitDimension", Attribute(*attribute_parameter.resource).get< std::array< double, 7 > >());
        else
            throw std::runtime_error("Unexpected Attribute datatype for 'unitDimension'");

        attribute_parameter.name = "timeOffset";
        this->IOHandler->enqueue(IOTask(this, attribute_parameter));
        this->IOHandler->flush();
        if( *attribute_parameter.dtype == DT::FLOAT )
            this->setAttribute("timeOffset", Attribute(*attribute_parameter.resource).get< float >());
        else if( *attribute_parameter.dtype == DT::DOUBLE )
            this->setAttribute("timeOffset", Attribute(*attribute_parameter.resource).get< double >());
        else
            throw std::runtime_error("Unexpected Attribute datatype for 'timeOffset'");
    }
    bool m_containsScalar;

public:
    enum class UnitDimension : uint8_t
    {
        L = 0, M, T, I, theta, N, J
    };

    BaseRecord(BaseRecord const& b)
            : Container< T_elem >(b),
              m_containsScalar{b.m_containsScalar}
    { }
    virtual ~BaseRecord() { }

    // TODO
    // references can be handles polymorphically
    // making the overriding in subclasses unnecessarry
    //virtual T_elem& operator[](std::string key);

    virtual std::array< double, 7 > unitDimension() const = 0;

private:
    virtual void flush(std::string const&) = 0;
};  //BaseRecord


class Record : public BaseRecord< RecordComponent >
{
    friend class Container< Record >;
    friend class Iteration;
    friend class ParticleSpecies;

private:
    Record();

public:
    Record(Record const&);
    virtual ~Record();

    //Specialize access to elements for RecordComponent::SCALAR's
    RecordComponent& operator[](std::string key);
    size_type erase(std::string const& key);

    std::array< double, 7 > unitDimension() const;
    Record& setUnitDimension(std::map< Record::UnitDimension, double > const&);

    template< typename T >
    T timeOffset() const;
    template< typename T >
    Record& setTimeOffset(T);

private:
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
    dirty = true;
    return *this;
}

