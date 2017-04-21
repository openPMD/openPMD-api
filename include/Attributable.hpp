#pragma once


#include <map>
#include <string>

#include "../include/Attribute.hpp"


class Attributable
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

private:
    std::shared_ptr< A_MAP > m_attributes;
};  //Attributable

inline Attributable::Attributable()
        : m_attributes(std::make_shared< A_MAP >())
{ }

inline Attributable::Attributable(Attributable const& rhs)
// Deep-copy the entries in the Attribute map since the lifetime of the rhs does not end
        : m_attributes(std::make_shared< A_MAP >(*rhs.m_attributes))
{ }

inline Attributable::Attributable(Attributable&& rhs)
// Take ownership of the Attribute map since the lifetime of the rhs does end
        : m_attributes(rhs.m_attributes)
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
    {
        return it->second;
    }
    throw std::runtime_error("No such attribute: " + key);
}

inline bool
Attributable::deleteAttribute(std::string const& key)
{
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
    {
        ret.emplace_back(entry.first);
    }
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
