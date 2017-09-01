#include <iostream>

#include "../include/Auxiliary.hpp"
#include "../include/Mesh.hpp"


Mesh::Mesh()
{
    setGeometry(Geometry::cartesian);
    setDataOrder(DataOrder::C);

    setAxisLabels({""});
    setGridSpacing({1});
    setGridGlobalOffset({0});
    setGridUnitSI(1);
}

MeshRecordComponent&
Mesh::operator[](std::string key)
{
    auto it = this->find(key);
    if( it != this->end() )
        return it->second;
    else
    {
        bool scalar = (key == MeshRecordComponent::SCALAR);
        if( (scalar && !empty() && !m_containsScalar) || (m_containsScalar && !scalar) )
            throw std::runtime_error("A scalar component can not be contained at "
                                     "the same time as one or more regular components.");

        MeshRecordComponent & ret = Container< MeshRecordComponent >::operator[](key);
        if( scalar )
        {
            m_containsScalar = true;
            ret.parent = this->parent;
        }
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
            setAttribute("geometry", std::string("cartesian"));
            break;
        case Geometry::thetaMode:
            setAttribute("geometry", std::string("thetaMode"));
            break;
        case Geometry::cylindrical:
            setAttribute("geometry", std::string("cylindrical"));
            break;
        case Geometry::spherical:
            setAttribute("geometry", std::string("spherical"));
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

std::array< double, 7 >
Mesh::unitDimension() const
{
    return getAttribute("unitDimension").get< std::array< double, 7 > >();
}

Mesh&
Mesh::setUnitDimension(std::map< Mesh::UnitDimension, double > const& udim)
{
    if( !udim.empty() )
    {
        std::array< double, 7 > unitDimension = this->unitDimension();
        for( auto const& entry : udim )
            unitDimension[static_cast<uint8_t>(entry.first)] = entry.second;
        setAttribute("unitDimension", unitDimension);
        dirty = true;
    }
    return *this;
}

float
Mesh::timeOffset() const
{
    return getAttribute("timeOffset").get< float >();
}

Mesh&
Mesh::setTimeOffset(float const to)
{
    setAttribute("timeOffset", to);
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
           MeshRecordComponent& r = at(RecordComponent::SCALAR);
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

    flushAttributes();
}

void
Mesh::read()
{
    /* allow all attributes to be set */
    written = false;

    using DT = Datatype;
    Parameter< Operation::READ_ATT > attribute_parameter;

    attribute_parameter.name = "geometry";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::STRING )
    {
        std::string geometry = strip(Attribute(*attribute_parameter.resource).get< std::string >(), {'\0'});
        if( geometry == "cartesian" )
            setGeometry(Geometry::cartesian);
        else if( geometry == "thetaMode" )
            setGeometry(Geometry::thetaMode);
        else if( geometry == "cylindrical" )
            setGeometry(Geometry::cylindrical);
        else if( geometry == "spherical" )
            setGeometry(Geometry::spherical);
        else
            throw std::runtime_error("Unkonwn geometry " + geometry);
    }
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'geometry'");

    attribute_parameter.name = "dataOrder";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::CHAR )
        setDataOrder(static_cast<DataOrder>(Attribute(*attribute_parameter.resource).get< char >()));
    else if( *attribute_parameter.dtype == DT::STRING )
    {
        std::string dataOrder = strip(Attribute(*attribute_parameter.resource).get< std::string >(), {'\0'});
        if( dataOrder.size() == 1 )
            setDataOrder(static_cast<DataOrder>(dataOrder[0]));
        else
            throw std::runtime_error("Unexpected Attribute value for 'dataOrder': " + dataOrder);
    }
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'dataOrder'");

    attribute_parameter.name = "axisLabels";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::VEC_STRING )
        setAxisLabels(Attribute(*attribute_parameter.resource).get< std::vector< std::string > >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'axisLabels'");

    attribute_parameter.name = "gridSpacing";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::VEC_FLOAT )
        setGridSpacing(Attribute(*attribute_parameter.resource).get< std::vector< float > >());
    else if( *attribute_parameter.dtype == DT::VEC_DOUBLE )
    {
        std::cerr << "Non-standard attribute datatype for 'gridSpacing' (should be float, is double)\n";
        std::vector< float > gridSpacing;
        for( auto const& val : Attribute(*attribute_parameter.resource).get< std::vector< double > >() )
            gridSpacing.push_back(static_cast<float>(val));
        setGridSpacing(gridSpacing);
    }
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'gridSpacing'");

    attribute_parameter.name = "gridGlobalOffset";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::VEC_DOUBLE )
        setGridGlobalOffset(Attribute(*attribute_parameter.resource).get< std::vector< double > >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'gridGlobalOffset'");

    attribute_parameter.name = "gridUnitSI";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::DOUBLE )
        setGridUnitSI(Attribute(*attribute_parameter.resource).get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'gridUnitSI'");

    if( m_containsScalar )
    {
        /* using operator[] will falsely update parent */
        (*this).find(MeshRecordComponent::SCALAR)->second.read();
    } else
    {
        Parameter< Operation::LIST_PATHS > plist_parameter;
        IOHandler->enqueue(IOTask(this, plist_parameter));
        IOHandler->flush();

        Parameter< Operation::OPEN_PATH > path_parameter;
        for( auto const& component : *plist_parameter.paths )
        {
            MeshRecordComponent& rc = (*this)[component];
            path_parameter.path = component;
            IOHandler->enqueue(IOTask(&rc, path_parameter));
            IOHandler->flush();
            rc.m_isConstant = true;
            rc.read();
        }

        Parameter< Operation::LIST_DATASETS > dlist_parameter;
        IOHandler->enqueue(IOTask(this, dlist_parameter));
        IOHandler->flush();

        Parameter< Operation::OPEN_DATASET > dataset_parameter;
        for( auto const& component : *dlist_parameter.datasets )
        {
            MeshRecordComponent& rc = (*this)[component];
            dataset_parameter.name = component;
            IOHandler->enqueue(IOTask(&rc, dataset_parameter));
            IOHandler->flush();
            rc.resetDataset(Dataset(*dataset_parameter.dtype, *dataset_parameter.extent));
            rc.read();
        }
    }

    readBase();

    readAttributes();

    /* this file need not be flushed */
    written = true;
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
