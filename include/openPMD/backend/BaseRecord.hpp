/* Copyright 2017-2019 Fabian Koller
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "openPMD/backend/Container.hpp"
#include "openPMD/RecordComponent.hpp"

#include <array>
#include <string>
#include <stdexcept>


namespace openPMD
{
enum class UnitDimension : uint8_t
{
    L = 0, M, T, I, theta, N, J
};

template< typename T_elem >
class BaseRecord : public Container< T_elem >
{
    friend class Iteration;
    friend class ParticleSpecies;

public:
    using key_type = typename Container< T_elem >::key_type;
    using mapped_type = typename Container< T_elem >::mapped_type;
    using value_type = typename Container< T_elem >::value_type;
    using size_type = typename Container< T_elem >::size_type;
    using difference_type = typename Container< T_elem >::difference_type;
    using allocator_type = typename Container< T_elem >::allocator_type;
    using reference = typename Container< T_elem >::reference;
    using const_reference = typename Container< T_elem >::const_reference;
    using pointer = typename Container< T_elem >::pointer;
    using const_pointer = typename Container< T_elem >::const_pointer;
    using iterator = typename Container< T_elem >::iterator;
    using const_iterator = typename Container< T_elem >::const_iterator;

    BaseRecord(BaseRecord const& b);
    ~BaseRecord() override { }

    mapped_type& operator[](key_type const& key) override;
    mapped_type& operator[](key_type&& key) override;
    size_type erase(key_type const& key) override;

    virtual std::array< double, 7 > unitDimension() const;

protected:
    BaseRecord();

    void readBase();

    std::shared_ptr< bool > m_containsScalar;

private:
    virtual void flush(std::string const&) final;
    virtual void flush_impl(std::string const&) = 0;
    virtual void read() = 0;
};  //BaseRecord


template< typename T_elem >
BaseRecord< T_elem >::BaseRecord(BaseRecord const& b)
        : Container< T_elem >(b),
          m_containsScalar{b.m_containsScalar}
{ }

template< typename T_elem >
BaseRecord< T_elem >::BaseRecord()
        : Container< T_elem >(),
          m_containsScalar{std::make_shared< bool >(false)}
{
    this->setAttribute("unitDimension",
                       std::array< double, 7 >{{0., 0., 0., 0., 0., 0., 0.}});
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
        if( (scalar && !Container< T_elem >::empty() && !*m_containsScalar) || (*m_containsScalar && !scalar) )
            throw std::runtime_error("A scalar component can not be contained at "
                                     "the same time as one or more regular components.");

        mapped_type& ret = Container< T_elem >::operator[](key);
        if( scalar )
        {
            *m_containsScalar = true;
            ret.m_writable->parent = this->m_writable->parent;
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
        if( (scalar && !Container< T_elem >::empty() && !*m_containsScalar) || (*m_containsScalar && !scalar) )
            throw std::runtime_error("A scalar component can not be contained at "
                                     "the same time as one or more regular components.");

        mapped_type& ret = Container< T_elem >::operator[](std::move(key));
        if( scalar )
        {
            *m_containsScalar = true;
            ret.m_writable->parent = this->m_writable->parent;
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
    if( !scalar || (scalar && *this->at(key).m_isConstant) )
        res = Container< T_elem >::erase(key);
    else
    {
        mapped_type& rc = this->find(RecordComponent::SCALAR)->second;
        if( rc.written )
        {
            Parameter< Operation::DELETE_DATASET > dDelete;
            dDelete.name = ".";
            this->IOHandler->enqueue(IOTask(&rc, dDelete));
            this->IOHandler->flush();
        }
        res = Container< T_elem >::erase(key);
    }

    if( scalar )
    {
        this->written = false;
        this->m_writable->abstractFilePosition.reset();
        *this->m_containsScalar = false;
    }
    return res;
}

template< typename T_elem >
inline std::array< double, 7 >
BaseRecord< T_elem >::unitDimension() const
{
    return Attributable::getAttribute("unitDimension").template get< std::array< double, 7 > >();
}

template< typename T_elem >
inline void
BaseRecord< T_elem >::readBase()
{
    using DT = Datatype;
    Parameter< Operation::READ_ATT > aRead;

    aRead.name = "unitDimension";
    this->IOHandler->enqueue(IOTask(this, aRead));
    this->IOHandler->flush();
    if( *aRead.dtype == DT::ARR_DBL_7 )
        this->setAttribute("unitDimension", Attribute(*aRead.resource).template get< std::array< double, 7 > >());
    else if( *aRead.dtype == DT::VEC_DOUBLE )
    {
        auto vec = Attribute(*aRead.resource).template get< std::vector< double > >();
        if( vec.size() == 7 )
        {
            std::array< double, 7 > arr;
            std::copy(vec.begin(),
                      vec.end(),
                      arr.begin());
            this->setAttribute("unitDimension", arr);
        } else
            throw std::runtime_error("Unexpected Attribute datatype for 'unitDimension'");
    }
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'unitDimension'");

    aRead.name = "timeOffset";
    this->IOHandler->enqueue(IOTask(this, aRead));
    this->IOHandler->flush();
    if( *aRead.dtype == DT::FLOAT )
        this->setAttribute("timeOffset", Attribute(*aRead.resource).template get< float >());
    else if( *aRead.dtype == DT::DOUBLE )
        this->setAttribute("timeOffset", Attribute(*aRead.resource).template get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'timeOffset'");
}

template< typename T_elem >
inline void
BaseRecord< T_elem >::flush(std::string const& name)
{
    if( !this->written && this->empty() )
        throw std::runtime_error("A Record can not be written without any contained RecordComponents: " + name);

    this->flush_impl(name);
}
} // openPMD
