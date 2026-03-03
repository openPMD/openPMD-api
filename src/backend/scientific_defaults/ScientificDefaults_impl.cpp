
#include "openPMD/backend/scientific_defaults/ScientificDefaults_impl.hpp"

#include "openPMD/Datatype.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/ThrowError.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/scientific_defaults/ScientificDefaults.hpp"

#include "openPMD/Datatype.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Writable.hpp"
#include <functional>

#include <iostream>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace openPMD::internal
{

// Helper function to set geometry based on string value
auto setMeshGeometryFromString(Mesh &mesh, std::string val)
    -> std::optional<error::ReadError>
{
    if ("cartesian" == val)
        mesh.setGeometry(Mesh::Geometry::cartesian);
    else if ("thetaMode" == val)
        mesh.setGeometry(Mesh::Geometry::thetaMode);
    else if ("cylindrical" == val)
        mesh.setGeometry(Mesh::Geometry::cylindrical);
    else if ("spherical" == val)
        mesh.setGeometry(Mesh::Geometry::spherical);
    else
        mesh.setGeometry(std::move(val));
    return std::nullopt;
}

// Helper function to set data order based on char value
auto setMeshDataOrderFromChar(Mesh &mesh, char val)
    -> std::optional<error::ReadError>
{
    if (val == 'C' || val == 'F')
    {
        mesh.setDataOrder(static_cast<Mesh::DataOrder>(val));
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
}

auto RequireVector::operator()(
    Attributable &record, char const *attrName, Attribute const &attr)
    -> std::optional<error::ReadError>
{
    auto res = attr.requireVector();
    using res_t = std::optional<error::ReadError>;
    return std::visit(
        auxiliary::overloaded{
            [](std::runtime_error const &err) -> res_t {
                std::string msg = "Expected a vector type: ";
                msg += err.what();
                return error::ReadError(
                    error::AffectedObject::Attribute,
                    error::Reason::UnexpectedContent,
                    std::nullopt,
                    std::move(msg));
            },
            [&](Attribute converted_attr) -> res_t {
                record.setAttribute(attrName, std::move(converted_attr));
                return std::nullopt;
            }},
        std::move(res));
}

auto RequireScalar::operator()(
    Attributable &record, char const *attrName, Attribute const &attr)
    -> std::optional<error::ReadError>
{
    auto res = attr.requireScalar();
    using res_t = std::optional<error::ReadError>;
    return std::visit(
        auxiliary::overloaded{
            [](std::runtime_error const &err) -> res_t {
                std::string msg = "Expected a scalar type: ";
                msg += err.what();
                return error::ReadError(
                    error::AffectedObject::Attribute,
                    error::Reason::UnexpectedContent,
                    std::nullopt,
                    std::move(msg));
            },
            [&](Attribute converted_attr) -> res_t {
                record.setAttribute(attrName, std::move(converted_attr));
                return std::nullopt;
            }},
        std::move(res));
}

// 3. AttributeReader class implementations
AttributeReader::AttributeReader(
    std::deque<Datatype> eligibleDatatypes_in,
    std::optional<std::shared_ptr<ProcessAttribute>> processAttribute_in)
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
        auto maybe_error = (**processAttribute)(record, attrName, a);
        if (maybe_error.has_value())
        {
            return *maybe_error;
        }
    }
    return attribute_read_result::Success{};
}

// 4. ConfigAttribute class implementations
ConfigAttribute::ConfigAttribute(
    Attributable &child_in, char const *attrName_in)
    : child(child_in), attrName(attrName_in)
{}

auto ConfigAttribute::withReader(
    std::deque<Datatype> eligibleDatatypes,
    std::optional<std::shared_ptr<ProcessAttribute>> processAttribute)
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

std::shared_ptr<ProcessAttribute> require_scalar =
    std::make_shared<RequireScalar>();
// try converting to vectors (e.g. when a scalar or an array is given)
std::shared_ptr<ProcessAttribute> require_vector =
    std::make_shared<RequireVector>();

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
} // namespace openPMD::internal
