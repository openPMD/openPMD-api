#if 0
#include "openPMD/backend/ScientificDefaults.hpp"
#include "openPMD/backend/ScientificDefaults_auxiliary.hpp"
#include "openPMD/backend/ScientificDefaults_impl.hpp"

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Record.hpp"
#include "openPMD/UnitDimension.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"
#include "openPMD/backend/Writable.hpp"

#include <iostream>
#include <type_traits>
#include <utility>

namespace openPMD::internal
{
// 7. ScientificDefaults template implementations
template <typename Child>
auto ScientificDefaults<Child>::asChild() -> Child &
{
    return *static_cast<Child *>(this);
}

template <typename Child>
auto ScientificDefaults<Child>::asChild() const -> Child const &
{
    return *static_cast<Child const *>(this);
}

template <typename Child>
[[nodiscard]] auto
ScientificDefaults<Child>::defaultAttribute(char const *attrName)
    -> ConfigAttribute
{
    return ConfigAttribute{asChild(), attrName};
}

template <typename Child>
template <typename Parent, bool write>
void ScientificDefaults<Child>::addParentDefaults(OpenpmdStandard standard)
{
    // Cannot directly call read_impl as it is private
    if constexpr (write)
    {
        asChild().ScientificDefaults<Parent>::writeDefaults(standard);
    }
    else
    {
        asChild().ScientificDefaults<Parent>::readDefaults(standard);
    }
}

template <typename Child>
void ScientificDefaults<Child>::writeDefaultsRecursively(
    OpenpmdStandard standard)
{
    writeDefaults(standard);
    if constexpr (IsContainer_v<Child>)
    {
        using Container_t = AsContainer_t<Child>;
        using mapped_type = typename Container_t::mapped_type;
        if constexpr (HasScientificDefaults_v<mapped_type>)
        {
            for (auto &[_, right] : asChild())
            {
                (void)_;
                right.ScientificDefaults<mapped_type>::writeDefaultsRecursively(
                    standard);
            }
        }
    }
    // sic! no else

    if constexpr (std::is_same_v<Child, Iteration>)
    {
        for (auto &[_, right] : asChild().meshes)
        {
            (void)_;
            right.ScientificDefaults<Mesh>::writeDefaultsRecursively(standard);
        }
        for (auto &[_, right] : asChild().particles)
        {
            (void)_;
            right.ScientificDefaults<ParticleSpecies>::writeDefaultsRecursively(
                standard);
        }
    }
    else if constexpr (std::is_same_v<Child, ParticleSpecies>)
    {
        for (auto &[_, right] : asChild().particlePatches)
        {
            (void)_;
            right.ScientificDefaults<PatchRecord>::writeDefaultsRecursively(
                standard);
        }
    }
}

template <typename Child>
template <bool write>
void ScientificDefaults<Child>::defaults_impl(OpenpmdStandard standard)
{
    auto float_types = get_float_types();
    auto string_types = get_string_types();

    constexpr auto const wor = write ? WriteOrRead::Write : WriteOrRead::Read;

    // First some verifications
    if constexpr (write && auxiliary::IsTemplateBaseOf_v<BaseRecord, Child>)
    {
        if (asChild().empty() && !asChild().datasetDefined())
        {
            std::cerr
                << "Cannot flush Record without any contained components: '"
                << asChild().myPath().openPMDPath() << "'. Will ignore.";
            if (asChild().written())
            {
                std::cerr << "\n(Note: The Record seems to have been written "
                             "previously?)";
            }
            std::cerr << std::endl;
            return;
        }
    }

    if constexpr (std::is_same_v<Child, Iteration>)
    {
        defaultAttribute("time")
            .template withSetter<Iteration>(0., &Iteration::setTime)
            .withReader(float_types, require_scalar)(wor);
        defaultAttribute("dt")
            .template withSetter<Iteration>(1., &Iteration::setDt)
            .withReader(float_types, require_scalar)(wor);
        defaultAttribute("timeUnitSI")
            .template withSetter<Iteration>(1.0, &Iteration::setTimeUnitSI)
            .withReader(float_types, require_type<double>())(wor);
    }
    else if constexpr (std::is_same_v<Child, Mesh>)
    {
        auto dimensionality = asChild().retrieveDimensionality();

        defaultAttribute("geometry")
            .template withSetter<Mesh>(
                Mesh::Geometry::cartesian, &Mesh::setGeometry)
            .withReader(
                string_types,
                require_type(asChild(), &setMeshGeometryFromString))(wor);

        defaultAttribute("dataOrder")
            .template withSetter<Mesh>(Mesh::DataOrder::C, &Mesh::setDataOrder)
            .withReader(
                string_types,
                require_type(asChild(), &setMeshDataOrderFromChar))(wor);

        defaultAttribute("axisLabels")
            .template withSetter<Mesh, std::vector<std::string> const &>(
                [&]() -> std::vector<std::string> {
                    return auxiliary::createDefaultAxisLabels(dimensionality);
                },
                &Mesh::setAxisLabels)
            .withReader(string_types, require_vector)(wor);

        defaultAttribute("gridSpacing")
            .template withSetter<Mesh, std::vector<double> const &>(
                [&]() {
                    return auxiliary::createDefaultVector(dimensionality, 1.0);
                },
                &Mesh::setGridSpacing)
            .withReader(float_types, require_vector)(wor);

        defaultAttribute("gridGlobalOffset")
            .template withSetter<Mesh, std::vector<double> const &>(
                [&]() {
                    return auxiliary::createDefaultVector(dimensionality, 0.0);
                },
                &Mesh::setGridGlobalOffset)
            .withReader(float_types, require_type<std::vector<double>>())(wor);

        defaultAttribute("timeOffset")
            .template withSetter<Mesh>(0.f, &Mesh::setTimeOffset)
            .withReader(float_types, require_scalar)(wor);

        if (standard >= OpenpmdStandard::v_2_0_0)
        {
            defaultAttribute("gridUnitSI")
                .template withSetter<Mesh, std::vector<double> const &>(
                    [&]() {
                        return auxiliary::createDefaultVector(
                            dimensionality, 1.);
                    },
                    &Mesh::setGridUnitSIPerDimension)
                .withReader(float_types, require_type<std::vector<double>>())(
                    wor);
        }
        else
        {
            defaultAttribute("gridUnitSI")
                .template withSetter<Mesh>(1.0, &Mesh::setGridUnitSI)
                .withReader(float_types, require_type<std::vector<double>>())(
                    wor);
        }

        addParentDefaults<BaseRecord<MeshRecordComponent>, write>(standard);
    }
    else if constexpr (std::is_same_v<Child, Record>)
    {
        defaultAttribute("timeOffset")
            .template withSetter<Record>(0.f, &Record::setTimeOffset)
            .withReader(float_types, require_scalar)(wor);

        auto const &keyInParent = asChild().writable().ownKeyWithinParent;
        if (keyInParent == "position" || keyInParent == "positionOffset")
        {
            defaultAttribute("unitDimension")
                .template withSetter<
                    Record,
                    unit_representations::AsMap const &>(
                    []() {
                        return unit_representations::AsMap{
                            {UnitDimension::L, 1.0}};
                    },
                    &Record::setUnitDimension)(wor);
        }

        defaultAttribute("timeOffset")
            .template withSetter<Record>(0.f, &Record::setTimeOffset)
            .withReader(float_types, require_scalar)(wor);

        addParentDefaults<BaseRecord<RecordComponent>, write>(standard);
    }
    else if constexpr (std::is_same_v<Child, PatchRecord>)
    {
        addParentDefaults<BaseRecord<PatchRecordComponent>, write>(standard);
    }
    else if constexpr (std::is_same_v<Child, RecordComponent>)
    {
        defaultAttribute("unitSI")
            .template withSetter<RecordComponent>(
                1.0, &RecordComponent::setUnitSI)
            .withReader(float_types, require_type<double>())(wor);
    }
    else if constexpr (std::is_same_v<Child, MeshRecordComponent>)
    {
        auto dimensionality = asChild().getDimensionality();

        defaultAttribute("position")
            .template withSetter<MeshRecordComponent>(
                [&]() {
                    return auxiliary::createDefaultVector(dimensionality, 0.5);
                },
                &MeshRecordComponent::setPosition)
            .withReader(float_types, require_vector)(wor);

        addParentDefaults<RecordComponent, write>(standard);
    }
    else if constexpr (std::is_same_v<Child, PatchRecordComponent>)
    {
        // We don't require unitSI for PatchRecordComponent
        //
        // addParentDefaults<RecordComponent, write>();
        //
        // But we still set it when writing. Doesnt hurt and some readers might
        // expect it.
        defaultAttribute("unitSI").template withSetter<PatchRecordComponent>(
            1.0, &PatchRecordComponent::setUnitSI)
            // Do NOT add a reader here.
            // .withReader(float_types, require_type<double>())
            (wor);
    }
    else if constexpr (detail::IsBaseRecord_v<Child>)
    {
        defaultAttribute("unitDimension")
            .withGenericSetter(unit_representations::AsArray{})
            .withReader(
                float_types,
                require_type<unit_representations::AsArray>())(wor);
    }
    else if constexpr (std::is_same_v<Child, ParticleSpecies>)
    {
        // no-op
    }
    else
    {
        static_assert(auxiliary::dependent_false_v<Child>, "Unknown class");
    }
}

template <typename Child>
void ScientificDefaults<Child>::writeDefaults(OpenpmdStandard standard)
{
    defaults_impl</* write = */ true>(standard);
}

template <typename Child>
void ScientificDefaults<Child>::readDefaults(OpenpmdStandard standard)
{
    defaults_impl</* write = */ false>(standard);
}

// 8. Template instantiations
template class ScientificDefaults<Iteration>;
template class ScientificDefaults<Mesh>;
template class ScientificDefaults<MeshRecordComponent>;
template class ScientificDefaults<RecordComponent>;
template class ScientificDefaults<PatchRecordComponent>;
template class ScientificDefaults<ParticleSpecies>;
template class ScientificDefaults<Record>;
template class ScientificDefaults<BaseRecord<MeshRecordComponent>>;
template class ScientificDefaults<BaseRecord<PatchRecordComponent>>;
template class ScientificDefaults<BaseRecord<RecordComponent>>;
template class ScientificDefaults<PatchRecord>;
} // namespace openPMD::internal
#endif
