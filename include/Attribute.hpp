#pragma once


#include <array>
#include <string>
#include <vector>

#include <boost/variant.hpp>

#include "Datatype.hpp"


//TODO This might have to be a Writable
//Reasoning - Flushes are expeted to be done often.
//Attributes should not be written unless dirty.
//At the moment the dirty check is done at Attributable level
class Attribute
{
public:
    /* To guarantee that the resource type is detected correctly,
     * the order of types in this variant MUST match
     * the order of types in Dtype. */
    using resource =
    boost::variant< char, int, float, double,
                    uint32_t, uint64_t, std::string,
                    std::array< double, 7 >,
                    std::vector< int >,
                    std::vector< float >,
                    std::vector< double >,
                    std::vector< uint64_t >,
                    std::vector< std::string >,
                    Datatype
    >;

    Attribute(resource r)
            : dtype{static_cast<Datatype>(r.which())},
              m_data{r}
    { }

    template< typename T >
    T get() const
    {
        return boost::get< T >(m_data);
    }

    resource getResource() const
    {
        return m_data;
    }

    Datatype dtype;

private:
    resource m_data;
};  //Attribute
