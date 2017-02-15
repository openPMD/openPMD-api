#pragma once


#include <array>

#include "Attributable.hpp"
#include "Record.hpp"


class Mesh : public Record
{
public:
    enum class Geometry
    {
        cartesian,
        thetaMode,
        cylindrical,
        spherical
    };  //Geometry

    enum class DataOrder : char
    {
        C = 'C',
        F = 'F'
    };  //DataOrder

    //TODO
    Mesh(): Record(Record::Dimension::three, {}, true) { }
    //TODO
    Mesh(Mesh const&) = default;
    Mesh(Record const&);

    Mesh& operator=(Record const&);

    Geometry geometry() const;
    Mesh setGeometry(Geometry);

    std::string geometryParameters() const;
    Mesh setGeometryParameters(std::string const&);

    DataOrder dataOrder() const;
    Mesh setDataOrder(DataOrder);

    std::vector< std::string > axisLabels() const;
    Mesh setAxisLabels(std::vector< std::string >);

    std::vector< float > gridSpacing() const;
    Mesh setGridSpacing(std::vector< float >);

    std::vector< double > gridGlobalOffset() const;
    Mesh setGridGlobalOffset(std::vector< double >);

    double gridUnitSI() const;
    Mesh setGridUnitSI(double);

    std::map< std::string, std::vector< double > > position() const;
    Mesh setPosition(std::map< std::string, std::vector< double > > const&);

    Record record() const;
};  //Mesh

std::ostream&
operator<<(std::ostream&, Mesh::Geometry);

std::ostream&
operator<<(std::ostream&, Mesh::DataOrder);
