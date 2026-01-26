#include "openPMD/backend/ScientificDefaults.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/ThrowError.hpp"
#include "openPMD/backend/ScientificDefaults_internal.hpp"

#include "openPMD/Error.hpp"
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
#include "openPMD/backend/Variant_internal.hpp"
#include "openPMD/backend/Writable.hpp"

#include <iostream>
#include <optional>
#include <type_traits>
#include <utility>

namespace openPMD::internal
{

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
        asChild().ScientificDefaults<Parent>::addDefaults(standard);
    }
    else
    {
        asChild().ScientificDefaults<Parent>::readDefaults(standard);
    }
}

template <typename Child>
void ScientificDefaults<Child>::addDefaultsRecursively(OpenpmdStandard standard)
{
    addDefaults(standard);
    if constexpr (IsContainer_v<Child>)
    {
        using Container_t = AsContainer_t<Child>;
        using mapped_type = typename Container_t::mapped_type;
        if constexpr (HasScientificDefaults_v<mapped_type>)
        {
            for (auto &[_, right] : asChild())
            {
                (void)_;
                right.ScientificDefaults<mapped_type>::addDefaultsRecursively(
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
            right.ScientificDefaults<Mesh>::addDefaultsRecursively(standard);
        }
        for (auto &[_, right] : asChild().particles)
        {
            (void)_;
            right.ScientificDefaults<ParticleSpecies>::addDefaultsRecursively(
                standard);
        }
    }
    else if constexpr (std::is_same_v<Child, ParticleSpecies>)
    {
        for (auto &[_, right] : asChild().particlePatches)
        {
            (void)_;
            right.ScientificDefaults<PatchRecord>::addDefaultsRecursively(
                standard);
        }
    }
}

template <typename SetterFunctor>
auto ensureFloatingVector(SetterFunctor &&set)
{
    return [set_lambda = std::forward<SetterFunctor>(set)](
               auto &, auto &&converted_val, Attribute const &orig_val) {
        std::visit(
            [&converted_val, &set_lambda](auto const &val) {
                using actual_type =
                    std::remove_cv_t<std::remove_reference_t<decltype(val)>>;
                if constexpr (auxiliary::IsVector_v<actual_type>)
                {
                    if constexpr (std::is_floating_point_v<
                                      typename actual_type::value_type>)
                    {
                        set_lambda(val);
                    }
                    else
                    {
                        set_lambda(
                            static_cast<decltype(converted_val)>(
                                converted_val));
                    }
                }
                else
                {
                    if constexpr (std::is_floating_point_v<actual_type>)
                    {
                        set_lambda(std::vector<actual_type>{val});
                    }
                    else
                    {
                        set_lambda(
                            static_cast<decltype(converted_val)>(
                                converted_val));
                    }
                }
            },
            orig_val.getVariant<attribute_types>());
    };
}

template <typename SetterFunctor>
auto ensureFloatingScalar(SetterFunctor &&set)
{
    return [set_lambda = std::forward<SetterFunctor>(set)](
               auto &, auto &&converted_val, Attribute const &orig_val) {
        std::visit(
            [&converted_val, &set_lambda](auto const &val) {
                using actual_type =
                    std::remove_cv_t<std::remove_reference_t<decltype(val)>>;
                if constexpr (std::is_floating_point_v<actual_type>)
                {
                    set_lambda(val);
                }
                else
                {
                    set_lambda(
                        static_cast<decltype(converted_val)>(converted_val));
                }
            },
            orig_val.getVariant<attribute_types>());
    };
}

template <typename Child>
template <bool write>
void ScientificDefaults<Child>::defaults_impl(OpenpmdStandard standard)
{
    using maybe_read_error = std::optional<error::ReadError>;
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
            .withReader(ensureFloatingScalar(
                [this](auto &&val) { this->asChild().setTime(val); }))(wor);
        defaultAttribute("dt")
            .template withSetter<Iteration>(1., &Iteration::setDt)
            .withReader(ensureFloatingScalar(
                [this](auto &&val) { this->asChild().setDt(val); }))(wor);
        defaultAttribute("timeUnitSI")
            .template withSetter<Iteration>(1.0, &Iteration::setTimeUnitSI)
            .withReader()(wor);
    }
    else if constexpr (std::is_same_v<Child, Mesh>)
    {
        auto dimensionality = asChild().retrieveDimensionality();

        defaultAttribute("geometry")
            .template withSetter<Mesh>(
                Mesh::Geometry::cartesian, &Mesh::setGeometry)
            .withReader([](Mesh &m, std::string val) {
                if ("cartesian" == val)
                    m.setGeometry(Mesh::Geometry::cartesian);
                else if ("thetaMode" == val)
                    m.setGeometry(Mesh::Geometry::thetaMode);
                else if ("cylindrical" == val)
                    m.setGeometry(Mesh::Geometry::cylindrical);
                else if ("spherical" == val)
                    m.setGeometry(Mesh::Geometry::spherical);
                else
                    m.setGeometry(std::move(val));
            })(wor);

        defaultAttribute("dataOrder")
            .template withSetter<Mesh>(Mesh::DataOrder::C, &Mesh::setDataOrder)
            .withReader([](Mesh &m, char val) -> maybe_read_error {
                if (val == 'C' || val == 'F')
                {
                    m.setDataOrder(static_cast<Mesh::DataOrder>(val));
                    return std::nullopt;
                }
                else
                {
                    return error::ReadError(
                        error::AffectedObject::Attribute,
                        error::Reason::UnexpectedContent,
                        std::nullopt,
                        "Data order must be either C or F.");
                }
            })(wor);

        defaultAttribute("axisLabels")
            .template withSetter<Mesh, std::vector<std::string> const &>(
                [&]() -> std::vector<std::string> {
                    switch (dimensionality)
                    {
                    case 0:
                    case 1:
                        return {"x"};
                    case 2:
                        return {"x", "y"};
                    case 3:
                        return {"x", "y", "z"};
                    default:
                        if (dimensionality < 100)
                        {
                            // x1, x2, x3, x4, ...
                            std::vector<std::string> res;
                            res.reserve(dimensionality);
                            for (uint64_t i = 0; i < dimensionality; ++i)
                            {
                                res.emplace_back("x" + std::to_string(i));
                            }
                            return res;
                        }
                        else
                        {
                            return {
                                "Please verify dimensionality. Was inferred as "
                                "'" +
                                std::to_string(dimensionality) +
                                "'. Seems a bit much."};
                        }
                    }
                    return std::vector<std::string>{"x", "y", "z"};
                },
                &Mesh::setAxisLabels)
            .withReader()(wor);

        defaultAttribute("gridSpacing")
            .template withSetter<Mesh, std::vector<double> const &>(
                [&]() {
                    if (dimensionality < 100)
                    {
                        return std::vector<double>(dimensionality, 1.0);
                    }
                    else
                    {
                        return std::vector<double>{1.0};
                    }
                },
                &Mesh::setGridSpacing)
            .withReader(ensureFloatingVector([this](auto &&val) {
                asChild().setGridSpacing(static_cast<decltype(val)>(val));
            }))(wor);

        defaultAttribute("gridGlobalOffset")
            .template withSetter<Mesh, std::vector<double> const &>(
                [&]() {
                    if (dimensionality < 100)
                    {
                        return std::vector<double>(dimensionality, 0.0);
                    }
                    else
                    {
                        return std::vector<double>{0.0};
                    }
                },
                &Mesh::setGridGlobalOffset)
            .withReader(
                /* gridGlobalOffset requires vector<double> precisely, so no
                   handling for different floating types here */
                )(wor);

        defaultAttribute("timeOffset")
            .template withSetter<Mesh>(0.f, &Mesh::setTimeOffset)
            .withReader(ensureFloatingScalar([this](auto &&val) {
                asChild().setAttribute("timeOffset", val);
            }))(wor);

        if (standard >= OpenpmdStandard::v_2_0_0)
        {
            defaultAttribute("gridUnitSI")
                .template withSetter<Mesh, std::vector<double> const &>(
                    [&]() {
                        if (dimensionality < 100)
                        {
                            return std::vector<double>(dimensionality, 1.);
                        }
                        else
                        {
                            return std::vector<double>{1.};
                        }
                    },
                    &Mesh::setGridUnitSIPerDimension)
                .withReader()(wor);
        }
        else
        {
            defaultAttribute("gridUnitSI")
                .template withSetter<Mesh>(1.0, &Mesh::setGridUnitSI)
                .withReader()(wor);
        }

        addParentDefaults<BaseRecord<MeshRecordComponent>, write>(standard);
    }
    else if constexpr (std::is_same_v<Child, Record>)
    {
        defaultAttribute("timeOffset")
            .template withSetter<Record>(0.f, &Record::setTimeOffset)(wor);

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
            .withReader(ensureFloatingScalar([this](auto &&val) {
                asChild().setAttribute("timeOffset", val);
            }))(wor);

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
            .withReader()(wor);
    }
    else if constexpr (std::is_same_v<Child, MeshRecordComponent>)
    {
        auto dimensionality = asChild().getDimensionality();

        defaultAttribute("position")
            .template withSetter<MeshRecordComponent>(
                [&]() {
                    if (dimensionality < 100)
                    {
                        return std::vector<double>(dimensionality, 0.5);
                    }
                    else
                    {
                        return std::vector<double>{0.0};
                    }
                },
                &MeshRecordComponent::setPosition)
            .withReader(ensureFloatingVector([this](auto &&val) {
                this->asChild().setPosition(static_cast<decltype(val)>(val));
            }))(wor);

        addParentDefaults<RecordComponent, write>(standard);
    }
    else if constexpr (std::is_same_v<Child, PatchRecordComponent>)
    {
        // We don't require unitSI for PatchRecordComponent
        //
        // addParentDefaults<RecordComponent, write>();
    }
    else if constexpr (auxiliary::IsTemplateBaseOf_v<BaseRecord, Child>)
    {
        defaultAttribute("unitDimension")
            .withGenericSetter(unit_representations::AsArray{})
            .withReader()(wor);
    }
}

template <typename Child>
void ScientificDefaults<Child>::addDefaults(OpenpmdStandard standard)
{
    defaults_impl</* write = */ true>(standard);
}

template <typename Child>
void ScientificDefaults<Child>::readDefaults(OpenpmdStandard standard)
{
    defaults_impl</* write = */ false>(standard);
}

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
