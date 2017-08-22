#include <iostream>
#include "../include/Mesh.hpp"


Mesh::Mesh(Record const& r)
        : Record(r)
{
    setGeometry(Geometry::cartesian);
    setDataOrder(DataOrder::C);

    setAxisLabels({""});
    setGridSpacing({1});
    setGridGlobalOffset({0});
    setGridUnitSI(1);
}

Mesh&
Mesh::operator=(Record const& r)
{
    Mesh tmp(r);
    std::swap(*this, tmp);
    return *this;
}

RecordComponent&
Mesh::operator[](std::string key)
{
    auto it = this->find(key);
    if( it != this->end() )
        return it->second;
    else
    {
        RecordComponent & ret = Record::operator[](key);
        ret.setPosition({0});
        ret.parent = this;
        return ret;
    }
}

Mesh::Geometry
Mesh::geometry() const
{
    std::string ret = getAttribute("geometry").get< std::string >();
    if( ret == "cartesian" ) { return Geometry::cartesian; }
    else if( ret == "thetaMode" ) { return Geometry::thetaMode; }
    else if( ret == "cylindrical" ) { return Geometry::cylindrical; }
    else if( ret == "spherical" ) { return Geometry::spherical; }
    throw std::runtime_error("Unkonwn geometry " + ret);
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
    dirty = true;
    return *this;
}

std::string
Mesh::geometryParameters() const
{
    return getAttribute("geometryParameters").get< std::string >();
}

Mesh&
Mesh::setGeometryParameters(std::string const& gp)
{
    setAttribute("geometryParameters", gp);
    dirty = true;
    return *this;
}

Mesh::DataOrder
Mesh::dataOrder() const
{
    return Mesh::DataOrder(boost::get< char >(getAttribute("dataOrder").getResource()));
}

Mesh&
Mesh::setDataOrder(Mesh::DataOrder dor)
{
    setAttribute("dataOrder", static_cast<char>(dor));
    dirty = true;
    return *this;
}

std::vector< std::string >
Mesh::axisLabels() const
{
    return getAttribute("axisLabels").get< std::vector< std::string > >();
}

Mesh&
Mesh::setAxisLabels(std::vector< std::string > als)
{
    setAttribute("axisLabels", als);
    dirty = true;
    return *this;
}

std::vector< float >
Mesh::gridSpacing() const
{
    return getAttribute("gridSpacing").get< std::vector< float > >();
}

Mesh&
Mesh::setGridSpacing(std::vector< float > gs)
{
    setAttribute("gridSpacing", gs);
    dirty = true;
    return *this;
}

std::vector< double >
Mesh::gridGlobalOffset() const
{
    return boost::get< std::vector< double>>(getAttribute("gridGlobalOffset").getResource());
}

Mesh&
Mesh::setGridGlobalOffset(std::vector< double > ggo)
{
    setAttribute("gridGlobalOffset", ggo);
    dirty = true;
    return *this;
}

double
Mesh::gridUnitSI() const
{
    return getAttribute("gridUnitSI").get< double >();
}

Mesh&
Mesh::setGridUnitSI(double gusi)
{
    setAttribute("gridUnitSI", gusi);
    dirty = true;
    return *this;
}

std::map< std::string, std::vector< double > >
Mesh::position() const
{
    std::map< std::string, std::vector< double > > ret;
    for( auto const& component : *this )
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
        auto it = find(entry.first);
        if( it != end() )
        {
            it->second.setPosition(entry.second);
        } else
        {
            std::cerr<<"Unknown Record component: "<<entry.first<<'\n';
        }
    }
    dirty = true;
    return *this;
}

void
Mesh::flush(std::string const& name)
{
    if( !written )
    {
        if( m_containsScalar )
        {
            RecordComponent& r = at(RecordComponent::SCALAR);
            r.parent = parent;
            r.flush(name);
            abstractFilePosition = r.abstractFilePosition;
            written = true;
        } else
        {
            Parameter< Operation::CREATE_PATH > path_parameter;
            path_parameter.path = name;
            IOHandler->enqueue(IOTask(this, path_parameter));
            IOHandler->flush();
            for( auto& comp : *this )
                comp.second.parent = this;
        }
    }

    for( auto& comp : *this )
        comp.second.flush(comp.first);

    if( dirty )
    {
        Parameter< Operation::WRITE_ATT > attribute_parameter;
        for( std::string const & att_name : attributes() )
        {
            attribute_parameter.name = att_name;
            attribute_parameter.resource = getAttribute(att_name).getResource();
            attribute_parameter.dtype = getAttribute(att_name).dtype;
            IOHandler->enqueue(IOTask(this, attribute_parameter));
        }
    }

    dirty = false;
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
