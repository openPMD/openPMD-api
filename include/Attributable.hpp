#pragma once


#include <iostream>
#include <map>
#include <set>
#include <string>

#include "Attribute.hpp"
#include "Auxiliary.hpp"
#include "IO/AbstractIOHandler.hpp"
#include "Writable.hpp"


class Attributable : public Writable
{
    using A_MAP = std::map< std::string, Attribute >;

public:
    Attributable();
    Attributable(Attributable const&);
    Attributable(Attributable&&);

    virtual ~Attributable()
    { }

    Attributable& operator=(Attributable const&);
    Attributable& operator=(Attributable&&);

    template< typename T >
    void setAttribute(std::string const& key, T&& value);
    Attribute getAttribute(std::string const& key) const;
    bool deleteAttribute(std::string const& key);

    std::vector< std::string > attributes() const;
    size_t numAttributes() const;

    std::string comment() const;
    Attributable& setComment(std::string const&);

protected:
    void flushAttributes();
    void readAttributes();

    template< typename T >
    T readFloatingpoint(std::string const& key) const;
    template< typename T >
    std::vector< T > readVectorFloatingpoint(std::string const& key) const;

private:
    std::shared_ptr< A_MAP > m_attributes;
};  //Attributable

inline Attributable::Attributable()
        : m_attributes(std::make_shared< A_MAP >())
{ }

inline Attributable::Attributable(Attributable const& rhs)
// Deep-copy the entries in the Attribute map since the lifetime of the rhs does not end
        : Writable(rhs),
          m_attributes(std::make_shared< A_MAP >(*rhs.m_attributes))
{ }

inline Attributable::Attributable(Attributable&& rhs)
// Take ownership of the Attribute map since the lifetime of the rhs does end
        : Writable(rhs),
          m_attributes(rhs.m_attributes)
{ }

inline Attributable&
Attributable::operator=(Attributable const& a)
{
    if( this != &a )
    {
        Attributable tmp(a);
        std::swap(m_attributes, tmp.m_attributes);
    }
    return *this;
}

inline Attributable&
Attributable::operator=(Attributable&& a)
{
    m_attributes = std::move(a.m_attributes);
    return *this;
}

template< typename T >
inline void
Attributable::setAttribute(std::string const& key, T&& value)
{
    using std::make_shared;
    using std::make_pair;
    A_MAP::iterator it = m_attributes->lower_bound(key);
    if( it != m_attributes->end() && !m_attributes->key_comp()(key, it->first) )
    {
        // key already exists in map, just replace the value
        it->second = Attribute(value);
    } else
    {
        // emplace a new map element for an unknown key
        m_attributes->emplace_hint(it,
                                   make_pair(key, Attribute(value)));
    }
}

inline Attribute
Attributable::getAttribute(std::string const& key) const
{
    A_MAP::const_iterator it = m_attributes->find(key);
    if( it != m_attributes->cend() )
        return it->second;

    throw std::runtime_error("No such attribute: " + key);
}

inline bool
Attributable::deleteAttribute(std::string const& key)
{
    if( IOHandler && IOHandler->accessType == AccessType::READ_ONLY )
        throw std::runtime_error("Can not delete from a read-only file.");

    auto it = m_attributes->find(key);
    if( it != m_attributes->end() )
    {
        m_attributes->erase(it);
        return true;
    }
    return false;
}

inline std::vector< std::string >
Attributable::attributes() const
{
    std::vector< std::string > ret;
    ret.reserve(m_attributes->size());
    for( auto const& entry : *m_attributes )
        ret.emplace_back(entry.first);

    return ret;
}

inline size_t
Attributable::numAttributes() const
{
    return m_attributes->size();
}

inline std::string
Attributable::comment() const
{
    return boost::get< std::string >(getAttribute("comment").getResource());
}

inline Attributable&
Attributable::setComment(std::string const& c)
{
    setAttribute("comment", c);
    return *this;
}

inline void
Attributable::flushAttributes()
{
    if( dirty )
    {
        Parameter< Operation::WRITE_ATT > attribute_parameter;
        for( std::string const & att_name : attributes() )
        {
            attribute_parameter.name = att_name;
            attribute_parameter.resource = getAttribute(att_name).getResource();
            attribute_parameter.dtype = getAttribute(att_name).dtype;
            IOHandler->enqueue(IOTask(this, attribute_parameter));
        }

        dirty = false;
    }
}

inline void
Attributable::readAttributes()
{
    Parameter< Operation::LIST_ATTS > list_parameter;
    IOHandler->enqueue(IOTask(this, list_parameter));
    IOHandler->flush();
    std::vector< std::string > written_attributes = attributes();

    /* std::set_difference requires sorted ranges */
    std::sort(list_parameter.attributes->begin(), list_parameter.attributes->end());
    std::sort(written_attributes.begin(), written_attributes.end());

    std::set< std::string > attributes;
    std::set_difference(list_parameter.attributes->begin(), list_parameter.attributes->end(),
                        written_attributes.begin(), written_attributes.end(),
                        std::inserter(attributes, attributes.begin()));

    using DT = Datatype;
    Parameter< Operation::READ_ATT > attribute_parameter;

    for( auto const& att_name : attributes )
    {
        attribute_parameter.name = att_name;
        std::string att = strip(att_name, {'\0'});
        IOHandler->enqueue(IOTask(this, attribute_parameter));
        try
        {
            IOHandler->flush();
        } catch( unsupported_data_error e )
        {
            std::cerr << "Skipping non-standard attribute "
                      << att << " ("
                      << e.what()
                      << ")\n";
            continue;
        }
        Attribute a(*attribute_parameter.resource);
        switch( *attribute_parameter.dtype )
        {
            case DT::CHAR:
                setAttribute(att, a.get< char >());
                break;
            case DT::INT:
                setAttribute(att, a.get< int >());
                break;
            case DT::FLOAT:
                setAttribute(att, a.get< float >());
                break;
            case DT::DOUBLE:
                setAttribute(att, a.get< double >());
                break;
            case DT::UINT32:
                setAttribute(att, a.get< uint32_t >());
                break;
            case DT::UINT64:
                setAttribute(att, a.get< uint64_t >());
                break;
            case DT::STRING:
                setAttribute(att, a.get< std::string >());
                break;
            case DT::ARR_DBL_7:
                setAttribute(att, a.get< std::array< double, 7 > >());
                break;
            case DT::VEC_INT:
                setAttribute(att, a.get< std::vector< int > >());
                break;
            case DT::VEC_FLOAT:
                setAttribute(att, a.get< std::vector< float > >());
                break;
            case DT::VEC_DOUBLE:
                setAttribute(att, a.get< std::vector< double > >());
                break;
            case DT::VEC_UINT64:
                setAttribute(att, a.get< std::vector< uint64_t > >());
                break;
            case DT::VEC_STRING:
                setAttribute(att, a.get< std::vector< std::string > >());
                break;
            case DT::INT16:
                setAttribute(att, a.get< int16_t >());
                break;
            case DT::INT32:
                setAttribute(att, a.get< int32_t >());
                break;
            case DT::INT64:
                setAttribute(att, a.get< int64_t >());
                break;
            case DT::UINT16:
                setAttribute(att, a.get< uint16_t >());
                break;
            case DT::UCHAR:
                setAttribute(att, a.get< unsigned char >());
                break;
            case DT::BOOL:
                setAttribute(att, a.get< bool >());
                break;
            case DT::DATATYPE:
            case DT::UNDEFINED:
                throw std::runtime_error("Invalid Attribute datatype during read");
        }
    }

    IOHandler->flush();
    dirty = false;
}

template< typename T >
inline T
Attributable::readFloatingpoint(std::string const& key) const
{
    static_assert(std::is_floating_point< T >::value, "Type of attribute must be floating point");

    T t{0};
    Attribute a = getAttribute(key);
    Datatype target_dtype = determineDatatype(T());
    if( a.dtype == target_dtype )
        t = a.get< T >();
    else
    {
        using DT = Datatype;
        switch( a.dtype )
        {
            case DT::FLOAT:
                t = static_cast< T >(a.get< float >());
                break;
            case DT::DOUBLE:
                t = static_cast< T >(a.get< double >());
                break;
            default:
                throw std::runtime_error("Unknown floating point datatype.");
        }
    }
    return t;
}

template< typename T >
inline std::vector< T >
Attributable::readVectorFloatingpoint(std::string const& key) const
{
    static_assert(std::is_floating_point< T >::value, "Type of attribute must be floating point");

    std::vector< T > vt{};
    Attribute a = getAttribute(key);
    Datatype target_dtype = determineDatatype(T());
    if( a.dtype == target_dtype )
        vt = a.get< std::vector< T > >();
    else
    {
        using DT = Datatype;
        switch( a.dtype )
        {
            case DT::VEC_FLOAT:
                for( auto const& val : a.get< std::vector< float > >() )
                    vt.push_back(static_cast< T >(val));
                break;
            case DT::VEC_DOUBLE:
                for( auto const& val : a.get< std::vector< double > >() )
                    vt.push_back(static_cast< T >(val));
                break;
            default:
                throw std::runtime_error("Unknown floating point datatype.");
        }
    }
    return vt;
}
