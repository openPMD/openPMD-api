#pragma once


#include <string>
#include <map>

#include "../include/Attribute.h"


class Attributable
{
    using A_MAP = std::map< std::string, std::shared_ptr< Attribute > >;

public:
    Attributable() : m_attributes(std::make_unique< A_MAP >())
    { }

    Attributable(Attributable const& rhs)
            : m_attributes(std::make_unique< A_MAP >(*rhs.m_attributes))
    { } // Deep-copy the entries in the Attribute map
    Attributable(Attributable&& rhs)
            : m_attributes(rhs.m_attributes.release())
    { } // Take ownership of the unique Attribute map
    virtual ~Attributable()
    { }

    /* This could be done via universal references to gain speed */
    template< typename T >
    void setAttribute(std::string const& key, T value);
    bool deleteAttribute(std::string const& key);

//    std::ostream &writeAttributes(std::ostream &);

protected:
    Attribute::resource getAttribute(std::string const& key) const;

private:
    std::unique_ptr< A_MAP > m_attributes;
};  //Attributable


template< typename T >
inline void
Attributable::setAttribute(std::string const& key, T value)
{
    using std::make_shared;
    A_MAP::iterator it = m_attributes->lower_bound(key);
    if( it != m_attributes->end() && !m_attributes->key_comp()(key, it->first) )
    {
        // key already exists in map, just replace the value
        it->second = make_shared< Attribute >(value);
    } else
    {
        // emplace a new map element for an unknown key
        m_attributes->emplace_hint(it, {key, make_shared< Attribute >(value)});
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

inline Attribute::resource
Attributable::getAttribute(std::string const& key) const
{
    A_MAP::const_iterator it = m_attributes->find(key);
    if( it != m_attributes->cend() )
    {
        return it->second->get();
    }
    return Attribute::resource();
}