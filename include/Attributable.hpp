#pragma once


#include <string>
#include <map>

#include "../include/Attribute.hpp"


class Attributable
{
    using A_MAP = std::map< std::string, std::shared_ptr< Attribute > >;

public:
    Attributable() : m_attributes(std::make_shared< A_MAP >())
    { }

    Attributable(Attributable const& rhs)
            : m_attributes(std::make_shared< A_MAP >(*rhs.m_attributes))
    { } // Deep-copy the entries in the Attribute map
    Attributable(Attributable&& rhs)
            : m_attributes(rhs.m_attributes)
    { } // Take ownership of the Attribute map
    Attributable& operator=(Attributable const& a)
    {
        if( this != &a )
        {
            Attributable tmp(a);
            std::swap(m_attributes, tmp.m_attributes);
        }
        return *this;
    }

    Attributable& operator=(Attributable&& a)
    {
        m_attributes = std::move(a.m_attributes);
        return *this;
    }

    virtual ~Attributable()
    { }

    /* This could be done via universal references to gain speed */
    template< typename T >
    void setAttribute(std::string const& key, T&& value);
    bool deleteAttribute(std::string const& key);

    std::vector< std::string > attributes() const;
    size_t numAttributes() const;

    Attribute getAttribute(std::string const& key) const;

    std::string comment() const;
    Attributable& setComment(std::string const &);

private:
    std::shared_ptr< A_MAP > m_attributes;
};  //Attributable


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
        it->second = make_shared< Attribute >(value);
    } else
    {
        // emplace a new map element for an unknown key
        m_attributes->emplace_hint(it,
                                   make_pair(key,
                                             make_shared< Attribute >(value)));
    }
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

inline Attribute
Attributable::getAttribute(std::string const& key) const
{
    A_MAP::const_iterator it = m_attributes->find(key);
    if( it != m_attributes->cend() )
    {
        return *(it->second);
    }
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
