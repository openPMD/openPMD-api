#pragma once


#include <memory>

#include "Attributable.hpp"

//class Record_base;

class Record : public Attributable
{
    friend class Mesh;
public:
    enum class Dimension : size_t
    {
        one = 1, two, three
    };

    enum class UnitDimension : size_t
    {
        L = 0, M, T, I, theta, N, J
    };

    Record(std::string name, Dimension dim, std::initializer_list< std::string > comps, bool isRecordComponent = false);
    Record(Record const &);

    std::string const name() const;
    Record setName(std::string const&);

    std::array< double, 7 > unitDimension() const;
    Record setUnitDimension(std::map< Record::UnitDimension, double > const &);

    float timeOffset() const;
    Record setTimeOffset(float const);

    double unitSI() const;
    Record setUnitSI(double);
    Record setUnitSI(std::map< std::string, double >);

    Record& operator[](std::string const&);

    template< typename T >
    Record linkData(T* /*data*/);

    void unlinkData();

protected:
    std::map< std::string, Record > m_components;
    bool m_isComponent;
    Dimension m_dimension;
};  //Record


//Record
//makeTensorRecord(RecDim /*rd*/,
//                 RecComp /*rc*/,
//                 std::initializer_list< std::string > /*comps*/);
//
//Record
//makeVectorRecord(std::initializer_list< std::string > /*comps*/);
//
//Record
//        makeScalarRecord(RecDim /*rd*/);
//
//Record
//makeConstantRecord(const double value, std::initializer_list< uint64_t > shape);
