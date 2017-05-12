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
            this->insert({key, T()});
            auto t = this->find(key);
            t->second.IOHandler = IOHandler;
            return t->second;
        }
    }
};
