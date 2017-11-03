#pragma once


#include <iostream>
#include <memory>

#include "Attributable.hpp"
#include "Container.hpp"
#include "RecordComponent.hpp"


template< typename T_elem >
class BaseRecord : public Container< T_elem >
{
public:
    using key_type = typename Container< T_elem >::key_type;
    using mapped_type = typename Container< T_elem >::mapped_type;
    using value_type = typename Container< T_elem >::value_type;
    using size_type = typename Container< T_elem >::size_type;
    using difference_type = typename Container< T_elem >::difference_type;
    using key_equal = typename Container< T_elem >::key_equal;
    using allocator_type = typename Container< T_elem >::allocator_type;
    using reference = typename Container< T_elem >::reference;
    using const_reference = typename Container< T_elem >::const_reference;
    using pointer = typename Container< T_elem >::pointer;
    using const_pointer = typename Container< T_elem >::const_pointer;
    using iterator = typename Container< T_elem >::iterator;
    using const_iterator = typename Container< T_elem >::const_iterator;
    
    enum class UnitDimension : uint8_t
    {
        L = 0, M, T, I, theta, N, J
    };

    BaseRecord(BaseRecord const& b);
    virtual ~BaseRecord() { }

    mapped_type& operator[](key_type const& key) override;
    mapped_type& operator[](key_type&& key) override;
    size_type erase(key_type const& key) override;

    virtual std::array< double, 7 > unitDimension() const;

protected:
    BaseRecord();

    virtual void read() = 0;
    void readBase();

    bool m_containsScalar;

private:
    virtual void flush(std::string const&) = 0;
};  //BaseRecord


class Record : public BaseRecord< RecordComponent >
{
    friend class Container< Record >;
    friend class Iteration;
    friend class ParticleSpecies;

public:
    Record(Record const&);
    virtual ~Record();

    Record& setUnitDimension(std::map< Record::UnitDimension, double > const&);

    template< typename T >
    T timeOffset() const;
    template< typename T >
    Record& setTimeOffset(T);

private:
    Record();

    void flush(std::string const&) override;
    void read() override;
};  //Record


template< typename T_elem >
BaseRecord< T_elem >::BaseRecord(BaseRecord const& b)
        : Container< T_elem >(b),
          m_containsScalar{b.m_containsScalar}
{ }

template< typename T_elem >
BaseRecord< T_elem >::BaseRecord()
        : m_containsScalar{false}
{
    this->setAttribute("unitDimension",
                       std::array< double, 7 >{{0., 0., 0., 0., 0., 0., 0.}});
    this->setAttribute("timeOffset",
                       static_cast<float>(0));
}

template< typename T_elem >
inline void
BaseRecord< T_elem >::readBase()
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

template< typename T_elem >
inline typename BaseRecord< T_elem >::mapped_type&
BaseRecord< T_elem >::operator[](key_type const& key)
{
    auto it = this->find(key);
    if( it != this->end() )
        return it->second;
    else
    {
        bool scalar = (key == RecordComponent::SCALAR);
        if( (scalar && !Container< T_elem >::empty() && !m_containsScalar) || (m_containsScalar && !scalar) )
            throw std::runtime_error("A scalar component can not be contained at "
                                     "the same time as one or more regular components.");

        mapped_type & ret = Container< T_elem >::operator[](key);
        if( scalar )
        {
            m_containsScalar = true;
            ret.parent = this->parent;
        }
        return ret;
    }
}

template< typename T_elem >
inline typename BaseRecord< T_elem >::mapped_type&
BaseRecord< T_elem >::operator[](key_type&& key)
{
    auto it = this->find(key);
    if( it != this->end() )
        return it->second;
    else
    {
        bool scalar = (key == RecordComponent::SCALAR);
        if( (scalar && !Container< T_elem >::empty() && !m_containsScalar) || (m_containsScalar && !scalar) )
            throw std::runtime_error("A scalar component can not be contained at "
                                             "the same time as one or more regular components.");

        mapped_type& ret = Container< T_elem >::operator[](std::move(key));
        if( scalar )
        {
            m_containsScalar = true;
            ret.parent = this->parent;
        }
        return ret;
    }
}

template< typename T_elem >
inline typename BaseRecord< T_elem >::size_type
BaseRecord< T_elem >::erase(key_type const& key)
{
    bool scalar = (key == RecordComponent::SCALAR);
    size_type res;
    if( !scalar || (scalar && this->at(key).m_isConstant) )
        res = Container< T_elem >::erase(key);
    else
    {
        mapped_type& rc = this->find(RecordComponent::SCALAR)->second;
        if( rc.written )
        {
            Parameter< Operation::DELETE_DATASET > dataset_parameter;
            dataset_parameter.name = ".";
            this->IOHandler->enqueue(IOTask(&rc, dataset_parameter));
            this->IOHandler->flush();
        }
        res = Container< T_elem >::erase(key);
    }

    if( scalar )
    {
        this->written = false;
        this->abstractFilePosition.reset();
        this->m_containsScalar = false;
    }
    return res;
}

template< typename T_elem >
inline std::array< double, 7 >
BaseRecord< T_elem >::unitDimension() const
{
    return Attributable::getAttribute("unitDimension").get< std::array< double, 7 > >();
}

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

