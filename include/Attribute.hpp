#pragma once


#include <array>
#include <string>
#include <vector>

#include <boost/variant.hpp>

#include "Datatype.hpp"


template< class T_DTYPES, typename ... T >
class Variadic
{
public:
    using resource = boost::variant< T ... >;
    Variadic(resource r)
            : dtype{static_cast<T_DTYPES>(r.which())},
              m_data{r}
    { static_assert(std::is_enum< T_DTYPES >::value, "Datatypes to variadic have to be supplied as enum."); }

    template< typename U >
    U get() const
    {
        return boost::get< U >(m_data);
    }

    resource getResource() const
    {
        return m_data;
    }

    T_DTYPES dtype;

private:
    resource m_data;
};

//TODO This might have to be a Writable
//Reasoning - Flushes are expeted to be done often.
//Attributes should not be written unless dirty.
//At the moment the dirty check is done at Attributable level
using Attribute = Variadic< Datatype,
                            char, int, float, double,
                            uint32_t, uint64_t, std::string,
                            std::array< double, 7 >,
                            std::vector< int >,
                            std::vector< float >,
                            std::vector< double >,
                            std::vector< uint64_t >,
                            std::vector< std::string >,
                            int16_t, int32_t, int64_t,
                            uint16_t,
                            unsigned char,
                            bool >;
