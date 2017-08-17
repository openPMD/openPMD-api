#pragma once


#include <string>
#include <unordered_map>
#include "Attributable.hpp"
#include "Writable.hpp"


template<
        typename T,
        typename T_key = std::string
>
class Container
        : public std::unordered_map< T_key, T >,
          public Attributable,
          public Writable
{
    friend class Iteration;
    friend class Output;

public:
    using value_type = T;
    using key_type = T_key;

    T& operator[](T_key key)
    {
        auto it = this->find(key);
        if( it != this->end() )
            return it->second;
        else
        {
            T t = T();
            t.IOHandler = IOHandler;
            t.parent = this;
            return this->insert({key, std::move(t)}).first->second;
        }
    }

    typename std::unordered_map< T_key, T >::size_type erase(T_key const& key)
    {
        auto res = std::unordered_map< T_key, T >::find(key);
        if( res != std::unordered_map< T_key, T >::end() && res->second.written )
        {
            // TODO delete element on disk
        }
        return std::unordered_map< T_key, T >::erase(key);
    }

private:
    void flush(std::string const path)
    {
        if( !written )
        {
            Parameter< Operation::CREATE_PATH > path_parameter;
            path_parameter.path = path;
            IOHandler->enqueue(IOTask(this, path_parameter));
            IOHandler->flush();
        }

        Parameter< Operation::WRITE_ATT > attribute_parameter;
        for( std::string const & att_name : attributes() )
        {
            attribute_parameter.name = att_name;
            attribute_parameter.resource = getAttribute(att_name).getResource();
            attribute_parameter.dtype = getAttribute(att_name).dtype;
            IOHandler->enqueue(IOTask(this, attribute_parameter));
        }
    }
};
