#pragma once


#include <array>

#include "Attributable.hpp"
#include "Record.hpp"


class Mesh : public BaseRecord< MeshRecordComponent >
{
    friend class Container< Mesh >;
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

    template< typename T >
    std::vector< T > gridSpacing() const;
    template< typename T >
    Mesh& setGridSpacing(std::vector< T >);

    std::vector< double > gridGlobalOffset() const;
    Mesh& setGridGlobalOffset(std::vector< double >);

    double gridUnitSI() const;
    Mesh& setGridUnitSI(double);

    std::array< double, 7 > unitDimension() const override;
    Mesh& setUnitDimension(std::map< Mesh::UnitDimension, double > const&);

    template< typename T >
    T timeOffset() const;
    template< typename T >
    Mesh& setTimeOffset(T);

private:
    void flush(std::string const&) override;
    void read() override;
};  //Mesh

template< typename T >
inline std::vector< T >
Mesh::gridSpacing() const
{ return readVectorFloatingpoint< T >("gridSpacing"); }

template< typename T >
inline T
Mesh::timeOffset() const
{ return readFloatingpoint< T >("timeOffset"); }

std::ostream&
operator<<(std::ostream&, Mesh::Geometry);

std::ostream&
operator<<(std::ostream&, Mesh::DataOrder);
