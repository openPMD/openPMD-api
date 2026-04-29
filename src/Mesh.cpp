/* Copyright 2017-2025 Fabian Koller, Axel Huebl, Franz Poeschel
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
#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/ThrowError.hpp"
#include "openPMD/UnitDimension.hpp"
#include "openPMD/auxiliary/DerefDynamicCast.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Writable.hpp"
#include "openPMD/backend/scientific_defaults/ConfigAttribute.hpp"
#include "openPMD/backend/scientific_defaults/ScientificDefaults.hpp"

#include <algorithm>
#include <iostream>

namespace openPMD
{
void Mesh::visitHierarchy(HierarchyVisitor &v, bool recursive)
{
    visitHierarchyImpl<Mesh>(v, recursive);
}

Mesh::Mesh() = default;

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
    return getAttribute("axisLabels").get<std::vector<std::string>>();
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
    return getAttribute("gridGlobalOffset").get<std::vector<double>>();
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
    if (auto standard = IOHandler()->m_standard;
        standard >= OpenpmdStandard::v_2_0_0)
    {
        std::cerr << "[Mesh::setGridUnitSI] Warning: Setting a scalar "
                     "`gridUnitSI` in a file with openPMD version '" +
                std::string(auxiliary::formatStandard(standard)) +
                "'. Consider specifying a vector instead in order to "
                "specify the gridUnitSI per axis (ref.: "
                "https://github.com/openPMD/openPMD-standard/pull/193).\n";
    }
    setAttribute("gridUnitSI", gusi);
    return *this;
}

Mesh &Mesh::setGridUnitSI(std::vector<double> const &gusi)
{
    return setGridUnitSIPerDimension(gusi);
}

auto Mesh::retrieveDimensionality() const -> uint64_t
{
    if (containsAttribute("axisLabels"))
    {
        return axisLabels().size();
    }

    // maybe we have record components and can ask them
    if (auto it = begin(); it != end())
    {
        return it->second.getDimensionality();
    }
    /*
     * Since some backends cannot distinguish between vector and
     * scalar values, the most likely answer here is 1.
     */
    return 1;
}

void Mesh::scientificDefaults_impl(
    internal::WriteOrRead wor, OpenpmdStandard standard)
{
    using namespace internal;
    auto float_types = get_float_types();
    auto int_types = get_int_types();
    auto numerical_types = get_numerical_types();
    auto string_types = get_string_types();
    auto dimensionality = retrieveDimensionality();

    defaultAttribute(*this, "geometry")
        .template withSetter<Mesh>(
            Mesh::Geometry::cartesian, &Mesh::setGeometry)
        .withReader(
            string_types, require_type(*this, &setMeshGeometryFromString))(wor);

    defaultAttribute(*this, "dataOrder")
        .template withSetter<Mesh>(Mesh::DataOrder::C, &Mesh::setDataOrder)
        .withReader(
            string_types, require_type(*this, &setMeshDataOrderFromChar))(wor);

    defaultAttribute(*this, "axisLabels")
        .template withSetter<Mesh, std::vector<std::string> const &>(
            [&]() -> std::vector<std::string> {
                return auxiliary::createDefaultAxisLabels(dimensionality);
            },
            &Mesh::setAxisLabels)
        .withReader(string_types, require_vector)(wor);

    defaultAttribute(*this, "gridSpacing")
        .template withSetter<Mesh, std::vector<double> const &>(
            [&]() {
                return auxiliary::createDefaultVector(dimensionality, 1.0);
            },
            &Mesh::setGridSpacing)
        .withReader(float_types, require_vector)
        .withReader(int_types, require_type<std::vector<double>>())(wor);

    defaultAttribute(*this, "gridGlobalOffset")
        .template withSetter<Mesh, std::vector<double> const &>(
            [&]() {
                return auxiliary::createDefaultVector(dimensionality, 0.0);
            },
            &Mesh::setGridGlobalOffset)
        .withReader(numerical_types, require_type<std::vector<double>>())(wor);

    defaultAttribute(*this, "timeOffset")
        .template withSetter<Mesh>(0.f, &Mesh::setTimeOffset)
        .withReader(float_types, require_scalar)
        .withReader(int_types, require_type<double>())(wor);

    if (standard >= OpenpmdStandard::v_2_0_0)
    {
        defaultAttribute(*this, "gridUnitSI")
            .template withSetter<Mesh, std::vector<double> const &>(
                [&]() {
                    return auxiliary::createDefaultVector(dimensionality, 1.);
                },
                &Mesh::setGridUnitSIPerDimension)
            .withReader(int_types, require_type<std::vector<double>>())(wor);
    }
    else
    {
        defaultAttribute(*this, "gridUnitSI")
            .template withSetter<Mesh>(1.0, &Mesh::setGridUnitSI)
            .withReader(numerical_types, require_type<double>())(wor);
    }

    BaseRecord<MeshRecordComponent>::scientificDefaults_impl(wor, standard);
}
std::vector<double> Mesh::gridUnitSIPerDimension() const
{
    if (containsAttribute("gridUnitSI"))
    {
        if (IOHandler()->m_standard < OpenpmdStandard::v_2_0_0)
        {
            // If the openPMD version is lower than 2.0, the gridUnitSI is a
            // scalar interpreted for all axes. Copy it d times.
            return std::vector<double>(
                retrieveDimensionality(),
                getAttribute("gridUnitSI").get<double>());
        }
        return getAttribute("gridUnitSI").get<std::vector<double>>();
    }
    else
    {
        // gridUnitSI is an optional attribute
        // if it is missing, the mesh is interpreted as unscaled
        return std::vector<double>(retrieveDimensionality(), 1.);
    }
}

Mesh &Mesh::setGridUnitSIPerDimension(std::vector<double> const &gridUnitSI)
{
    if (auto standard = IOHandler()->m_standard;
        standard < OpenpmdStandard::v_2_0_0)
    {
        throw error::IllegalInOpenPMDStandard(
            "[Mesh::setGridUnitSI] Setting `gridUnitSI` as a vector in a "
            "file with openPMD version '" +
            std::string(auxiliary::formatStandard(standard)) +
            "', but per-axis specification is only supported as of "
            "openPMD 2.0. Either upgrade the file to openPMD >= 2.0 "
            "or specify a scalar that applies to all axes.");
    }
    setAttribute("gridUnitSI", gridUnitSI);
    return *this;
}

Mesh &Mesh::setUnitDimension(unit_representations::AsMap const &udim)
{
    if (!udim.empty())
    {
        std::array<double, 7> tmpUnitDimension =
            this->containsAttribute("unitDimension") ? this->unitDimension()
                                                     : std::array<double, 7>{0};
        unit_representations::auxiliary::fromMapOfUnitDimension(
            tmpUnitDimension.data(), udim);
        setAttribute("unitDimension", tmpUnitDimension);
    }
    return *this;
}

Mesh &Mesh::setUnitDimension(unit_representations::AsArray const &udim)
{
    return setUnitDimension(
        unit_representations::asMap(udim, /* skip_zeros = */ false));
}

Mesh &Mesh::setGridUnitDimension(unit_representations::AsMaps const &udims)
{
    auto rawGridUnitDimension = [&udims, this]() {
        if (!this->containsAttribute("gridUnitDimension"))
        {
            std::vector<double> res(udims.size() * 7);
            // for (size_t i = 0; i < udims.size(); ++i)
            // {
            //     res[7 * i] = 1;
            // }
            return res;
        }
        else
        {
            return this->getAttribute("gridUnitDimension")
                .get<std::vector<double>>();
        }
    }();
    rawGridUnitDimension.resize(7 * udims.size());
    auto cursor = rawGridUnitDimension.begin();
    for (auto const &udim : udims)
    {
        unit_representations::auxiliary::fromMapOfUnitDimension(&*cursor, udim);
        cursor += 7;
    }
    setAttribute("gridUnitDimension", rawGridUnitDimension);
    return *this;
}

Mesh &Mesh::setGridUnitDimension(unit_representations::AsArrays const &udims)
{
    return setGridUnitDimension(
        unit_representations::asMaps(udims, /* skip_zeros = */ false));
}

unit_representations::AsArrays Mesh::gridUnitDimension() const
{
    if (containsAttribute("gridUnitDimension"))
    {
        std::vector<double> rawRes =
            getAttribute("gridUnitDimension").get<std::vector<double>>();
        if (rawRes.size() % 7 != 0)
        {
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                std::nullopt,
                "[Mesh::gridUnitDimension()] `gridUnitDimension` attribute "
                "must have a length equal to a multiple of 7.");
        }
        unit_representations::AsArrays res(rawRes.size() / 7);
        for (size_t dim = 0; dim < res.size(); ++dim)
        {
            std::copy_n(rawRes.begin() + dim * 7, 7, res.at(dim).begin());
        }
        return res;
    }
    else
    {
        // gridUnitDimension is an optional attribute
        // if it is missing, the mesh is interpreted as spatial
        auto spatialMesh =
            unit_representations::asArray({{UnitDimension::L, 1}});
        auto dim = retrieveDimensionality();
        unit_representations::AsArrays res(dim, spatialMesh);
        return res;
    }
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
    if (!dirtyRecursive())
    {
        return;
    }
    if (access::readOnly(IOHandler()->m_frontendAccess))
    {
        auto &m = get();
        if (m.m_datasetDefined)
        {
            T_RecordComponent::flush(SCALAR, flushParams);
        }
        else
        {
            for (auto &comp : *this)
                comp.second.flush(comp.first, flushParams);
        }
    }
    else
    {
        if (!written())
        {
            if (scalar())
            {
                MeshRecordComponent &mrc = *this;
                mrc.flush(name, flushParams);
            }
            else
            {
                Parameter<Operation::CREATE_PATH> pCreate;
                pCreate.path = name;
                IOHandler()->enqueue(IOTask(this, pCreate));
                for (auto &comp : *this)
                {
                    comp.second.parent() = &this->writable();
                    comp.second.flush(comp.first, flushParams);
                }
            }
        }
        else
        {
            if (scalar())
            {
                T_RecordComponent::flush(name, flushParams);
            }
            else
            {
                for (auto &comp : *this)
                    comp.second.flush(comp.first, flushParams);
            }
        }
        flushAttributes(flushParams);
    }
}

void Mesh::read()
{
    internal::HomogenizeExtents homogenizeExtents(
        IOHandler()->m_verify_homogeneous_extents);
    internal::EraseStaleEntries<Mesh> map{*this};

    if (scalar())
    {
        T_RecordComponent::read();
        homogenizeExtents.check_extent(*this, *this);
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
            rc.get().m_isConstant = true;
            try
            {
                rc.read();
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read record component '" << component
                          << "' and will skip it due to read error:\n"
                          << err.what() << std::endl;
                map.forget(component);
                continue;
            }
            homogenizeExtents.check_extent(*this, rc);
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
            rc.setWritten(false, Attributable::EnqueueAsynchronously::No);
            rc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
            rc.setWritten(true, Attributable::EnqueueAsynchronously::No);
            try
            {
                rc.read();
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read record component '" << component
                          << "' and will skip it due to read error:\n"
                          << err.what() << std::endl;
                map.forget(component);
                continue;
            }
            homogenizeExtents.check_extent(*this, rc);
        }
    }

    std::move(homogenizeExtents).homogenize(*this);

    readAttributes(ReadMode::FullyReread);
    internal::ScientificDefaults::readDefaults(*this, IOHandler()->m_standard);
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
