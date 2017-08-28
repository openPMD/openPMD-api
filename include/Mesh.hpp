#pragma once


#include <array>

#include "Attributable.hpp"
#include "Record.hpp"


class Mesh : public BaseRecord< MeshRecordComponent >
{
    template<
            typename T,
            typename T_key
    >
    friend class Container;
    friend class Iteration;

private:
    Mesh();

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

    MeshRecordComponent& operator[](std::string);

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

    std::array< double, 7 > unitDimension() const override;
    Mesh& setUnitDimension(std::map< Mesh::UnitDimension, double > const&);

    float timeOffset() const override;
    Mesh& setTimeOffset(float const);

private:
    void flush(std::string const&) override;
    void read() override;
};  //Mesh

std::ostream&
operator<<(std::ostream&, Mesh::Geometry);

std::ostream&
operator<<(std::ostream&, Mesh::DataOrder);
