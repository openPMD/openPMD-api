#pragma once


#include <std::array>
#include <std::string>
#include <std::vector>

#include <boost/variant.hpp>


class Attribute
{
public:
    using resource =
    boost::variant< char, int, float, double,
                    uint32_t, uint64_t, std::string,
                    std::array< double, 7 >,
                    std::vector< char >,
                    std::vector< int >,
                    std::vector< float >,
                    std::vector< double >,
                    std::vector< uint64_t >,
                    std::vector< std::string >
    >;

    Attribute(resource r) : m_data{r}
    { }

    resource get() const
    {
        return m_data;
    }

private:
    resource m_data;
};  //Attribute