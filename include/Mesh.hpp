#pragma once


#include <array>

#include "Attributable.hpp"
#include "Record.hpp"


class Mesh : public Record
{
    template<
            typename T,
            typename T_key
    >
    friend class Container;

private:
    //TODO
    Mesh(): Mesh(Record()) { }
    //TODO
    Mesh(Record const&);

    Mesh& operator=(Record const&);

public:
    Mesh(Mesh const&) = default;

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

    Geometry geometry() const;
    Mesh& setGeometry(Geometry);

    std::string geometryParameters() const;
    Mesh& setGeometryParameters(std::string const&);

    DataOrder dataOrder() const;
    Mesh& setDataOrder(DataOrder);

    std::vector< std::string > axisLabels() const;
    Mesh& setAxisLabels(std::vector< std::string >);

    std::vector< float > gridSpacing() const;
    Mesh& setGridSpacing(std::vector< float >);

    std::vector< double > gridGlobalOffset() const;
    Mesh& setGridGlobalOffset(std::vector< double >);

    double gridUnitSI() const;
    Mesh& setGridUnitSI(double);

    std::map< std::string, std::vector< double > > position() const;
    Mesh& setPosition(std::map< std::string, std::vector< double > > const&);

    Record record() const;
};  //Mesh

std::ostream&
operator<<(std::ostream&, Mesh::Geometry);

std::ostream&
operator<<(std::ostream&, Mesh::DataOrder);
