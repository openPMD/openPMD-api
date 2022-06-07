/* Copyright 2017-2021 Fabian Koller
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#include "openPMD/Mesh.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/DerefDynamicCast.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Writable.hpp"

#include <algorithm>
#include <iostream>

namespace openPMD
{
Mesh::Mesh()
{
    setTimeOffset(0.f);

    setGeometry(Geometry::cartesian);
    setDataOrder(DataOrder::C);

    setAxisLabels({"x"}); // empty strings are not allowed in HDF5
    setGridSpacing(std::vector<double>{1});
    setGridGlobalOffset({0});
    setGridUnitSI(1);
}

Mesh::Geometry Mesh::geometry() const
{
    std::string ret = geometryString();
    if ("cartesian" == ret)
    {
        return Geometry::cartesian;
    }
    else if ("thetaMode" == ret)
    {
        return Geometry::thetaMode;
    }
    else if ("cylindrical" == ret)
    {
        return Geometry::cylindrical;
    }
    else if ("spherical" == ret)
    {
        return Geometry::spherical;
    }
    else
    {
        return Geometry::other;
    }
}

std::string Mesh::geometryString() const
{
    return getAttribute("geometry").get<std::string>();
}

Mesh &Mesh::setGeometry(Mesh::Geometry g)
{
    switch (g)
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
    case Geometry::other:
        // use the std::string overload to be more specific
        setAttribute("geometry", std::string("other"));
        break;
    }
    return *this;
}

Mesh &Mesh::setGeometry(std::string geometry)
{
    std::string knownGeometries[] = {
        "cartesian", "thetaMode", "cylindrical", "spherical", "other"};
    if ( // 1. condition: geometry is not one of the known geometries
        std::find(
            std::begin(knownGeometries), std::end(knownGeometries), geometry) ==
            std::end(knownGeometries)
        // 2. condition: prefix is not already there
        && !auxiliary::starts_with(geometry, std::string("other:")))
    {
        geometry = "other:" + geometry;
    }
    setAttribute("geometry", std::move(geometry));
    return *this;
}

std::string Mesh::geometryParameters() const
{
    return getAttribute("geometryParameters").get<std::string>();
}

Mesh &Mesh::setGeometryParameters(std::string const &gp)
{
    setAttribute("geometryParameters", gp);
    return *this;
}

Mesh::DataOrder Mesh::dataOrder() const
{
    return Mesh::DataOrder(
        getAttribute("dataOrder").get<std::string>().c_str()[0]);
}

Mesh &Mesh::setDataOrder(Mesh::DataOrder dor)
{
    setAttribute("dataOrder", std::string(1u, static_cast<char>(dor)));
    return *this;
}

std::vector<std::string> Mesh::axisLabels() const
{
    return getAttribute("axisLabels").get<std::vector<std::string> >();
}

Mesh &Mesh::setAxisLabels(std::vector<std::string> const &als)
{
    setAttribute("axisLabels", als);
    return *this;
}

template <typename T, typename>
Mesh &Mesh::setGridSpacing(std::vector<T> const &gs)
{
    static_assert(
        std::is_floating_point<T>::value,
        "Type of attribute must be floating point");

    setAttribute("gridSpacing", gs);
    return *this;
}

template Mesh &Mesh::setGridSpacing(std::vector<float> const &gs);
template Mesh &Mesh::setGridSpacing(std::vector<double> const &gs);
template Mesh &Mesh::setGridSpacing(std::vector<long double> const &gs);

std::vector<double> Mesh::gridGlobalOffset() const
{
    return getAttribute("gridGlobalOffset").get<std::vector<double> >();
}

Mesh &Mesh::setGridGlobalOffset(std::vector<double> const &ggo)
{
    setAttribute("gridGlobalOffset", ggo);
    return *this;
}

double Mesh::gridUnitSI() const
{
    return getAttribute("gridUnitSI").get<double>();
}

Mesh &Mesh::setGridUnitSI(double gusi)
{
    setAttribute("gridUnitSI", gusi);
    return *this;
}

Mesh &Mesh::setUnitDimension(std::map<UnitDimension, double> const &udim)
{
    if (!udim.empty())
    {
        std::array<double, 7> tmpUnitDimension = this->unitDimension();
        for (auto const &entry : udim)
            tmpUnitDimension[static_cast<uint8_t>(entry.first)] = entry.second;
        setAttribute("unitDimension", tmpUnitDimension);
    }
    return *this;
}

template <typename T, typename>
Mesh &Mesh::setTimeOffset(T to)
{
    static_assert(
        std::is_floating_point<T>::value,
        "Type of attribute must be floating point");

    setAttribute("timeOffset", to);
    return *this;
}

template Mesh &Mesh::setTimeOffset(long double);

template Mesh &Mesh::setTimeOffset(double);

template Mesh &Mesh::setTimeOffset(float);

void Mesh::flush_impl(
    std::string const &name, internal::FlushParams const &flushParams)
{
    if (IOHandler()->m_frontendAccess == Access::READ_ONLY)
    {
        for (auto &comp : *this)
            comp.second.flush(comp.first, flushParams);
    }
    else
    {
        if (!written())
        {
            if (scalar())
            {
                MeshRecordComponent &mrc = at(RecordComponent::SCALAR);
                mrc.parent() = parent();
                mrc.flush(name, flushParams);
                IOHandler()->flush(flushParams);
                writable().abstractFilePosition =
                    mrc.writable().abstractFilePosition;
                written() = true;
            }
            else
            {
                Parameter<Operation::CREATE_PATH> pCreate;
                pCreate.path = name;
                IOHandler()->enqueue(IOTask(this, pCreate));
                for (auto &comp : *this)
                    comp.second.parent() = &this->writable();
            }
        }

        if (scalar())
        {
            for (auto &comp : *this)
            {
                comp.second.flush(name, flushParams);
                writable().abstractFilePosition =
                    comp.second.writable().abstractFilePosition;
            }
        }
        else
        {
            for (auto &comp : *this)
                comp.second.flush(comp.first, flushParams);
        }

        flushAttributes(flushParams);
    }
}

void Mesh::read()
{
    auto map = eraseStaleEntries();

    using DT = Datatype;
    Parameter<Operation::READ_ATT> aRead;

    aRead.name = "geometry";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::STRING)
    {
        std::string tmpGeometry = Attribute(*aRead.resource).get<std::string>();
        if ("cartesian" == tmpGeometry)
            setGeometry(Geometry::cartesian);
        else if ("thetaMode" == tmpGeometry)
            setGeometry(Geometry::thetaMode);
        else if ("cylindrical" == tmpGeometry)
            setGeometry(Geometry::cylindrical);
        else if ("spherical" == tmpGeometry)
            setGeometry(Geometry::spherical);
        else
            setGeometry(tmpGeometry);
    }
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'geometry'");

    aRead.name = "dataOrder";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::CHAR)
        setDataOrder(
            static_cast<DataOrder>(Attribute(*aRead.resource).get<char>()));
    else if (*aRead.dtype == DT::STRING)
    {
        std::string tmpDataOrder =
            Attribute(*aRead.resource).get<std::string>();
        if (tmpDataOrder.size() == 1)
            setDataOrder(static_cast<DataOrder>(tmpDataOrder[0]));
        else
            throw std::runtime_error(
                "Unexpected Attribute value for 'dataOrder': " + tmpDataOrder);
    }
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'dataOrder'");

    aRead.name = "axisLabels";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::VEC_STRING || *aRead.dtype == DT::STRING)
        setAxisLabels(
            Attribute(*aRead.resource).get<std::vector<std::string> >());
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'axisLabels'");

    aRead.name = "gridSpacing";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    Attribute a = Attribute(*aRead.resource);
    if (*aRead.dtype == DT::VEC_FLOAT || *aRead.dtype == DT::FLOAT)
        setGridSpacing(a.get<std::vector<float> >());
    else if (*aRead.dtype == DT::VEC_DOUBLE || *aRead.dtype == DT::DOUBLE)
        setGridSpacing(a.get<std::vector<double> >());
    else if (
        *aRead.dtype == DT::VEC_LONG_DOUBLE || *aRead.dtype == DT::LONG_DOUBLE)
        setGridSpacing(a.get<std::vector<long double> >());
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'gridSpacing'");

    aRead.name = "gridGlobalOffset";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::VEC_DOUBLE || *aRead.dtype == DT::DOUBLE)
        setGridGlobalOffset(
            Attribute(*aRead.resource).get<std::vector<double> >());
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'gridGlobalOffset'");

    aRead.name = "gridUnitSI";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::DOUBLE)
        setGridUnitSI(Attribute(*aRead.resource).get<double>());
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'gridUnitSI'");

    if (scalar())
    {
        /* using operator[] will incorrectly update parent */
        map.at(MeshRecordComponent::SCALAR).read();
    }
    else
    {
        Parameter<Operation::LIST_PATHS> pList;
        IOHandler()->enqueue(IOTask(this, pList));
        IOHandler()->flush(internal::defaultFlushParams);

        Parameter<Operation::OPEN_PATH> pOpen;
        for (auto const &component : *pList.paths)
        {
            MeshRecordComponent &rc = map[component];
            pOpen.path = component;
            IOHandler()->enqueue(IOTask(&rc, pOpen));
            *rc.m_isConstant = true;
            rc.read();
        }

        Parameter<Operation::LIST_DATASETS> dList;
        IOHandler()->enqueue(IOTask(this, dList));
        IOHandler()->flush(internal::defaultFlushParams);

        Parameter<Operation::OPEN_DATASET> dOpen;
        for (auto const &component : *dList.datasets)
        {
            MeshRecordComponent &rc = map[component];
            dOpen.name = component;
            IOHandler()->enqueue(IOTask(&rc, dOpen));
            IOHandler()->flush(internal::defaultFlushParams);
            rc.written() = false;
            rc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
            rc.written() = true;
            rc.read();
        }
    }

    readBase();

    readAttributes(ReadMode::FullyReread);
}
} // namespace openPMD

std::ostream &
openPMD::operator<<(std::ostream &os, openPMD::Mesh::Geometry const &go)
{
    switch (go)
    {
    case openPMD::Mesh::Geometry::cartesian:
        os << "cartesian";
        break;
    case openPMD::Mesh::Geometry::thetaMode:
        os << "thetaMode";
        break;
    case openPMD::Mesh::Geometry::cylindrical:
        os << "cylindrical";
        break;
    case openPMD::Mesh::Geometry::spherical:
        os << "spherical";
        break;
    case openPMD::Mesh::Geometry::other:
        os << "other";
        break;
    }
    return os;
}

std::ostream &
openPMD::operator<<(std::ostream &os, openPMD::Mesh::DataOrder const &dor)
{
    switch (dor)
    {
    case openPMD::Mesh::DataOrder::C:
        os << 'C';
        break;
    case openPMD::Mesh::DataOrder::F:
        os << 'F';
        break;
    }
    return os;
}
