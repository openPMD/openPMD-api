#pragma once


#include <memory>


//class Record_base;

class Record : public Attributable
{
public:
    enum class Dimension
    {
        one = 1, two, three
    };

    enum class Component
    {
        one = 1, two, three
    };

    enum class UnitDimension : size_t
    {
        L = 0, M, T, I, theta, N, J
    };

    Record(Dimension dim, std::initializer_list< std::string > comps);

    std::string const name() const;
    Record setName(std::string const&);

    std::array< double, 7 > unitDimension() const;
    Record setUnitDimension(std::map< Record::UnitDimension, double > udim);

    float timeOffset() const;
    Record setTimeOffset(float const);

    std::map< Record::Component, double > unitSI() const;
    Record setUnitSI(std::map< Record::Component, double >);

    Record& operator[](const std::string&);

    template< typename T >
    Record linkData(T* /*data*/);

private:
    std::map< Record::Component, Record > m_components;
    //std::shared_ptr< Record_base > prb;
};  //Record


Record
makeTensorRecord(RecDim /*rd*/,
                 RecComp /*rc*/,
                 std::initializer_list< std::string > /*comps*/);

Record
makeVectorRecord(std::initializer_list< std::string > /*comps*/);

Record
        makeScalarRecord(RecDim /*rd*/);

Record
makeConstantRecord(const double value, std::initializer_list< uint64_t > shape);
