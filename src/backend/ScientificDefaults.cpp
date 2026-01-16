#include "openPMD/backend/ScientificDefaults.hpp"

#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Record.hpp"
#include "openPMD/UnitDimension.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"
#include "openPMD/backend/Writable.hpp"

#include <iostream>
#include <utility>

namespace openPMD::internal
{
template <typename RecordType, typename GetDefaultValue>
template <typename S>
auto ConfigAttribute<RecordType, GetDefaultValue>::withSetter(
    SetterType<S> setter)
    && -> ConfigAttributeWithSetter<RecordType, GetDefaultValue, SetterType<S>>
{
    return ConfigAttributeWithSetter<
        RecordType,
        GetDefaultValue,
        SetterType<S>>{std::move(*this), setter};
}

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
template <typename Setter, typename V>
void ScientificDefaults<Child>::addDefaultFor_worker(
    char const *, V &&value, Setter &&setter)
{
    (asChild().*setter)(std::forward<V>(value));
}

template <typename Child>
template <typename V>
void ScientificDefaults<Child>::addDefaultFor_worker(char const *key, V &&value)
{
    asChild().setAttribute(key, std::forward<V>(value));
}

template <typename Child>
template <typename F, typename... Args>
void ScientificDefaults<Child>::addDefaultFor_resolveValue(
    char const *key, F &&get_value, Args &&...args)
{
    if (asChild().containsAttribute(key))
    {
        return;
    }
    auto value = [&]() {
        if constexpr (detail::IsCallable_v<F>)
        {
            return get_value();
        }
        else
        {
            return get_value;
        }
    }();
    constexpr bool debug = true;
    if constexpr (debug)
    {
        std::cout << "\tInitializing default for '" << key << "' = '";
        if constexpr (
            auxiliary::IsVector_v<std::remove_reference_t<decltype(value)>> ||
            auxiliary::IsArray_v<std::remove_reference_t<decltype(value)>>)
        {
            auxiliary::write_vec_to_stream(std::cout, value);
        }
        else if constexpr (std::is_same_v<
                               std::remove_reference_t<decltype(value)>,
                               unit_representations::AsMap>)
        {
            std::cout << "Unit_Map";
        }
        else
        {
            std::cout << value;
        }
        std::cout << "'" << std::endl;
    }
    addDefaultFor_worker(key, std::move(value), std::forward<Args>(args)...);
}

template <typename Child>
template <typename Setter, typename F>
void ScientificDefaults<Child>::addDefaultFor(
    char const *key,
    F &&get_value,
    Child &(Child::*setter)(std::conditional_t<
                            std::is_void_v<Setter>,
                            std::remove_reference_t<detail::CallResult_t<F>>,
                            Setter>))
{
    addDefaultFor_resolveValue(key, std::forward<F>(get_value), setter);
}

template <typename Child>
template <typename F>
void ScientificDefaults<Child>::addDefaultFor(char const *key, F &&get_value)
{
    addDefaultFor_resolveValue(key, std::forward<F>(get_value));
}

template <typename Child>
template <typename Parent>
void ScientificDefaults<Child>::addParentDefaults()
{
    asChild().ScientificDefaults<Parent>::addDefaults();
}

template <typename Child>
void ScientificDefaults<Child>::finalize(Access at)
{
    if (access::write(at))
    {
        addDefaults();
    }
    if constexpr (IsContainer_v<Child>)
    {
        using Container_t = AsContainer_t<Child>;
        using mapped_type = typename Container_t::mapped_type;
        if constexpr (HasScientificDefaults_v<mapped_type>)
        {
            for (auto &[_, right] : asChild())
            {
                (void)_;
                right.ScientificDefaults<mapped_type>::finalize(at);
            }
        }
    }
    // sic! no else

    if constexpr (std::is_same_v<Child, Iteration>)
    {
        for (auto &[_, right] : asChild().meshes)
        {
            (void)_;
            right.ScientificDefaults<Mesh>::finalize(at);
        }
        for (auto &[_, right] : asChild().particles)
        {
            (void)_;
            right.ScientificDefaults<ParticleSpecies>::finalize(at);
        }
    }
    else if constexpr (std::is_same_v<Child, ParticleSpecies>)
    {
        for (auto &[_, right] : asChild().particlePatches)
        {
            (void)_;
            right
                .ScientificDefaults<BaseRecord<PatchRecordComponent>>::finalize(
                    at);
        }
    }
}

template <typename Child>
void ScientificDefaults<Child>::addDefaults()
{
    std::cout << "Adding defaults for '" << asChild().myPath().openPMDPath()
              << "'" << std::endl;

    // First some verifications
    if constexpr (auxiliary::IsTemplateBaseOf_v<BaseRecord, Child>)
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
        defaultAttribute("time", 0.).withSetter (&Iteration::setTime)();
        defaultAttribute("dt", 1.).withSetter (&Iteration::setDt)();
        defaultAttribute("timeUnitSI", 1.0)
            .withSetter (&Iteration::setTimeUnitSI)();
    }
    else if constexpr (std::is_same_v<Child, Mesh>)
    {
        auto dimensionality = asChild().retrieveDimensionality();

        defaultAttribute("timeOffset", 0.f).withSetter (&Mesh::setTimeOffset)();
        defaultAttribute("geometry", Mesh::Geometry::cartesian)
            .withSetter (&Mesh::setGeometry)();
        defaultAttribute("dataOrder", Mesh::DataOrder::C)
            .withSetter (&Mesh::setDataOrder)();
        defaultAttribute(
            "axisLabels",
            [&]() -> std::vector<std::string> {
                switch (dimensionality)
                {
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
                            "Please verify dimensionality. Was inferred as '" +
                            std::to_string(dimensionality) +
                            "'. Seems a bit much."};
                    }
                }
                return std::vector<std::string>{"x", "y", "z"};
            })
            .template withSetter<std::vector<std::string> const &> (
                &Mesh::setAxisLabels)();
        defaultAttribute(
            "gridSpacing",
            [&]() {
                if (dimensionality < 100)
                {
                    return std::vector<double>(dimensionality, 1.0);
                }
                else
                {
                    return std::vector<double>{1.0};
                }
            })
            .template withSetter<std::vector<double> const &> (
                &Mesh::setGridSpacing)();
        defaultAttribute(
            "gridGlobalOffset",
            [&]() {
                if (dimensionality < 100)
                {
                    return std::vector<double>(dimensionality, 0.0);
                }
                else
                {
                    return std::vector<double>{0.0};
                }
            })
            .template withSetter<std::vector<double> const &> (
                &Mesh::setGridGlobalOffset)();

        addParentDefaults<BaseRecord<MeshRecordComponent>>();
    }
    else if constexpr (std::is_same_v<Child, Record>)
    {
        defaultAttribute("timeOffset", 0.f)
            .withSetter (&Record::setTimeOffset)();
        auto const &keyInParent = asChild().writable().ownKeyWithinParent;

        if (keyInParent == "position" || keyInParent == "positionOffset")
        {
            addDefaultFor<unit_representations::AsMap const &>(
                "unitDimension",
                []() {
                    return unit_representations::AsMap{{UnitDimension::L, 1.0}};
                },
                &Record::setUnitDimension);
        }

        addParentDefaults<BaseRecord<RecordComponent>>();
    }
    else if constexpr (std::is_same_v<Child, PatchRecord>)
    {
        addParentDefaults<BaseRecord<PatchRecordComponent>>();
    }
    else if constexpr (std::is_same_v<Child, RecordComponent>)
    {
        defaultAttribute("unitSI", 1.0)
            .withSetter (&RecordComponent::setUnitSI)();
    }
    else if constexpr (std::is_same_v<Child, MeshRecordComponent>)
    {
        // position
        auto dimensionality = asChild().getDimensionality();
        defaultAttribute("position", [&]() {
            if (dimensionality < 100)
            {
                return std::vector<double>(dimensionality, 0.5);
            }
            else
            {
                return std::vector<double>{0.0};
            }
        }).withSetter (&MeshRecordComponent::setPosition)();
        addParentDefaults<RecordComponent>();
    }
    else if constexpr (std::is_same_v<Child, PatchRecordComponent>)
    {
        addParentDefaults<RecordComponent>();
    }
    else if constexpr (auxiliary::IsTemplateBaseOf_v<BaseRecord, Child>)
    {
        addDefaultFor<unit_representations::AsArray const &>(
            "unitDimension", unit_representations::AsArray{});
    }
}

template class ScientificDefaults<Iteration>;
template class ScientificDefaults<Mesh>;
template class ScientificDefaults<MeshRecordComponent>;
template class ScientificDefaults<RecordComponent>;
template class ScientificDefaults<PatchRecordComponent>;
template class ScientificDefaults<ParticleSpecies>;
template class ScientificDefaults<Record>;
template class ScientificDefaults<BaseRecord<MeshRecordComponent>>;
} // namespace openPMD::internal
