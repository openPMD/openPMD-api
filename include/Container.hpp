#pragma once


#include <string>
#include <unordered_map>


template<
        typename T,
        typename T_key = std::string
>
class Container : public Attributable
{
protected:
    using MapType = std::unordered_map< T_key, T >;
    MapType m_data;

public:
    std::size_t size() const;
    T& operator[](T_key const& key);
    bool insert(typename MapType::value_type const& value);
    bool remove(T_key const& key);

    typename MapType::iterator begin()
    { return m_data.begin(); }

    typename MapType::const_iterator begin() const
    { return m_data.cbegin(); }

    typename MapType::iterator end()
    { return m_data.end(); }

    typename MapType::const_iterator end() const
    { return m_data.cend(); }

    std::vector< T_key > keys() const;

};  //Container

template<
        typename T,
        typename T_key
>
inline T&
Container< T, T_key >::operator[](T_key const& key)
{
    typename MapType::iterator it = m_data.find(key);
    if( it != m_data.end() )
    {
        return it->second;
    } else
    {
        // TODO in this verison, the "operator[]" requires "T()" to be available
        // trivial solution - "friend" Container in both, make a private default constructor
        // another solution - make the MapType "map<T_key, boost:optional<T>>"
        m_data[key] = T();
        return m_data[key];
    }
}

template<
        typename T,
        typename T_key
>
inline bool
Container< T, T_key >::insert(typename MapType::value_type const& value)
{
    return m_data.insert(value).second;
}

template<
        typename T,
        typename T_key
>
inline bool
Container< T, T_key >::remove(T_key const& key)
{
    typename MapType::iterator it = m_data.find(key);
    if( it != m_data.end() )
    {
        m_data.erase(it);
        return true;
    }
    return false;
}

template<
        typename T,
        typename T_key
>
inline std::size_t
Container< T, T_key >::size() const
{ return m_data.size(); }


template<
        typename T,
        typename T_key
>
typename Container< T, T_key >::MapType::iterator
begin(Container< T, T_key >& c)
{
    return c.begin();
};

template<
        typename T,
        typename T_key
>
typename Container< T, T_key >::MapType::iterator
end(Container< T, T_key >& c)
{
    return c.end();
};

template<
        typename T,
        typename T_key
>
inline std::vector< T_key >
Container<T, T_key>::keys() const
{
    std::vector< T_key > ret;
    ret.reserve(m_data.size());
    auto e = m_data.cend();
    for( auto it = m_data.cbegin(); it != e; ++it )
    {
        ret.emplace_back(it->first);
    }
    return ret;
}
