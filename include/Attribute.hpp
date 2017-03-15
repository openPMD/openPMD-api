#pragma once


#include <array>
#include <string>
#include <vector>

#include <boost/variant.hpp>


class Attribute
{
public:
    enum class Dtype : int
    {
        CHAR = 0, INT, FLOAT, DOUBLE,
        UINT32, UINT64, STRING,
        ARR_DBL_7,
        VEC_INT,
        VEC_FLOAT,
        VEC_DOUBLE,
        VEC_UINT64,
        VEC_STRING,
        UNDEFINED
    };  //Dtype

    using resource =
    boost::variant< char, int, float, double,
                    uint32_t, uint64_t, std::string,
                    std::array< double, 7 >,
                    std::vector< int >,
                    std::vector< float >,
                    std::vector< double >,
                    std::vector< uint64_t >,
                    std::vector< std::string >
    >;

    Attribute(resource r)
            : dtype{static_cast<Attribute::Dtype>(r.which())},
              m_data{r}
    { }

    resource getResource() const
    {
        return m_data;
    }

    Dtype dtype;

private:
    resource m_data;
};  //Attribute
