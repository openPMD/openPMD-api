#include "openPMD/backend/ScientificDefaults.hpp"
#include "openPMD/Datatype.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/ThrowError.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/ScientificDefaults_internal.hpp"

#include "openPMD/Datatype.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Record.hpp"
#include "openPMD/UnitDimension.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"
#include "openPMD/backend/Variant_internal.hpp"
#include "openPMD/backend/Writable.hpp"
#include <functional>

#include <iostream>
#include <optional>
#include <type_traits>
#include <utility>

namespace openPMD::internal
{
namespace
{
    template <typename T>
    auto write_val_to_stderr(T const &val) -> std::ostream &
    {
        if constexpr (auxiliary::IsVector_v<T> || auxiliary::IsArray_v<T>)
        {
            auxiliary::write_vec_to_stream(std::cerr, val);
        }
        else if constexpr (std::is_same_v<T, unit_representations::AsMap>)
        {
            std::cerr << "Unit_Map";
        }
        else
        {
            std::cerr << val;
        }
        return std::cerr;
    }
    auto write_to_stderr(Attribute const &a) -> std::ostream &
    {
        std::visit(
            [](auto const &val) { write_val_to_stderr(val); },
            a.getVariant<attribute_types>());
        return std::cerr;
    }
} // namespace

AttributeReader::AttributeReader(
    std::deque<Datatype> eligibleDatatypes_in,
    std::optional<process_attribute_type> processAttribute_in)
    : eligibleDatatypes(std::move(eligibleDatatypes_in))
    , processAttribute(std::move(processAttribute_in))
{}

auto AttributeReader::operator()(
    Attributable &record,
    char const *attrName,
    Attribute const &a,
    std::deque<Datatype> unmatched_so_far) -> AttributeReadResult
{
    if (std::find(
            eligibleDatatypes.begin(), eligibleDatatypes.end(), a.dtype) ==
        eligibleDatatypes.end())
    {
        auto res =
            attribute_read_result::TypeUnmatched{std::move(unmatched_so_far)};
        for (auto dt : this->eligibleDatatypes)
        {
            res.expectedDatatypes.push_back(dt);
        }
        return res;
    }
    if (processAttribute.has_value())
    {
        auto maybe_error = (*processAttribute)(record, attrName, a);
        if (maybe_error.has_value())
        {
            return *maybe_error;
        }
    }
    return attribute_read_result::Success{};
}

ConfigAttribute::ConfigAttribute(
    Attributable &child_in, char const *attrName_in)
    : child(child_in), attrName(attrName_in)
{}

template <typename RecordType, typename S, typename GetDefaultValue>
auto ConfigAttribute::withSetter(
    GetDefaultValue &&getDefaultVal,
    set_default_val_t<
        RecordType,
        std::conditional_t<
            std::is_void_v<S>,
            detail::CallResult_t<GetDefaultValue>,
            S>> setDefaultVal) -> ConfigAttribute &
{
    initDefaultAttribute = [getDefaultVal_lambda =
                                std::forward<GetDefaultValue>(getDefaultVal),
                            setDefaultVal](Attributable &attr) {
        RecordType *record = dynamic_cast<RecordType *>(&attr);
        if (!record)
        {
            throw error::Internal("dynamic cast failure");
        }
        if constexpr (detail::IsCallable_v<GetDefaultValue>)
        {
            ((*record).*setDefaultVal)(getDefaultVal_lambda());
        }
        else
        {
            ((*record).*setDefaultVal)(std::move(getDefaultVal_lambda));
        }
    };
    return *this;
}

template <typename DefaultValue>
auto ConfigAttribute::withGenericSetter(DefaultValue &&defaultVal)
    -> ConfigAttribute &
{
    initDefaultAttribute =
        [this, defaultVal_lambda = std::forward<DefaultValue &&>(defaultVal)](
            Attributable &attr) {
            attr.setAttribute(this->attrName, std::move(defaultVal_lambda));
        };
    return *this;
}

auto ConfigAttribute::withReader(
    std::deque<Datatype> eligibleDatatypes,
    std::optional<AttributeReader::process_attribute_type> processAttribute)
    -> ConfigAttribute &
{
    this->attributeReaders.emplace_back(
        std::move(eligibleDatatypes), std::move(processAttribute));
    return *this;
}

void ConfigAttribute::write()
{
    if (this->child.containsAttribute(this->attrName) ||
        !this->initDefaultAttribute.has_value())
    {
        return;
    }
    this->initDefaultAttribute.operator*()(this->child);
}

void ConfigAttribute::read()
{
    if (attributeReaders.empty())
    {
        // No readers emplaced for this attribute
        return;
    }
    AttributeReadResult res = attribute_read_result::TypeUnmatched{};
    Parameter<Operation::READ_ATT> aRead;
    aRead.name = this->attrName;
    auto IOHandler = this->child.IOHandler();
    IOHandler->enqueue(IOTask(&this->child, aRead));
    try
    {
        IOHandler->flush(defaultFlushParams);
    }
    catch (error::ReadError const &e)
    {
        std::cerr << "Could not read expected attribute '" << this->attrName
                  << "' in '" << this->child.myPath().openPMDPath() << ".";
        if (this->initDefaultAttribute.has_value())
        {
            std::cerr << " Will initialize it with a default value.";
            this->initDefaultAttribute.operator*()(this->child);
        }
        std::cerr << " Original error: " << e.what() << std::endl;
        return;
    }

    Attribute attribute(Attribute::from_any, std::move(*aRead.m_resource));
    for (auto &attributeReader : attributeReaders)
    {

        if (auto *not_matched =
                std::get_if<attribute_read_result::TypeUnmatched>(&res))
        {
            res = attributeReader(
                this->child,
                this->attrName,
                attribute,
                std::move(not_matched->expectedDatatypes));
        }
        else
        {
            break;
        }
    }
    auto dt = attribute.dtype;
    std::visit(
        auxiliary::overloaded{
            [&](attribute_read_result::TypeUnmatched &&type_unmatched) {
                std::cerr << "Unexpected type '" << dt << "' for attribute '"
                          << this->attrName << "' in '"
                          << this->child.myPath().openPMDPath()
                          << "' with value '";
                write_to_stderr(attribute) << "'. Expected one of ";
                auxiliary::write_vec_to_stream(
                    std::cerr, type_unmatched.expectedDatatypes)
                    << " or convertible to such a type." << std::endl;
            },
            [&](error::ReadError const &err) {
                std::cerr << "Unexpected error while trying to read "
                             "attribute '"
                          << this->attrName << "' in '"
                          << this->child.myPath().openPMDPath()
                          << "'' with value '";
                write_to_stderr(attribute) << "': " << err.what() << std::endl;
            },
            [](attribute_read_result::Success) { /* no-op */ }},
        std::move(res));
}

void ConfigAttribute::operator()(WriteOrRead wor)
{
    switch (wor)
    {
    case WriteOrRead::Write:
        write();
        break;
    case WriteOrRead::Read:
        read();
        break;
    }
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

namespace
{
    ConfigAttribute::process_attribute_type require_scalar =
        [](Attributable &record,
           char const *attrName,
           Attribute const &attr) -> std::optional<error::ReadError> {
        return std::visit(
            [&](auto const &attr_val) -> std::optional<error::ReadError> {
                using actual_type = std::remove_cv_t<
                    std::remove_reference_t<decltype(attr_val)>>;
                if constexpr (
                    auxiliary::IsVector_v<actual_type> ||
                    auxiliary::IsArray_v<actual_type>)
                {
                    using base_type = typename actual_type::value_type;
                    auto converted_or_error =
                        detail::doConvert<actual_type, base_type>(&attr_val);
                    return std::visit(
                        auxiliary::overloaded{
                            [&](base_type casted_val)
                                -> std::optional<error::ReadError> {
                                record.setAttribute<base_type>(
                                    attrName, std::move(casted_val));
                                return std::nullopt;
                            },
                            [](std::runtime_error const &err)
                                -> std::optional<error::ReadError> {
                                return error::ReadError(
                                    error::AffectedObject::Attribute,
                                    error::Reason::UnexpectedContent,
                                    std::nullopt,
                                    std::string("Expected a scalar type: ") +
                                        err.what());
                            }},
                        converted_or_error);
                }
                else
                {
                    return std::nullopt;
                }
            },
            attr.getVariant<attribute_types>());
    };

    ConfigAttribute::process_attribute_type require_vector =
        [](Attributable &record,
           char const *attrName,
           Attribute const &attr) -> std::optional<error::ReadError> {
        return std::visit(
            [&](auto const &attr_val) -> std::optional<error::ReadError> {
                using actual_type = std::remove_cv_t<
                    std::remove_reference_t<decltype(attr_val)>>;
                if constexpr (std::is_same_v<actual_type, bool>)
                {
                    return error::ReadError(
                        error::AffectedObject::Attribute,
                        error::Reason::UnexpectedContent,
                        std::nullopt,
                        "Expected a vector type, found a boolean.");
                }
                else if constexpr (!auxiliary::IsVector_v<actual_type>)
                {
                    using base_type = auxiliary::ScalarType_t<actual_type>;

                    auto converted_or_error =
                        detail::doConvert<actual_type, std::vector<base_type>>(
                            &attr_val);
                    return std::visit(
                        auxiliary::overloaded{
                            [&](std::vector<base_type> casted_val)
                                -> std::optional<error::ReadError> {
                                record.setAttribute<std::vector<base_type>>(
                                    attrName, std::move(casted_val));
                                return std::nullopt;
                            },
                            [](std::runtime_error const &err)
                                -> std::optional<error::ReadError> {
                                return error::ReadError(
                                    error::AffectedObject::Attribute,
                                    error::Reason::UnexpectedContent,
                                    std::nullopt,
                                    std::string("Expected a scalar type: ") +
                                        err.what());
                            }},
                        converted_or_error);
                }
                else
                {
                    return std::nullopt;
                }
            },
            attr.getVariant<attribute_types>());
    };

    template <typename T, typename Fun>
    auto require_type_impl(Fun &&fun) -> AttributeReader::process_attribute_type
    {
        return [fun_lambda = std::forward<Fun>(fun)](
                   Attributable &record,
                   char const *attrName,
                   Attribute const &attr) -> std::optional<error::ReadError> {
            return std::visit(
                [&](auto const &attr_val) -> std::optional<error::ReadError> {
                    using actual_type = std::remove_cv_t<
                        std::remove_reference_t<decltype(attr_val)>>;

                    auto converted_or_error =
                        detail::doConvert<actual_type, T>(&attr_val);
                    return std::visit(
                        auxiliary::overloaded{
                            [&](T casted_val)
                                -> std::optional<error::ReadError> {
                                if constexpr (std::is_void_v<
                                                  std::invoke_result_t<
                                                      decltype(fun_lambda),
                                                      Attributable &,
                                                      char const *,
                                                      T>>)
                                {
                                    std::move(fun_lambda)(
                                        record,
                                        attrName,
                                        std::move(casted_val));
                                    return std::nullopt;
                                }
                                else
                                {
                                    return std::move(fun_lambda)(
                                        record,
                                        attrName,
                                        std::move(casted_val));
                                }
                            },
                            [](std::runtime_error const &err)
                                -> std::optional<error::ReadError> {
                                return error::ReadError(
                                    error::AffectedObject::Attribute,
                                    error::Reason::UnexpectedContent,
                                    std::nullopt,
                                    std::string("Expected a scalar type: ") +
                                        err.what());
                            }},
                        converted_or_error);
                },
                attr.getVariant<attribute_types>());
        };
    }
    // namespace

    template <typename T, typename Fun>
    auto require_type(Fun &&fun) -> ConfigAttribute::process_attribute_type
    {
        return require_type_impl<T>([fun_lambda = std::forward<Fun>(fun)](
                                        Attributable &, char const *, T val) {
            std::move(fun_lambda)(std::move(val));
        });
    }

    template <typename T>
    auto require_type() -> ConfigAttribute::process_attribute_type
    {
        return require_type_impl<T>(
            [](Attributable &record, char const *attrName, T val) {
                record.template setAttribute<T>(attrName, std::move(val));
            });
    }

    auto get_float_types() -> std::deque<Datatype>
    {
        std::deque<Datatype> res;
        for (auto dt : openPMD_Datatypes())
        {
            if (isFloatingPoint(dt))
            {
                res.push_back(dt);
            }
        }
        res.push_back(Datatype::ARR_DBL_7);
        return res;
    }

    auto get_string_types() -> std::deque<Datatype>
    {
        return {
            Datatype::STRING,
            Datatype::VEC_STRING,
            Datatype::CHAR,
            Datatype::VEC_CHAR,
            Datatype::UCHAR,
            Datatype::VEC_UCHAR,
            Datatype::SCHAR,
            Datatype::VEC_SCHAR};
    }
} // namespace

template <typename Child>
template <bool write>
void ScientificDefaults<Child>::defaults_impl(OpenpmdStandard standard)
{
    using maybe_read_error = std::optional<error::ReadError>;

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
                require_type<std::string>([this](std::string val) {
                    auto &m = asChild();
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
                }))(wor);

        defaultAttribute("dataOrder")
            .template withSetter<Mesh>(Mesh::DataOrder::C, &Mesh::setDataOrder)
            .withReader(
                string_types,
                require_type<char>([this](char val) -> maybe_read_error {
                    auto &m = this->asChild();
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
                }))(wor);

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
            .withReader(string_types, require_vector)(wor);

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
            .withReader(float_types, require_vector)(wor);

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
            .withReader(float_types, require_type<std::vector<double>>())(wor);

        defaultAttribute("timeOffset")
            .template withSetter<Mesh>(0.f, &Mesh::setTimeOffset)
            .withReader(float_types, require_scalar)(wor);

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
    else if constexpr (auxiliary::IsTemplateBaseOf_v<BaseRecord, Child>)
    {
        defaultAttribute("unitDimension")
            .withGenericSetter(unit_representations::AsArray{})
            .withReader(
                float_types,
                require_type<unit_representations::AsArray>())(wor);
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
