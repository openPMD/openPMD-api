
#include "openPMD/backend/scientific_defaults/ProcessParsedAttribute.hpp"

#include "openPMD/Error.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ThrowError.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Writable.hpp"

#include <optional>
#include <stdexcept>
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
} // namespace openPMD::internal
