#include <iostream>
#include "../include/Mesh.hpp"


Mesh::Mesh(Record const& r)
        : Record(r)
{
    setGeometry(Geometry::cartesian);
    setDataOrder(DataOrder::C);

    std::size_t size = r.m_components.size();
    std::vector< float > ones(size, 1);
    std::vector< double > zeros(size, 0);

    setAxisLabels({"z", "y", "x"});
    setGridSpacing(ones);
    setGridGlobalOffset(zeros);
    setGridUnitSI(1);

    setPosition({{"z", zeros},
                 {"y", zeros},
                 {"x", zeros}});
}

Mesh&
Mesh::operator=(Record const& r)
{
    Mesh tmp(r);
    std::swap(*this, tmp);
    return *this;
}

Mesh::Geometry
Mesh::geometry() const
{
    std::string ret = boost::get< std::string >(getAttribute("geometry"));
    if( ret == "cartesian" ) { return Geometry::cartesian; }
    if( ret == "thetaMode" ) { return Geometry::thetaMode; }
    if( ret == "cylindrical" ) { return Geometry::cylindrical; }
    if( ret == "spherical" ) { return Geometry::spherical; }
}

Mesh&
Mesh::setGeometry(Mesh::Geometry g)
{
    switch( g )
    {
        case Geometry::cartesian:
            setAttribute("geometry", "cartesian");
            break;
        case Geometry::thetaMode:
            setAttribute("geometry", "thetaMode");
            break;
        case Geometry::cylindrical:
            setAttribute("geometry", "cylindrical");
            break;
        case Geometry::spherical:
            setAttribute("geometry", "spherical");
            break;
    }
    return *this;
}

std::string
Mesh::geometryParameters() const
{
    return boost::get< std::string >(getAttribute("geometryParameters"));
}

Mesh&
Mesh::setGeometryParameters(std::string const& gp)
{
    setAttribute("geometryParameters", gp);
    return *this;
}

Mesh::DataOrder
Mesh::dataOrder() const
{
    return Mesh::DataOrder(boost::get< char >(getAttribute("dataOrder")));
}

Mesh&
Mesh::setDataOrder(Mesh::DataOrder dor)
{
    setAttribute("dataOrder", static_cast<char>(dor));
    return *this;
}

std::vector< std::string >
Mesh::axisLabels() const
{
    return boost::get< std::vector< std::string > >(getAttribute("axisLabels"));
}

Mesh&
Mesh::setAxisLabels(std::vector< std::string > als)
{
    setAttribute("axisLabels", als);
    return *this;
}

std::vector< float >
Mesh::gridSpacing() const
{
    return boost::get< std::vector< float > >(getAttribute("gridSpacing"));
}

Mesh&
Mesh::setGridSpacing(std::vector< float > gs)
{
    assert(gs.size() == m_components.size());
    setAttribute("gridSpacing", gs);
    return *this;
}

std::vector< double >
Mesh::gridGlobalOffset() const
{
    return boost::get< std::vector< double>>(getAttribute("gridGlobalOffset"));
}

Mesh&
Mesh::setGridGlobalOffset(std::vector< double > ggo)
{
    assert(ggo.size() == m_components.size());
    setAttribute("gridGlobalOffset", ggo);
    return *this;
}

double
Mesh::gridUnitSI() const
{
    return boost::get< double >(getAttribute("gridUnitSI"));
}

Mesh&
Mesh::setGridUnitSI(double gusi)
{
    setAttribute("gridUnitSI", gusi);
    return *this;
}

std::map< std::string, std::vector< double > >
Mesh::position() const
{
    std::map< std::string, std::vector< double > > ret;
    for( auto const& component : m_components )
    {
        ret.insert({component.first,
                    component.second.position()});
    }
    return ret;
}

Mesh&
Mesh::setPosition(std::map< std::string, std::vector< double > > const& pos)
{
    for( auto const& entry : pos )
    {
        assert(entry.second.size() == static_cast<size_t>(m_dimension));
    }
    for( auto const& entry : pos )
    {
        auto it = m_components.find(entry.first);
        if( it != m_components.end() )
        {
            it->second.setPosition(entry.second);
        } else
        {
            std::cerr<<"Unknown Record component: "<<entry.first<<'\n';
        }
    }
    return *this;
}

std::ostream&
operator<<(std::ostream& os, Mesh::Geometry go)
{
    switch( go )
    {
        case Mesh::Geometry::cartesian:
            os<<"cartesian";
            break;
        case Mesh::Geometry::thetaMode:
            os<<"thetaMode";
            break;
        case Mesh::Geometry::cylindrical:
            os<<"cylindrical";
            break;
        case Mesh::Geometry::spherical:
            os<<"spherical";
            break;
    }
    return os;
}

std::ostream&
operator<<(std::ostream& os, Mesh::DataOrder dor)
{
    switch( dor )
    {
        case Mesh::DataOrder::C:
            os<<'C';
            break;
        case Mesh::DataOrder::F:
            os<<'F';
            break;
    }
    return os;
}
