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
          private Writable
{ };
